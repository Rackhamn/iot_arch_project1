#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // maybe

#include <sqlite3.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define STATE_AWAIT_USER	0
#define STATE_AWAIT_TAGID	1
#define USERNAME_SIZE		32
#define TAG_ID_SIZE		32
#define TAG_DESC_SIZE		255


void clear_terminal(void) {
	printf("\e[1;1H\e[2J");
}

void print_commands(void) {
	printf("Commands:\n");
	printf(" \tquit, :q\tQuit the Program\n");
	printf(" \thelp, :h\tPrint out the Help Message\n");
	printf(" \tclear, :c\tClear the Terminal\n");
	printf(" \tusername, :u\tSelect another username\n");
}

void print_help(void) {
	clear_terminal();
	printf("Help:\n");
	printf(" once logged in, enter something like '04AABBCCDDEE'\n");
	printf(" and the example picow server will get the description.\n");
	printf("\n");
	print_commands();
	printf("\n");
}

void print_startup(void) {
	printf(" ### PICO-EX Program ###\n");
	print_commands();
}

void remove_newline_end(char * buf, ssize_t len) {
	// printf("input = \"%s\"\n", username);
	if(len > 0 && buf[len - 1] == '\n') {
		buf[len - 1] = '\0';
		// for windows:
		if(len > 1 && buf[len - 2] == '\r') {
			buf[len - 2] = '\0';
		}
    	}
	// printf("output = \"%s\"\n", username);
}

int check_username(sqlite3 * db, char * username) {

	char buf[255] = { 0 };
	snprintf(buf, 255 - 1, 
		"SELECT * FROM users WHERE username = '%s';", 
		username);

	int rc;

	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db, buf, -1, &stmt, NULL);
	if(rc != SQLITE_OK) {
		printf("Prepare call error: %i\n", rc);
		return -1;
	}

	int count = 0;
	while(sqlite3_step(stmt) == SQLITE_ROW) {
		count++;
	}

	sqlite3_finalize(stmt);

//	return (count == 0);

	if(count == 1) {	
		return 0;
	} else {
		return 1;
	}

	return -1;
}

int fetch_tag_desc(sqlite3 * db, 
	const char * username, 
	const char * tag_id, 
	char * tag_desc,
	const size_t tag_desc_size) 
{

	char buf[255] = { 0 };
	snprintf(buf, 255 - 1, 
"SELECT description FROM tags WHERE user_id IN (SELECT id FROM users WHERE username = '%s') AND tag_uid = '%s';",
		username, 
		tag_id);

	int rc;

	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db, buf, -1, &stmt, NULL);
	if(rc != SQLITE_OK) {
		printf("Prepare call error: %i\n", rc);
		return -1;
	}

	int count = 0;
	while(sqlite3_step(stmt) == SQLITE_ROW) {
		const char * zstr = (const char*)sqlite3_column_text(stmt, 0);
		size_t zstr_len = strlen(zstr);

		memcpy(tag_desc, zstr, MIN(zstr_len, tag_desc_size));
		count++;
	}

	sqlite3_finalize(stmt);

//	return (count == 1);
	
	if(count == 1) {
		return 0;
	} else {
		return 1;
	}

	return -1;
}

int main(int argc, char **argv) {
	
	sqlite3 *db;
	char * zerrmsg = NULL;
	int rc;

	if(argc <= 1) {
		fprintf(stderr, "Usage: %s $DATABASE\n", argv[0]);
		return 1;
	}

	rc = sqlite3_open(argv[1], &db);
	if(rc) {
		fprintf(stderr, "Cant open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}


	// prefab info
	print_startup();

	// setup data
	char username[USERNAME_SIZE] = { 0 };
	char tag_id[TAG_ID_SIZE] = { 0 };
	char tag_desc[TAG_DESC_SIZE] = { 0 };

	char * line = NULL;
	ssize_t line_len = 0;

	int state = STATE_AWAIT_USER;
	while(state != -1) {
		switch(state) {
			case STATE_AWAIT_USER:
			printf("Enter Username:\n");
			break;
			case STATE_AWAIT_TAGID:
			printf("Enter Tag ID:\n");
			break;
			default:

			break;
		}
		printf("> ");

		// do sanitize pls
		int ret = getline(&line, &line_len, stdin);
		if(ret == 0) {
			continue;
		}

		// Special Commands
		if(memcmp(line, "quit", 4) == 0 || memcmp(line, ":q", 2) == 0) {
			printf(" ### QUIT ### \n");
			state = -1;
			break;
		}

		if(memcmp(line, "clear", 5) == 0 || memcmp(line, ":c", 2) == 0) {
			clear_terminal();
			continue;
		}

		if(memcmp(line, "username", 8) == 0 || memcmp(line, ":u", 2) == 0) {
			memset(username, 0, USERNAME_SIZE);
			state = STATE_AWAIT_USER;
			continue;
		}

		if(memcmp(line, "help", 4) == 0 || memcmp(line, ":h", 2) == 0) {
			print_help();
			continue;
		}

		// Program Handler
		switch(state) {
			case STATE_AWAIT_USER: {
			memcpy(username, line, MIN(line_len, USERNAME_SIZE - 1));

			remove_newline_end(username, strlen(username));

			if(check_username(db, username) == 0) {
				printf("Username: %s exists! [ OK ]\n\n", 
				username);

				state = STATE_AWAIT_TAGID;
			} else {
				printf("Username: %s does not exist! [ ERR ]\n\n", 
					username);

				memset(username, 0, USERNAME_SIZE);
			}

			} break;
			case STATE_AWAIT_TAGID: {

			memcpy(tag_id, line, MIN(line_len, TAG_ID_SIZE - 1));
			
			remove_newline_end(tag_id, strlen(tag_id));

			if(fetch_tag_desc(db, 
					username, 
					tag_id, 
					tag_desc, 
					TAG_DESC_SIZE - 1) == 0) 
			{
				printf("Tag %s : %s\n\n", tag_id, tag_desc);
				memset(tag_id, 0, TAG_ID_SIZE);
				memset(tag_desc, 0, TAG_DESC_SIZE);
			}

			} break;
			default:
			break;
		}
	}
	
	if(line != NULL)
		free(line);

	sqlite3_close(db);

	return 0;
}
