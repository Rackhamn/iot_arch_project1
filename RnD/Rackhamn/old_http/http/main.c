#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <arpa/inet.h>

#include <sqlite3.h>

#ifndef PORT
#define PORT 		8080
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 	4096
#endif

/*
 * TODO:
 * 	use a htmx like js that uses listeners and async await for post/get
 * 	use separete .dll/.so that defines the functions that js maps to
 * 	then also a plugin/config file that defines the function names in the lib
 * 	and which lib its in.
 * 	+ versisioning.
 *	
 *	make it load things at anytime - onchange / cli command
 *	then the base server does not need to be shut down or maintanance
*/

/*
 * Use HttpOnly Cookies
 * Store IP + User-Agent
 * opt. Cryptographic HMAC Token instead
 * TLS Fingerprint
 *
 * GET		200, 404
 * POST		200, 201, 400, 500
 * PUT		200, 201, 204
 * DELETE	200, 204, 404
 * HEAD		
 * OPTIONS	*
 * PATCH	200, 204
 *	REST = GET, POST, PUT, DELETE etc (over json i guess)
 *
 * Headers:
 * Host
 * User-Agent
 * Content-Type
 * Content-Length
 * Authorization
 * Accept
 * Cookie
 * Referer
 *
 * custom .htaccess or httpd.conf file
 *
 * store session token in hashtable or database that returns the username
 *
 * do tag replacement from html files with {{TAG}} info.
 * 	username
 * 	tag_id
 * 	tag_desc
 * 	...
 *
 * do cookie login thingy pls
 *
 *
 * http server
 * sql-pico server
 * pico ex client
 *
 * + hotload replace of html+image files
 * + cache for loaded files (images)
 * + arena allocator & on-crash handling (signal)
*/

// map.conf:
// in-path : real-path
// $Error : real-path

// scratch buffer / thread
// arena allocator baseline
// certain pages / files should be loaded into ram 
// hashtable + vtable memory
// cloadflare-tunnel?
// json/xml parser?
// html parser + rebuilder?
// threading and messagequeue (fifo stack)

// read templated HTML file
// replace tokens with dynamic data
// render into buffer and write to client

// no?
struct html_map_s {
	int n_strings;
	int n_tags;
	char ** strings;
	char ** tags;
};

// (EXT, CT) - make hashtable
#define NUM_CONTENT_TYPES	7
#define NUM_CT			(NUM_CONTENT_TYPES * 2)
static const char * content_types[NUM_CT] = {
	"jpg", "image/jpeg", 
	"jpeg", "image/jpeg",
	"png", "image/png",
	"svg", "image/svg+xml",
	"js", "text/javascript",
	"css", "text/css",
	"html", "text/html"
};

// not used!!!
#define POST	0
#define GET	1
#define PUT	2
#define DELETE	3
static const char * http_methods[] = {
	"POST", 	// CREATE
	"GET", 		// READ
	"PUT",		// UPDATE
	"DELETE",	// DELETE
/*	
	"OPTIONS",
	"UPDATE",
	"HEAD",
	"PATCH"
*/
};

void handle_file(int client_socket, const char * path) {
	
}

void handle_img(int client_socket, const char * path) {
	path++;
	FILE * fp = fopen(path, "rb"); // r for svg
	char buf[BUFFER_SIZE] = { 0 };
	size_t bufsize = 0;

	if(fp == NULL) {
		bufsize += sprintf(buf + bufsize, "HTTP/1.1 404 Not Found\r\n\r\n");
		write(client_socket, buf, bufsize);
		close(client_socket);
		return;
	}

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// check path for content type switch
	char * content_type = NULL;
	if(1) {
		size_t path_size = strlen(path);
		
		char * pend = (char*)path + path_size;
		while(pend > path) {
			if(*pend == '.') { 
				pend++;
				break;
			}
			pend--;
		}

		for(int i = 0; i < NUM_CT; i += 2) {
			if(strcmp(pend, content_types[i]) == 0) {
				content_type = (char*)content_types[i + 1];
				break;
			}
		}
	}

	if(content_type == NULL) {
		const char * tmp = "HTTP/1.1 415 Unsupported Media\r\n\r\n";
		write(client_socket, tmp, strlen(tmp));
		return;
	}
	printf("\t### CONTENT TYPE = %s ###\n", content_type);

	bufsize += sprintf(buf + bufsize, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", content_type, size);
	
	write(client_socket, buf, bufsize);


	char ibuf[BUFFER_SIZE];
	size_t iread = 0;
	while((iread = fread(ibuf, 1, BUFFER_SIZE, fp)) > 0) {
		write(client_socket, ibuf, iread);
	}

	fclose(fp);
}

size_t fetch_tags_by_username(char * buf, size_t * pbufsize, char * username) {
	size_t bufsize = *pbufsize;
	size_t obufsize = bufsize;

	sqlite3 * db;
	int rc = sqlite3_open("../ex1", &db);
     
    	char cmdbuf[1024] = { 0 };
	snprintf(cmdbuf, 1024 - 1, "SELECT * FROM tags WHERE user_id IN (SELECT id FROM users WHERE username = '%s');", username); 
	sqlite3_stmt * stmt;

	rc = sqlite3_prepare_v2(db, cmdbuf, -1, &stmt, NULL);
        if(rc != SQLITE_OK) {
                printf("Prepare call error: %i\n", rc);
        }

	bufsize += sprintf(buf + bufsize, "<div id=\"tags_div\">\n");

	// legend titles
	bufsize += sprintf(buf + bufsize, "<table><thead><tr><th scope=\"col\">UID</th><th scope=\"col\">Description</th></thead>\n");

	bufsize += sprintf(buf + bufsize, "<tbody>\n");
	int id = 1;
        while(sqlite3_step(stmt) == SQLITE_ROW) {
                const char * tag_uid_zstr = (const char*)sqlite3_column_text(stmt, 1);
                const char * tag_desc = (const char*)sqlite3_column_text(stmt, 4);

		bufsize += sprintf(buf + bufsize, "<tr><td>%s</td>\n<td contenteditable=\"true\" data-id=\"%s\" onfocus=\"recordTag(this)\" onblur=\"updateTag(this)\" onkeydown=\"checkEnter(event, this)\">%s</td>\n</tr>\n",
		tag_uid_zstr, tag_uid_zstr, tag_desc);
		id++;
        }

	bufsize += sprintf(buf + bufsize, "</tbody>\n");

	bufsize += sprintf(buf + bufsize, "</div>\n");

        sqlite3_finalize(stmt);
	sqlite3_close(db);
	
	*pbufsize = bufsize;
	return (bufsize - obufsize);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Read error");
        close(client_socket);
        return;
    }

    // Print received request
    buffer[bytes_read] = '\0';
    printf("Received request:\n%s\n", buffer);

    char buf[BUFFER_SIZE] = { 0 };
    size_t bufsize = 0;

    // "GET / HTTP/1.1" -> "/index.html"
    // "GET /img/SQLite370.svg" -> SVG "SQLite370.svg"
    // "GET /img/why.jpeg" -> JPEG "why.jpeg" | "why.jpg"

    if(memcmp(buffer, "GET /index.html", 15) == 0) {
    	handle_img(client_socket, "/index.html");
	goto lpostend;
    }
    if(memcmp(buffer, "GET ", 4) == 0) {
	printf("# GET\n");
char * offset = strstr(buffer, "GET ");
   if(offset != NULL) {
	// "/img/", 5

   // ??? calc inner length of uri ???	   
	int uri_len = 0;
	char * p = offset + 4;
	while(*p != ' ') {
		p++;
	}
	uri_len = p - (offset + 4);

   //  if(memcmp(offset + 4, "/img/", 5) == 0 || memcmp(offset + 4, "/style.css", 10) == 0) {
	// copy out the inner length of uri..???.
	// memcpy(path_buf, offset + 4, uri_len);
	if(uri_len > 1) {   
	char path_buf[255] = { 0 };
	char * p = offset + 4;
	// shit - horrid
	size_t path_size = 0;
	while(*p != ' ') {
		path_buf[path_size++] = *p;
		p++;
	}
	path_buf[path_size] = '\0';
    	
    	printf("### DO: GET '%s'\n", path_buf);
	handle_img(client_socket, path_buf);
    } else {
    bufsize += sprintf(buf + bufsize,
	"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");

	bufsize += sprintf(buf + bufsize, "<!DOCTYPE HTML><html><head><link rel=\"stylesheet\" href=\"style.css\"><title>SQLite C HTTP Server</title></head><body><h1>SQLite C HTTP Server!</h1>");

	bufsize += sprintf(buf + bufsize, "<p id=\"username\" contenteditable=\"true\" onblur=\"updateUser(this)\" onkeydown=\"checkEnter(event, this)\">%s</p>\n", "Username");

	bufsize += sprintf(buf + bufsize, "<div id=\"tagtable\"></div>\n");

	bufsize += sprintf(buf + bufsize, "<form action='/click' method='POST'><button type=\"submit\">Press Me</button></form>");
	bufsize += sprintf(buf + bufsize, "<img src=\"/img/SQLite370.svg\">");
	bufsize += sprintf(buf + bufsize, "<img src=\"/img/SQLite370.png\">");
	bufsize += sprintf(buf + bufsize, "<h1>Heading Test</h1>");
	bufsize += sprintf(buf + bufsize, "<script src=\"update.js\"></script>");
	bufsize += sprintf(buf + bufsize, "<button onclick=\"update_text(this)\">Click Me</button>");

	bufsize += sprintf(buf + bufsize, "</body></html>");

    // Basic response (simple HTML page)
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html>"
        "<html><head><title>My C Server</title></head>"
        "<body><h1>Welcome to My C HTTP Server!</h1></body>"
        "</html>";

	// printf("#response=\n%s\n\n", response);
	// printf("#prefer=\n%s\n\n", buf);
	
    // Send response to client
    write(client_socket, buf, bufsize);
//	write(client_socket, response, strlen(response));
	
    }
    } // !=null

    } else if(memcmp(buffer, "POST ", 5) == 0) {
	printf("# POST\n");
	// msg ... [001122334455, Initial Tag Info Help]

	char * ptr = NULL;
	if((ptr = strstr(buffer, "update_tag=")) != NULL) {
#define TAG_ID_STR_SIZE		24
#define TAG_DESC_STR_SIZE	254
	char tag_id[TAG_ID_STR_SIZE] = { 0 };
	char tag_desc[TAG_DESC_STR_SIZE] = { 0 };

	// shitty scan of x=[id, desc]
	// sscanf(buffer, "[%23[^,], %253[^]]]", tag_id, tag_desc);
	if(1) {
		char * end = buffer + bytes_read;
		char * left = NULL;
		char * sep = NULL;
		char * right = NULL;

		while(end > buffer) {
			if(right == NULL) {
				if(*end == ']')
					right = end;
			} else if(left == NULL) {
				if(*end == '[') {
					left = end;
					break;
				}
			}
			end--;
		}

		if(right != NULL && left != NULL) {
			end = left;
			while(end < right) {
				if(*end == ',' && *(end+1) == ' ') {
					sep = end;
					break;
				}
				end++;
			}
		}

		if(right != NULL && left != NULL && sep != NULL) {
			// extract bs
			memcpy(tag_id, left+1, (sep-left)-1);
			memcpy(tag_desc, sep+2, (right-sep)-2);
		}

	}
	// ^ fmt: xyz=[TAGUID, TAGDESC]
	printf(" ### ID: %s, DESC: '%s'\n", tag_id, tag_desc);

	// sql shit
	sqlite3 *db;
	int rc;

	rc = sqlite3_open("../ex1", &db);
	if(rc) { sqlite3_close(db); goto lpostend; }

	char cmd[4096] = { 0 };
	sprintf(cmd, 
		"UPDATE tags SET description = '%s' WHERE tag_uid = '%s';", 
		tag_desc, tag_id);

	sqlite3_stmt * stmt;
	rc = sqlite3_prepare_v2(db, cmd, -1, &stmt, NULL);
	if(rc != SQLITE_OK) {
		printf("SQL\n");
	}
	while(sqlite3_step(stmt) == SQLITE_ROW) {
		// --- 
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);


	const char * response = 
		"HTTP/1.1 200 OK\r\n\r\n";
	write(client_socket, response, strlen(response));

	}

	// should be a GET really
	if((ptr = strstr(buffer, "update_user=")) != NULL) {
		char * end = buffer + bytes_read;
		ptr += 13;

		char username[255] = { 0 };
		memcpy(username, ptr, (end - 1) - ptr);
		printf("new username set: %s\n", username);

		size_t tmp = fetch_tags_by_username(buf, &bufsize, username);
		if(tmp > 0) {
			printf("update_user=\"200 OK\"\n");
			// success :)
			char _buf[1024] = { 0 };
			size_t _len = snprintf(_buf, 1023, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %li\r\nConnection: close\r\n\r\n", tmp);

			write(client_socket, _buf, _len);
		}
		write(client_socket, buf, bufsize); 
	}
lpostend:
    }
    else if(memcmp(buffer, "UPDATE " , 7) == 0) {
	printf("# UPDATE\n");
    }
    else if(memcmp(buffer, "PUT ", 4) == 0) {
	printf("# PUT\n");
    }
    else if(memcmp(buffer, "DELETE ", 7) == 0) {
	printf("# DELETE\n");
    }
    else if(memcmp(buffer, "HEAD ", 5) == 0) {
	    printf("# HEAD\n");
    }
    else if(memcmp(buffer, "PATCH ", 6) == 0) {
	    printf("# PATCH\n");
    }
    else if(memcmp(buffer, "OPTIONS ", 8) == 0) {
    } else {
	printf("# UNKNOWN!!!\n");
    }

       // Close connection
    close(client_socket);
}

struct context_s {
	int server_socket, client_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len;
};
typedef struct context_s context_t;
context_t ctx;

void cleanup_and_exit(int rval) {
	if(ctx.client_socket > 0) {
		close(ctx.client_socket);
	}
	if(ctx.server_socket > 0) {
		close(ctx.server_socket);
	}
	exit(rval);
}

void sig_handler(int sig) {
	if(sig == SIGINT) {
		printf("\n\tCaught SIGINT! Do cleanup_and_exit(1)\n");
		cleanup_and_exit(1);
	}
}

int main() {
/*
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
*/
	memset(&ctx, 0, sizeof(context_t));
    ctx.client_len = sizeof(ctx.client_addr);

    signal(SIGINT, sig_handler);

    pid_t pid = getpid();
    printf(" ### HTTP SERVER + SQLITE3 COMM : pid = %d ###\n", (int)pid);

    // 1. Create server socket
    ctx.server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx.server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Set server address and bind
    ctx.server_addr.sin_family = AF_INET;
    ctx.server_addr.sin_addr.s_addr = INADDR_ANY;
    ctx.server_addr.sin_port = htons(PORT);

    if (bind(ctx.server_socket, (struct sockaddr *)&ctx.server_addr, sizeof(ctx.server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 3. Start listening for connections
    if (listen(ctx.server_socket, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server running on http://localhost:%d\n", PORT);

    // 4. Accept and handle client requests
    // do fork / thread it pls
    while (1) {
        ctx.client_socket = accept(ctx.server_socket, (struct sockaddr *)&ctx.client_addr, &ctx.client_len);
        if (ctx.client_socket < 0) {
            perror("Accept failed");
            continue;
        }
	// 5. main handler
        handle_client(ctx.client_socket);
    }

    // 6. Close server socket
    close(ctx.server_socket);
    return 0;
}

