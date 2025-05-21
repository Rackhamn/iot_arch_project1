#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
// #define _GNU_SOURCE
#include <errno.h>
#include <unistd.h> // gettid()
#include <pthread.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdarg.h>
#include <getopt.h> // lib?
#include <ctype.h> // for hexencode/hexdecode
#include <sqlite3.h>
#define DB_PATH_STR "../../Gabbemannen00/updated_database.db" 

// ARENA
#include "../../arena/arena.h"

// JSON
#include "../../json/src/json_cpac.h"
#include "../../json/src/json_dump.h"
#include "../../json/src/json_find.h"
#include "../../json/src/json_make.h"
#include "../../json/src/json_write.h"

#include "sha256.h"
#include "http.h"

#include "favicon.h"


int is_valid_fd(int fd) {
	return  (fcntl(fd, F_GETFL) != -1) ||
		(errno != EBADF);
}



// TODO:
// 	add CLI interface 
// 		change the debug print / log level
// 		and let task & threads use LOG_printf(level, fmt, ...)

// TODO;
// 	load all files into RAM
// 	keep virtual file system indexed
// 	use RAM as cache for the files
// 	and reload them on change!

// TODO:
// 	use epoll
// 	use events
// 	store sessions and active clients
// 		send set cookie on new session
// 		use HttpOnly & Secure & SameSite=Strict
// 		authorize with cookie
// 	non-blocking sockkets
//	keep a single copy per file in RAM + path match (READONLY)
//			PROT_READ + mmap()
//		make each file fit in a pow2 or +page sized buffer
//		> that way we can reload it into the same buffer (maybe needs a lock?)
//		> or double buffering ;)
//	load all files on startup
//		use inotify to hotreload
//	HTTPS + fake SSL cert?
//	whitelist / blacklist for directories and files (dont want traveral exploits)
//
struct file_cache_entry_s {
	uint32_t path_offset;
	size_t path_size;

	uint32_t data_offset;
	size_t data_size;
};
typedef struct file_cache_entry_s fc_entry_t;

#define FC_ENTRY_ZERO()	(fc_entry_t){ 0, 0, 0, 0 }
#define GET_FC_PATH(fc, entry) (char*)(fc.arena.data + entry.path_offset)
#define GET_FC_DATA(fc, entry) (char*)(fc.arena.data + entry.data_offset)

#define MAX_FILE_CACHE_SIZE	16
struct file_cache_s {
	// arena_t * arena;
	uint32_t num_files;
	fc_entry_t files[MAX_FILE_CACHE_SIZE];
};
typedef struct file_cache_s fc_t;


char * cat_large_img_data = NULL;
char * cat_large_img_path = NULL;
size_t cat_large_img_data_size = 0;

#if 0
session_t session; // tmp

// TODO: needs a shared mutex for the threads!
#define MAX_SESSIONS	128
#define SESSION_HT_SIZE	(MAX_SESSIONS * 4)
session_t session_ht[SESSION_HT_SIZE];
#endif 

// global state - plz make threaded...
context_t ctx;

const unsigned char favicon_icox[] = {
	// * https://en.wikipedia.org/wiki/ICO_(file_format)
    // ICONDIR (6 bytes)
    0x00, 0x00,       // Reserved
    0x01, 0x00,       // ICO type
    0x01, 0x00,       // One image

    // ICONDIRENTRY (16 bytes)
    0x10,             // Width = 16
    0x10,             // Height = 16
    0x10,             // 16 colors
    0x00,             // Reserved
    0x01, 0x00,       // Planes
    0x04, 0x00,       // Bits per pixel = 4
    0x68, 0x01, 0x00, 0x00, // Image size = 360 bytes
    0x16, 0x00, 0x00, 0x00, // Offset = 22 bytes

    // * https://en.wikipedia.org/wiki/BMP_file_format
    // BITMAPINFOHEADER (40 bytes)
    0x28, 0x00, 0x00, 0x00, // Header size = 40
    0x10, 0x00, 0x00, 0x00, // Width = 16
    0x20, 0x00, 0x00, 0x00, // Height = 32 (16 image + 16 mask)
    0x01, 0x00,             // Planes = 1
    0x04, 0x00,             // BPP = 4
    0x00, 0x00, 0x00, 0x00, // Compression = none
    0xA0, 0x00, 0x00, 0x00, // Image data size = 160
    0x00, 0x00, 0x00, 0x00, // X PPM
    0x00, 0x00, 0x00, 0x00, // Y PPM
    0x10, 0x00, 0x00, 0x00, // Colors used = 16
    0x00, 0x00, 0x00, 0x00, // Important colors

    // PALETTE (16 × 4 bytes = 64 bytes)
    0x00, 0x00, 0x00, 0x00, // 0: black
    0x00, 0x00, 0xFF, 0x00, // 1: red
    0x00, 0xFF, 0x00, 0x00, // 2: green
    0xFF, 0x00, 0x00, 0x00, // 3: blue
    0xFF, 0xFF, 0xFF, 0x00, // 4: white (used in background)
	// can be removed ;)
    0x00, 0x00, 0x00, 0x00, // 5
    0x00, 0x00, 0x00, 0x00, // 6
    0x00, 0x00, 0x00, 0x00, // 7
    0x00, 0x00, 0x00, 0x00, // 8
    0x00, 0x00, 0x00, 0x00, // 9
    0x00, 0x00, 0x00, 0x00, // A
    0x00, 0x00, 0x00, 0x00, // B
    0x00, 0x00, 0x00, 0x00, // C
    0x00, 0x00, 0x00, 0x00, // D
    0x00, 0x00, 0x00, 0x00, // E
    0x00, 0x00, 0x00, 0x00, // F

    // PIXEL DATA (16 rows × 8 bytes = 128 bytes)
    // bottom to top (row 15 at bottom)
    // Use palette index:
    //   1 = red, 3 = blue, 2 = green, 0 = black, 4 = white (background)

    // Row 15
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 14
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 13 (black)
    0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
    // Row 12
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 11
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 10
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 9
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 8 (green)
    0x22,0x22,0x22,0x22, 0x22,0x22,0x22,0x22,
    // Row 7
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 6
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 5
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 4 (blue)
    0x33,0x33,0x33,0x33, 0x33,0x33,0x33,0x33,
    // Row 3
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 2
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 1
    0x44,0x44,0x44,0x44, 0x44,0x44,0x44,0x44,
    // Row 0 (red)
    0x11,0x11,0x11,0x11, 0x11,0x11,0x11,0x11,

    // AND mask (16×1-bit rows padded to 4 bytes = 64 bytes)
    // all opaque (set to 0)
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00
};

#ifndef HEX_CODEC_H
#define HEX_CODEC_H

// for N bytes in, N*2 bytes will be encoded
void hex_encode(char * out, unsigned char * in, size_t len) {
	const char hex[] = "0123456789ABCDEF";
	for(size_t i = 0; i < len; i++) {
		out[i*2] = hex[(in[i] >> 4) & 0xF];
		out[i*2+1] = hex[in[i] & 0xF];
	}
	out[len * 2] = '\0';
}

#define hex2u8(x) \
	((isdigit(x)) ? x - '0' : \
	(isxdigit(x)) ? (tolower(x) - 'a' + 10) : 255)

// for N bytes in, then N/2 will be written to
int hex_decode(unsigned char * out, size_t out_len, char * hex) {
	size_t i = 0;

	size_t len = out_len * 2;
	while(i < len) {
		char c1, c2;
		unsigned char hi, lo;

		c1 = hex[i];
		c2 = hex[i + 1];

		hi = hex2u8(c1);
		lo = hex2u8(c2);		
		
		if(hi > 15 || lo > 15) {
			return -1;
		}	

		out[i / 2] = (hi << 4) | lo;
		i += 2;
	}

	/*
	// turns out that the string is of odd length
	if(hex[i]) {
		return -1;
	}
	*/

	return (int)(i / 2);
}
#endif

#ifndef THREAD_POOL_H
#define THREAD_POOL_H
// glibc hasnt added a wrapper for gettid on my system...
// so we now have to do this.
pid_t gettid(void) {
	return syscall(SYS_gettid);
}

//TODO: 
//	create struct thread_context_s please.
//	everything can be in there and passed by ptr.
// ex:
//	__thread thread_context_t thread_ctx;

// thread local specific data
// if hash is to be used outside of a given thread
// then the data needs to be copied over to a global array / other thread data
// TODO: make struct thread_ctx plz
__thread pid_t thread_id;

// TODO: this should be somewhere else i think...
// (part of the request buffer maybe?)
// altough who knows...
__thread arena_t thread_json_arena;

__thread char * thread_request_buffer;
__thread size_t thread_request_buffer_size;
__thread char * thread_response_buffer;
__thread size_t thread_response_buffer_size;

// TODO: store into struct plz
// 	we just store socket file descriptors (signed 32-bit integers) in it!
int task_queue[MAX_QUEUE] = { 0 };
int task_front = 0;
int task_rear = 0;
int task_count = 0;

volatile sig_atomic_t running = 1;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t login_mutex = PTHREAD_MUTEX_INITIALIZER;

void task_enqueue(int client) {
	// printf("enqueue\n");
	pthread_mutex_lock(&queue_mutex);
	while(task_count == MAX_QUEUE && running == 1) {
		pthread_cond_wait(&queue_cond, &queue_mutex);
	}

	if(running == 0) {
		pthread_mutex_unlock(&queue_mutex);
		close(client);
		return;
	}

	// printf("push client socket\n");
	task_queue[task_rear] = client;
	task_rear = (task_rear + 1) % MAX_QUEUE;
	task_count += 1;

	// printf("enqueue unlock\n");
	pthread_cond_signal(&queue_cond);
	pthread_mutex_unlock(&queue_mutex);
}

int task_dequeue() {
	// printf("dequeue\n");
	pthread_mutex_lock(&queue_mutex);
	while(task_count == 0 && running == 1) {
		pthread_cond_wait(&queue_cond, &queue_mutex);
	}

	if(running == 0 && task_count == 0) {
		pthread_mutex_unlock(&queue_mutex);
		return -1;
	}

	// printf("get client socket\n");
	int client = task_queue[task_front];
	task_front = (task_front + 1) % MAX_QUEUE;
	task_count -= 1;

	// printf("dequeue unlock\n");
	pthread_cond_signal(&queue_cond);
	pthread_mutex_unlock(&queue_mutex);

	// printf("return client socket\n");
	return client;
}


void mt_parse_http_request(http_request_t * req) {
	char * buffer = thread_request_buffer;
#if 1
	// dump request - use verbose flags!
	printf("\n ### CLIENT REQUEST : BEGIN ###\n");
	printf("%s\n", buffer);
	printf(" ### CLIENT REQUEST : END ###\n");
#endif

	int has_cookie = 0;
	unsigned char token[128] = { 0 };

	// not safe - plz do an actual parse
	// use known cmp for method
	// use whitespace to parse for request path
	// 	also handle %xx into ascii char. what about utf8?
	// use until CLRF to parse for http version
	sscanf(buffer, "%s %s %s", req->method, req->path, req->http_version);
	printf("method: %s\n", req->method);
	printf("path: %s\n", req->path);
	printf("version: %s\n", req->http_version);

	
	// read the total len of the request content body
	size_t content_length = 0;
	char * content_length_header = strstr(buffer, "Content-Length:");
	if(content_length_header != NULL) {
		char * p = content_length_header + strlen("Content-Length:");

		int len = atoi(p);
		printf("# Content-Length: %i\n", len);
		if(len > 0) {
			content_length = len;
		}
	}
	req->content_size = content_length;

	/*
	req->headers = buffer + (strlen(req->method) + strlen(
	req->body = 
	*/
	// char * content_ptr = thread_request_buffer + req->request_size - req->content_size;
	req->has_cookie = extract_sessionid_token(thread_request_buffer, req->token);
	if(req->has_cookie == 0) {
		printf("mt_parse_http_request error - no cookie / token found!\n");
	}
}

int mt_read_http_request(http_request_t * req) {
	if(req->socket <= 0) {
		printf("client socket error!\n");
		return -1;
	}

	// todo: move into static memory per thread
	// 	like 8MB or something
	// 	plus one for the output
	// maybe this size has to be bigger?
	// or in an incrementing buffer?
	// char buffer[BUFFER_SIZE] = { 0 };
	size_t buffer_size = thread_request_buffer_size;
	char * buffer = thread_request_buffer;
	memset(thread_request_buffer, 0, thread_request_buffer_size);
	
	int bytes_read = read(req->socket, buffer, buffer_size - 1);//sizeof(buffer) - 1);
	if(bytes_read < 0) {
		printf("client read error\n");
		close(req->socket);
		return -1;
	}
	buffer[bytes_read] = '\0';
	req->request_size = bytes_read;
	
	mt_parse_http_request(req);

	return 0;
}


void * worker(void * arg) {
	thread_id = gettid();

	arena_create(&thread_json_arena, 8192);

	thread_request_buffer_size = sizeof(char) * 8192;
	thread_request_buffer = malloc(thread_request_buffer_size);
	if(thread_request_buffer == NULL) {
		printf("thread %i could not allocate request buffer!\n", thread_id);
		return NULL;
	}
	memset(thread_request_buffer, 0, thread_request_buffer_size);

	thread_response_buffer_size = sizeof(char) * 8192;
	thread_response_buffer = malloc(thread_response_buffer_size);
	if(thread_response_buffer == NULL) {
		printf("thread %i could not allocate response buffer!\n", thread_id);
		return NULL;
	}
	memset(thread_response_buffer, 0, thread_response_buffer_size);

	printf("Thread Worker Start: %i\n", thread_id);

	int status = 0;
	while(1) {
		http_request_t request = { 0 };
		request.socket = task_dequeue();

		if(request.socket < 0) {
			break;
		}

		status = mt_read_http_request(&request);
		if(status != 0) {
			printf("Err: in read_http_request()\n");
			continue;
		}

		handle_request(&request);	
	}

	arena_destroy(&thread_json_arena);

	if(thread_request_buffer != NULL) {
		free(thread_request_buffer);
		thread_request_buffer = NULL;
		thread_request_buffer_size = 0;
	}

	if(thread_response_buffer != NULL) {
		free(thread_response_buffer);
		thread_response_buffer = NULL;
		thread_response_buffer_size = 0;
	}

	printf("Thread Worker End: %i\n", thread_id);
	return NULL;
}
#endif

// separete thread 
// command line interface for the server
// can do simpler things like:
// 	list all logged in users
// 	logout a specific user
// 	logout all user
//
// notes:
// - 	would be nice to have more specific features like
// 	1. set the maximum amount of worker threads
//	2. xxx
//	> and not just use them from the compiled values.

void print_cli_help_msg(void) {
#define CLI_BMSG "%s %s"
#define TAB_MSG "\t%.*s : %s\n"

	int olen = 10;
	printf(CLI_BMSG, "[CLI]", "HTTP Server CLI Help Message:\n");
	printf(TAB_MSG, olen, "help", "Display this message");
	printf(TAB_MSG, olen, "status", "Tells the status of the HTTP server");
	printf(TAB_MSG, olen, "quit", "Quits the HTTP Server threads and the program");
	printf("\n");

#undef CLI_BMSG
#undef TAB_MSG
}

int handle_cli_command(const char * cmd, size_t cmd_len) {
	if(strcmp(cmd, "help") == 0) {
		print_cli_help_msg();	
	}
	else if(strcmp(cmd, "status") == 0) {
		printf("[CLI] Server is running...\n");
	} 
	else if(strcmp(cmd, "quit") == 0) {
		printf("[CLI] quit command recv.!\n");
		running = 0;
                pthread_cond_broadcast(&queue_cond);

		return -1;
	} else {
		printf("[CLI] unknown command: %s\n", cmd);
	}

	return 0;
}

void * cli_worker(void * arg) {
	thread_id = gettid();

	thread_request_buffer_size = sizeof(char) * 8192;
	thread_request_buffer = malloc(thread_request_buffer_size);
	if(thread_request_buffer == NULL) {
		printf("thread %i could not allocate request buffer!\n", thread_id);
		return NULL;
	}
	memset(thread_request_buffer, 0, thread_request_buffer_size);

	thread_response_buffer_size = sizeof(char) * 8192;
	thread_response_buffer = malloc(thread_response_buffer_size);
	if(thread_response_buffer == NULL) {
		printf("thread %i could not allocate response buffer!\n", thread_id);
		return NULL;
	}
	memset(thread_response_buffer, 0, thread_response_buffer_size);

	printf("Thread CLI Worker Start: %i\n", thread_id);

	char * cli_input = thread_request_buffer;
	size_t cli_size = thread_request_buffer_size;
	int cli_running = 1; // should this be global or not?

	while(cli_running == 1) {

		// ----------
		pthread_mutex_lock(&queue_mutex);
		if(running == 0) {
			pthread_mutex_unlock(&queue_mutex);
			cli_running = 0;
			break;
		}
		pthread_mutex_unlock(&queue_mutex);
		// ----------

		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(STDIN_FILENO, &read_fds);

		struct timeval timeout = { 1, 0 }; // 1sec timeout...
	
		// works because we have not reassigned STDIN_FILENO in the program!
		int ret = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);

		if(ret > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
			if(fgets(cli_input, cli_size - 1, stdin)) {
				cli_input[strcspn(cli_input, "\n")] = 0;
				ret = handle_cli_command(cli_input, cli_size);
				if(ret == -1) {
					printf("[CLI] exit while loop!\n");
					break;
				}
			}
		}
	}

	if(thread_request_buffer != NULL) {
		free(thread_request_buffer);
		thread_request_buffer = NULL;
		thread_request_buffer_size = 0;
	}

	if(thread_response_buffer != NULL) {
		free(thread_response_buffer);
		thread_response_buffer = NULL;
		thread_response_buffer_size = 0;
	}

	printf("Thread CLI Worker End: %i\n", thread_id);
	return NULL;
}



// does bytes equals bytes during length?
// assumes that (a != NULL) and (b != NULL) and len != 0
//
// ret:
// 	1 on match
// 	0 on failure
int eq_bytes(char * a, char * b, size_t len) {
	if(a == NULL || b == NULL || len == 0) return 0;
	if(a == b) return 1;

	// great for SIMD 	;)
	for(size_t i = 0; i < len; i++) {
		if(a[i] != b[i]) {
			return 0;
		}
	}

	return 1;
}


int get_http_method_from_string(char * method_str) {
#if 1
	// ASSERT(method_str != NULL);
	if(method_str == NULL) return -1;

// TODO: move into code indexed mapped table
#define STRLEN(s) (sizeof(s) - 1)
#define SSTR(x) #x
#define MSTR(x) { SSTR(x), STRLEN(SSTR(x)), (int)(x) }

	struct _mstr_s { 
		char * str;
		size_t len;
		int value;
	};

	// i know the 9 is known somewhere ;)
	// store in .rodata
	static const
	struct _mstr_s _mstr[9] = {
		MSTR(GET),
		MSTR(POST),
		MSTR(PUT),
		MSTR(DELETE),
		MSTR(PATCH),
		MSTR(HEAD),
		MSTR(OPTIONS),
		MSTR(TRACE),
		MSTR(CONNECT)
		// { "GET", 3, GET },
	};

	int index = 0;
	while(index < 9) {
		if(memcmp(method_str, _mstr[index].str, _mstr[index].len) == 0) {
			return _mstr[index].value;
		}
		index++;
	}

	return -1;
#else
	if(method_str == NULL) return -1;
	int len = strlen(method_str);
	// wildy not good
	// why not generate a tree and walk it by chars from method_str?
	// 	could do a per bit check with simd.
	// 	there are only so many options that might match.
	// 	as soon as it misses, fail it.
	if(strncmp(method_str, "GET", 3) == 0) return GET;
	if(strncmp(method_str, "POST", 4) == 0) return POST;
	if(strncmp(method_str, "PUT", 3) == 0) return PUT;
	if(strncmp(method_str, "DELETE", 6) == 0) return DELETE;
	if(strncmp(method_str, "PATCH", 5) == 0) return PATCH;
	if(strncmp(method_str, "HEAD", 4) == 0) return HEAD;
	if(strncmp(method_str, "OPTIONS", 7) == 0) return OPTIONS;
	if(strncmp(method_str, "TRACE", 5) == 0) return TRACE;
	if(strncmp(method_str, "CONNECT", 7) == 0) return CONNECT;

	return -1;
#endif
}

// are we using this?
char * get_http_method_string(int http_method) {
	char * string = NULL;
	switch(http_method) {
		case POST:
		string = "POST";
		break;
		case GET:
		string = "GET";
		break;
		case PUT:
		string = "PUT";
		break;
		case DELETE:
		string = "DELETE";
		break;
		case PATCH:
		string = "PATCH";
		break;
		case HEAD:
		string = "HEAD";
		break;
		case OPTIONS:
		string = "OPTIONS";
		break;
		case TRACE:
		string = "TRACE";
		break;
		case CONNECT:
		string = "CONNECT";
		break;
	}	

	return string;
}

size_t get_http_method_string_size(int http_method) {
	size_t size;
	switch(http_method) {
		case POST:
		size = 4;
		break;
		case GET:
		size = 3;
		break;
		case PUT:
		size = 3;
		break;
		case DELETE:
		size = 6;
		break;
		case PATCH:
		size = 5;
		break;
		case HEAD:
		size = 4;
		break;
		case OPTIONS:
		size = 7;
		break;
		case TRACE:
		size = 5;
		break;
		case CONNECT:
		size = 7;
		break;
		default:
		size = 0;
		break;
	}	

	return size;
}

char * get_status_code_str(int status_code) {
	// * https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status
 	// * https://httpwg.org/specs/rfc9110.html#overview.of.status.codes	
	switch(status_code) {
		case 100: return "Continue";
		case 101: return "Switching Protocols";
		case 102: return "Processing";
		case 103: return "Early Hints";

		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No content";
		case 205: return "Reset Content";
		case 206: return "Partial Content";
		case 207: return "Multi-Status";
		case 208: return "Already Reported";
		case 226: return "IM Used";

		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 305: return "Use Proxy";
		case 306: return "Unused";
		case 307: return "Temporary Redirect";
		case 308: return "Permanent Redirect";
	
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 407: return "Proxy Authentication Required";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 412: return "Precondition Failed";
		case 413: return "Content Too Large";
		case 414: return "URI Too Large";
		case 416: return "Range Not Satisfiable";
		case 417: return "Expectation Failed";
		case 418: return "I'm a teapot";
		case 421: return "Misdirected Request";
		case 422: return "Unrpocessable Content";
		case 423: return "Locked";
		case 424: return "Failed Dependency";
		case 425: return "Too Early";
		case 428: return "Precondition Required";
		case 429: return "Too Many Requests";
		case 431: return "Request Header Fields Too Large";
		case 451: return "Unavailable For Legal Reasons";

		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";
		case 506: return "Variant Also Negotiates";
		case 507: return "Insufficient Storage";
		case 508: return "Loop Detected";
		case 510: return "Not Exetended";
		case 511: return "Network Authentication Required";
	}

	return "Unknown";
}


struct mime_type_s {
	size_t ext_size;
	const char * ext;
	const char * mime;
};
typedef struct mime_type_s mime_type_t;

#define MIME_TYPE(ext, mime) (mime_type_t){sizeof(ext), ext, mime}

// make htable?
const mime_type_t mime_type_table[] = {
	MIME_TYPE("html", "text/html"),
	MIME_TYPE("htm", "text/html"),
	MIME_TYPE("js", "text/javascript"),
	MIME_TYPE("css", "text/css"),
	MIME_TYPE("json", "application/json"),
	MIME_TYPE("jpg", "image/jpeg"),
	MIME_TYPE("jpeg", "image/jpeg"),
	MIME_TYPE("png", "image/png"),
	MIME_TYPE("svg", "image/svg+xml"),
	MIME_TYPE("webp", "image/webp"),
	MIME_TYPE("ico", "image/x-icon"), // favicon
	MIME_TYPE("gif", "image/gif"),
	MIME_TYPE("txt", "text/plain"),
	MIME_TYPE("wav", "audio/wav"),
	MIME_TYPE("ogg", "audio/ogg"),	// video/ogg
	MIME_TYPE("mp3", "audio/mpeg"),
	MIME_TYPE("mp4", "video/mp4"),
	MIME_TYPE("webm", "video/webm"),
	MIME_TYPE("md", "text/markdown"),
	MIME_TYPE("zip", "application/zip"),
	MIME_TYPE("pdf", "application/pdf"),
	MIME_TYPE("xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetxml.sheet"), // wow...
};

// htable would be faster
char * get_mime_type(char * ext) {
	if(ext == NULL) {
		return "application/octect-stream";
	}

	size_t mime_table_size = sizeof(mime_type_table) / sizeof(mime_type_table[0]);
	for(size_t i = 0; i < mime_table_size; i++) {
		if(strncmp(ext, mime_type_table[i].ext, mime_type_table[i].ext_size) == 0) {
			return (char*)mime_type_table[i].mime;
		}
	}

	return NULL;// "application/octect-stream";
}

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
		printf("\n ### INTERRUPT ###\n");

		(void)sig;
		running = 0;
		pthread_cond_broadcast(&queue_cond); // wake all threads!

		// cleanup_and_exit(1);
	}
}

// cookie data = [0-9a-fA-F] only
/*
int parse_cookie(char * request, size_t request_size, 
		char * name, char * data, size_t * data_size) {
	1. find "Cookie:"
	2. find name
	3. skip whitespace

	size_t parsed_len = 0;
	4. parse above chars until CRLF or ';' or whitespace or '\0'
	5. write back buffer into data
	// memcpy?
	6. write back size of data
	*data_size = parsed_len;

	return 1;
 }
*/

/* Internal Routing
ROOT_DIR = "./www"
ROOT_DIR + "/html/*.html"
ROOT_DIR + "/css/*.css"
ROOT_DIR + "/js/*.js"
ROOT_DIR + "/img/*.jpeg"
ROOT_DIR + "/img/*.png"
ROOT_DIR + "/img/*.svg"
*/

/*
 * note: we dont use dprintf or write since they can end up causing a PIPE error.
 * we dont want that, so well just use send() with MSG_NOSIGNAL instead.
 * - test: hold F5 (refresh) in the browser window to quick reload many multiple times.
*/

// lets try a custom dprintf using send instead
int hprintf(int socket, const char * fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(thread_response_buffer, thread_response_buffer_size, fmt, args);
	va_end(args);

	if(len < 0) {
		return -1;
	}

	// shouldnt happen
	if((size_t)len > thread_response_buffer_size) {
		len = thread_response_buffer_size;
	}

	int sent = send(socket, thread_response_buffer, len, MSG_NOSIGNAL);
	return sent;
}

void http_send_200(int socket) {
	hprintf(socket, "HTTP/1.1 200 OK\r\n\r\n");
}

void http_send_302(int socket, char * redirect_location) {
	hprintf(socket, "HTTP/1.1, 302 Found\r\n");
	hprintf(socket, "Location: %s\r\n", redirect_location);
	hprintf(socket, "Content-Length: 0\r\nConnection: close\r\n\r\n");
}

void http_send_401(int socket) {
	hprintf(socket, "HTTP/1.1 401 Bad Request\r\n\r\n");
}

void http_send_404(int socket) {
	hprintf(socket, "HTTP/1.1 404 File Not Found\r\n\r\n");
}

void http_send_500(int socket) {
	hprintf(socket, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
}

// remove response plz - get from code
void http_send(int socket, int status_code, char * mime_type, uint8_t * data, size_t data_size) {
	hprintf(socket, "HTTP/1.1 %d %s\r\n", status_code, get_status_code_str(status_code));
	hprintf(socket, "Content-Type: %s\r\n", mime_type);
	hprintf(socket, "Content-Length: %zu\r\n", data_size);
	hprintf(socket, "\r\n");
	
	if(data != NULL) {
		send(socket, data, data_size, MSG_NOSIGNAL);
	}
}

void http_send_wcookie(int socket, int status_code, char * cookie_str, char * mime_type, uint8_t * data, size_t data_size) {
	hprintf(socket, "HTTP/1.1 %d %s\r\n", status_code, get_status_code_str(status_code));
	if(mime_type != NULL)
		hprintf(socket, "Content-Type: %s\r\n", mime_type);
	if(data != NULL && data_size != 0)
		hprintf(socket, "Content-Length: %zu\r\n", data_size);

	if(cookie_str != NULL) {
		hprintf(socket, "Set-Cookie: %s\r\n", cookie_str);
		/*
		hprintf(socket, "Set-Cookie: %s\r\n", cookie_str);
		hprintf(socket, "Path=/; HttpOnly; SameSite=Strict; Secure;\r\n");
		*/
	}

	hprintf(socket, "\r\n");

	if(data != NULL) {
		send(socket, data, data_size, MSG_NOSIGNAL);
	}
}

// dont like this function
// hash cache or tree?
// send_static_file(socket, path)
// 	if in ram, mime is known and data is too - hashtable by path
//	if on disk, load file into memory then send
void load_and_send_file(int socket, char * mime_type, char * path) {
/*
	unsigned int index = -1;
	if((index = cache_has(path)) >= 0) {
		cache_index_send(socket, index);
		return;
	}
*/
	int fd = open(path, O_RDONLY);
	if(fd < 0) {
		// if mime_type == html -> send page_404.html instead ;)
		http_send_404(socket);
		return;
	}
/*
	if((index = cache_load(path)) >= 0) {
		cache_index_send(socket, index);
		return;
	}
*/
	// move to cache_load_file()
	struct stat st;
	if(fstat(fd, &st) == -1) {
		close(fd);
		http_send_500(socket);
		return;
	}

	http_send(socket, 200, mime_type, NULL, st.st_size);
	
	// todo: use thread_buffer
	char buffer[1024];
	ssize_t n;
	while((n = read(fd, buffer, sizeof(buffer))) > 0) {
		// write(socket, buffer, n);
		send(socket, buffer, n, MSG_NOSIGNAL);
	}

	close(fd);
}

// maybe cpy it out?
char * get_ext(char * str) {
	char * final_period = NULL;

	while(*str != '\0') {
		if(*str == '.') {
			final_period = str;
		}
		str++;
	}

	return final_period;	
}

// global
login_ht_t login_ht;

void init_login_ht() {
	memset(&login_ht, 0, sizeof(login_ht));

	int max_planned_entries = 128; // lowball
	login_ht.capacity = max_planned_entries * 4;
	login_ht.size = 0;

	size_t size = sizeof(login_entry_t) * login_ht.capacity;
	// login_ht.entry = aligned_alloc(alignof(login_entry_t), size);
	void * ptr = malloc(size);
	if(ptr == NULL) { 
		//exit(1); 
	}

	login_ht.entry = (login_entry_t *)ptr;

	if(login_ht.entry == NULL) {
//		exit(1);
	}

	memset(login_ht.entry, 0, size);
}

void free_login_ht() {
	if(login_ht.entry != NULL) {
		free(login_ht.entry);
	}
	memset(&login_ht, 0, sizeof(login_ht));
}

// could have just used a linear array and loop...
// use unsigned long?
// why do we even use a hash - we should already have sha256 as the fucking hash!
unsigned long hash_djb2(unsigned char * data, size_t bytes) {
	unsigned long hash = 5381;
	int c;

	unsigned int i = 0;
	while(i < bytes) {
		c = data[i];
		hash = ((hash << 5) + hash) + c;
		i++;
	}
	return hash;
}

// ht[hash % size]; return index;
int login_ht_insert_hash(login_ht_t * ht, unsigned long hash) {
	if(ht == NULL) return -1;

	int index = hash % ht->capacity;
	
	if(ht->entry[index].hash == 0 || ht->entry[index].hash == hash) {
		ht->entry[index].hash = hash;
		return index;
	}

	// find next empty index - incremental search
	int start = index;
	int end = ht->capacity - 1;
	while(index < end) {
		index++;
		if(ht->entry[index].hash == 0 || ht->entry[index].hash == hash) {
			ht->entry[index].hash = hash;
			return index;
		}
	}

	index = 0;
	end = start;
	while(index < end) {
		if(ht->entry[index].hash == 0 || ht->entry[index].hash == hash) {
			ht->entry[index].hash = hash;
			return index;
		}
		index++;
	}
	
	return -1;
}

// find
int login_ht_get_hash_index(login_ht_t * ht, unsigned long hash) {
	if(ht == NULL) return -1;

	int index = hash % ht->capacity;
	
	if(ht->entry[index].hash == hash) {
		return index;
	}

	// find next empty index - incremental search
	int start = index;
	int end = ht->capacity - 1;
	while(index < end) {
		index++;
		if(ht->entry[index].hash == hash) {
			return index;
		}
	}

	index = 0;
	end = start;
	while(index < end) {
		if(ht->entry[index].hash == hash) {
			return index;
		}
		index++;
	}
	
	return -1;
}

int login_ht_clear_hash_index(login_ht_t * ht, unsigned long hash) {
	if(ht == NULL) return -1;

	int index = login_ht_get_hash_index(ht, hash);
	if(index >= 0) {
		memset(&ht->entry[index], 0, sizeof(login_entry_t));
		return 0;
	}

	return 1; // err
}

int login_ht_clear_index(login_ht_t * ht, int index) {
	if(ht == NULL || index < 0) return 1;

	login_entry_t * entry = &ht->entry[index];
	memset(entry, 0, sizeof(login_entry_t));

	return 0;
}

#if 0
void ht_test() {
	printf("test login ht\n");
	
	init_login_ht();

	printf("insert 'a'\n");
	char * str_a = "aaaaaa";
	unsigned long hash_a = hash_djb2(str_a, strlen(str_a));
	
	char * str_b = "aaaaab";
	unsigned long hash_b = hash_djb2(str_b, strlen(str_b));

	printf("hash_a: %8lX - %lu\n", hash_a, hash_a % login_ht.capacity);
	printf("hash_b: %8lX - %lu\n", hash_b, hash_b % login_ht.capacity);

	int idx_a = login_ht_get_hash_index(&login_ht, hash_a);
	if(idx_a >= 0) {
		login_ht.entry[idx_a].hash = hash_a;
		login_ht.entry[idx_a].ok = 1;
	}

	free_login_ht();
}
#endif

// per thread
// ...

#if 0
user_t get_db_user_from_username(char * username) {
	user_t user = { 0 }; // sqlite3.find_user(username);

	return user;
}

int is_valid_session(unsigned char token[32]) {
	unsigned long hash = hash_djb2(token, 32);
	int idx = login_ht_get_hash_index(&login_ht, hash);

	if(idx < 0) { return 0; }

	if(memcmp(login_ht.entry[idx].token, token, 32) == 0) {
		return 1;
	}

	return 0;
}
#endif

const int num_users = 3;
const char * users[3] = {
	"admin",
	"cat",
	"dog"
};

int handle_logout(char token[TOKEN_SIZE]) {
	printf("attempt logout!\n");

	// arena_clear(&thread_json_arena);

	pthread_mutex_lock(&login_mutex);

        // check if logged in - move into handle requests and block access / redirect if not allowed
        unsigned long hash = hash_djb2(token, TOKEN_SIZE);
        int index = login_ht_get_hash_index(&login_ht, hash);
        if(index < 0) {
                pthread_mutex_unlock(&login_mutex);
                return 0;
        }

	// login_ht_clear_hash_index(&login_ht, hash);
	int rval = login_ht_clear_index(&login_ht, index);

        pthread_mutex_unlock(&login_mutex);

	return rval;
}

void handle_login(int socket, char token[TOKEN_SIZE], char * json_data) {
	arena_clear(&thread_json_arena);

	// TODO: move this json parse into the handle_api_request instead
	printf("# json data:\n%s\n", json_data);

	json_result_t result = json_parse(&thread_json_arena, json_data);

	json_value_t * username_value = json_find_by_path(result.root, "username");
        if(username_value && username_value->type == JSON_TOKEN_STRING) {
                printf("username := %s\n", username_value->string.chars);
        } else {
                printf("username := NULL\n");
        }

	json_value_t * password_value = json_find_by_path(result.root, "password");
        if(password_value && password_value->type == JSON_TOKEN_STRING) {
                printf("password := %s\n", password_value->string.chars);
        } else {
                printf("password := NULL\n");
        }

#if 1
	// TODO: replace with actual user matching on sqlite3
	int match_id = -1;
	int username_value_len = strlen(username_value->string.chars);
	for(int i = 0; i < num_users; i++) {
		// issue: strncmp okeys "dogcat" for "dog" ...
		// those are not the same things!!!
		int len = strlen(users[i]);
		if(username_value_len == len) {
			if(strncmp(username_value->string.chars, users[i], len) == 0) {
				printf("matched string: %s\n", users[i]);
				match_id = i;
				break;
			}
		}
	}

	if(match_id == -1) {
		printf("Login: username was not found!\n");
		// TODO: on fail, send back json data too
		http_send_401(socket);
		return;
	}
#endif
	// assume that token is 0
	// generate and insert new one!
	// then send it over as cookie

	size_t sha_input_len = 0;
	char sha_input[128] = { 0 }; // "lorem ipsum dasum scasf 1213 sfaf" };
	memcpy(sha_input, username_value->string.chars, strlen(username_value->string.chars));
	sha_input_len = strlen(sha_input);

	// TODO: use thread local version
	SHA256_CTX sha_ctx;
	sha256_init(&sha_ctx);
	sha256_update(&sha_ctx, (const uint8_t *)sha_input, sha_input_len);
	sha256_final(&sha_ctx, (uint8_t *)token);

	printf("lock login\n");
	pthread_mutex_lock(&login_mutex);

	unsigned long hash = 0;
	int index = 0;

	hash = hash_djb2(token, TOKEN_SIZE);
	printf("# hash = %08lX\n", hash);
	
	index = login_ht_get_hash_index(&login_ht, hash);
	if(index == -1) {
		printf("hash not in hashtable\n");
		index = login_ht_insert_hash(&login_ht, hash);
		printf("try %i\n", index);
	}

	if(index >= 0) {
		login_ht.entry[index].hash = hash;
		memcpy(login_ht.entry[index].token, token, TOKEN_SIZE);

		user_t user = { 0 };
		memcpy(user.username, 
			username_value->string.chars, strlen(username_value->string.chars));
		memcpy(user.password_hash, 
			password_value->string.chars, strlen(password_value->string.chars));
		login_ht.entry[index].user = user;

		printf("# login_ht index == %i\n", index);
	}

	printf("unlock login\n");
	pthread_mutex_unlock(&login_mutex);

	printf("build user sessionID\n");
	char cookie[512] = "sessionID=";
	
	char token_hex[128] = { 0 };
	hex_encode(token_hex, token, TOKEN_SIZE);

	size_t hex_len = strlen(token_hex);
	// memcpy(cookie + 10, token_hex, hex_len);
	// cookie[10 + hex_len] = '\0';
	
	snprintf(cookie, 512 - 1,
		"sessionID=%.64s; Path=/; HttpOnly; SameSite=Strict; Secure",
		token_hex);

	// build json for sending
	// json -> redirect_location = "/mypage.html"

	char * json_out_data = "{\"redirect_location\":\"/mypage.html\"}";

	printf("send wcookie\n");
	// "sessionID=XYZ; Path=/; HttpOnly; SameSite=Strict; Secure;"
	http_send_wcookie(socket, 200, cookie, "application/json", json_out_data, strlen(json_out_data));
	printf("handled login!\n");
}

int extract_sessionid_token(char * buffer, unsigned char * token) {
	char * cookie_header = strstr(buffer, "Cookie:");
	if(cookie_header == NULL) return 0;

	cookie_header += strlen("Cookie:");

	char * session_start = strstr(cookie_header, "sessionID=");
	if(session_start == NULL) return 0;

	session_start += strlen("sessionID=");

	// get the token
	// wont work for non-; ended strings --- fuck
	char * session_end = strpbrk(session_start, "; \r\n");
	if(session_end == NULL) {
		session_end = strpbrk(session_start, "\r\n"); // second attempt
	}

	size_t len = session_end ? (size_t)(session_end - session_start) : strlen(session_start);
	printf("req cookie len: %li\n", len);
	if(len == 0) {
		return 0;
	}

	// grab the shit
	char hex[128] = { 0 };
	for(size_t i = 0; i < len; i++) {
		hex[i] = session_start[i];
	}
#if 1
	int l1 = hex_decode(token, TOKEN_SIZE, hex);
	printf("hex decoded bytes: %i\n", l1);
	if(l1 == -1) {
		printf("HEX DECODE ERROR\n");
	}
#endif

	return 1;
}

int is_logged_in(unsigned char * token) {
	if(token == NULL) return 0;

	pthread_mutex_lock(&login_mutex);

        // check if logged in - move into handle requests and block access / redirect if not allowed
        unsigned long hash = hash_djb2(token, TOKEN_SIZE);
        int index = login_ht_get_hash_index(&login_ht, hash);
        if(index < 0) {
                pthread_mutex_unlock(&login_mutex);
                return 0;
        }

	pthread_mutex_unlock(&login_mutex);

	return 1;
}

login_entry_t * get_session_ptr(unsigned char * token, unsigned long hash) {
	if(hash == 0) {
		if(token == NULL) {
			return NULL;
		}
		hash = hash_djb2(token, TOKEN_SIZE);
	}
	
	pthread_mutex_lock(&login_mutex);

	int index = login_ht_get_hash_index(&login_ht, hash);
	if(index < 0) {
		pthread_mutex_unlock(&login_mutex);
		return NULL;
	}

	login_entry_t * ptr = &login_ht.entry[index];

	pthread_mutex_unlock(&login_mutex);

	return ptr;
}

// thread_response_buffer, thread_request_buffer
//void handle_api_request(int socket, int method, char * req_path, int has_cookie, unsigned char * token, char * buffer, char * json_in_data) {
void handle_api_request(http_request_t * req) {
	printf(" ### handle api request X ###\n");

	// should already have been done!!!!
	// figure out which action/ route to take
	int api_version = 0;

	// bad alias
	char * in_buf = thread_request_buffer;
	char * out_buf = thread_response_buffer;
	char * json_in_data = (thread_request_buffer + req->request_size) - req->content_size;

	if(json_in_data == NULL) {
		http_send_401(req->socket);
		close(req->socket);
		return;
	}

	// int has_cookie = 0;
	unsigned char token_hex[128] = { 0 };
	// unsigned char token[64] = { 0 };

	// grab the sessionID data from the request buffer - not safe
	// cookie = "sessionID="

	#if 0	
	//has_cookie = extract_sessionid_token(buffer, token);
	if(req->has_cookie) {
		printf("Token: --");
		for(int i = 0; i < TOKEN_SIZE; i++) {
			printf("%x", req->token[i]);
		}
		printf("--\n");		
		printf("\n");
	}
	#endif
	
	if(strncmp(req->path, "/api/v1/login", 13) == 0) {
		printf("API V1 LOGIN\n");
		handle_login(req->socket, req->token, json_in_data);
		return;
	} /* else {
		if(!has_cookie) {
			// user is not trying to login
			// forcibly redirect to login.html
			http_send_302(socket, "/login.html");
		}
	} */

	printf("lock login 2\n");
	pthread_mutex_lock(&login_mutex);

	// check if logged in - move into handle requests and block access / redirect if not allowed
	unsigned long hash = hash_djb2(req->token, TOKEN_SIZE);
	int index = login_ht_get_hash_index(&login_ht, hash);
	if(index < 0) {
		printf("unlock login 2 (a)\n");
		pthread_mutex_unlock(&login_mutex);

		http_send_401(req->socket);
		return;
	}

	if(memcmp(login_ht.entry[index].token, req->token, TOKEN_SIZE) == 0) {
		char * username = login_ht.entry[index].user.username;
		printf("user '%s' is logged in, cont. handling request!\n", username);
	} else {		
		printf("unlock login 2 (b)\n");
		pthread_mutex_unlock(&login_mutex);

		http_send_401(req->socket);
		return;
	}

	printf("unlock login 2\n");
	pthread_mutex_unlock(&login_mutex);

	printf(" ### handle actual api request ###\n");

	// handle actual requests
	login_entry_t * session_ptr = &login_ht.entry[index];

	// to parse request by json
	// arena_clear(&thread_json_arena); 

	// TODO: (BUG)
	// when i start by accessing MYPAGE and the cookie still exists - it breaks
	// then i cant login using the login page
	// and a thread seemingly gets hung
	//
	// it seems like the mutex is still locked but i dont know...

	printf("########## method: %s\n", req->method);
	int http_method = get_http_method_from_string(req->method);
	switch(http_method) {
		// should never be used btw - js disallows it. use POST instead
		case GET:
			printf(" > API: GET\n");
		break;
		case PUT:
			printf(" > API: PUT\n");
		break;
		case POST:
			printf(" > API: POST\n");

		// {"get":["username"]}
		printf("API get stuff from user with JSON\n");

		json_result_t result = json_parse(&thread_json_arena, json_in_data);

		if(strcmp(req->path, "/api/v1/logout") == 0) {
			printf(" > api logout\n");

			int rval = handle_logout(req->token);
			if(rval) {
				http_send_401(req->socket);
				return;
			} else {
				char * del_cookie = "sessionID=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/; HttpOnly; SameSite=Strict; Secure";
				size_t del_len = strlen(del_cookie);
				// http_wcookie(socket, 200, deleted_cookie, 0, NULL, del_len);
				http_send_wcookie(req->socket, 200, del_cookie, NULL, NULL, 0);
			}
		}

		if(strcmp(req->path, "/api/v1/user") == 0) {
			printf(" > attempt to bullshit\n");

			json_value_t * to_get = json_find_by_path(result.root, "get");

			if(to_get->type == JSON_TOKEN_STRING) {
				// likely only one
				printf("get %s\n", to_get->string.chars);

				if(strcmp(to_get->string.chars, "username") == 0) {
					// build json response or something
					char * username = session_ptr->user.username;

					json_value_t * obj = json_make_object(&thread_json_arena);
			        	json_object_add(&thread_json_arena, obj, 
						"username", 
						json_make_string(&thread_json_arena, username));
			
				        json_result_t result = {
				                .root = obj,
				                .err = NULL
        				};

				        // json_dump(result);
					char * output = NULL;
					size_t output_size = 0;
					output = json_write(&thread_json_arena, result, &output_size);
					
					http_send(req->socket, 200, "application/json", (uint8_t*)output, output_size);
					// close(socket);
					return;
				}
			}
			else if(to_get->type == JSON_TOKEN_ARRAY) {
				// possible several
				int count = to_get->array.count;
				for(int i = 0; i < count; i++) {
					json_value_t * val;
					val = to_get->array.items[i];
					printf("get %s\n", val->string.chars);
				}
			}

		}
	}

	#if 0
	if (request == "api/v1/login") {
		user = { 0 }; // sqlite.get_user_on_match(username, hashsalt(password));
		if(user.valid) {
			generate sha256 token
			login_ht_insert(user, token);
			respond 200 ok + sessionID cookie (token)
		} else {
			respond 401 unauth
		}
	}
	else if(have_sessionID_cookie) {
		// other requests
		hash = hash_djb2(token);
		idx = login_ht_get_index(hash);
		if(login_ht.entry[idx].token != token) {
			respond 401 unauth
		}

		// if(!is_valid_session(token)) {
		//	respond_401(socket);
		//	close(socket);
		// }

		// handle request json
	}
	#endif

	http_send_200(req->socket);
#if 0
	printf("HELP\n");

	char * json_data = "{\"status\":\"ok\",\"data\":{\"uid\":\"00112233\",\"img\":\"/img/cat_black.jpg\"}}";
	size_t json_len = strlen(json_data);

	// http_send(socket, 200, "OK", "application/json", (uint8_t*)json_data, json_len);
	char * cookie = "session_id=admin123";

	http_send_wcookie(socket, 200, cookie, "application/json", (uint8_t*)json_data, json_len);
#endif	
	// close(socket);

	return;
}

// file request - standard OK for all
void handle_request(http_request_t * req) {
/*
TODO:
	remake the handle_request function
	so that the website actually works
	then also update the handle_api_request func
	
TODO: should we add keep-alive sockets?
	needs a timeout
	and a maximum requests 


> done before this function
1. 	read and parse request
1.2. 	check if IP is blacklisted or not

> done before this function
2.	parse cookie and session info
	find the user and fetch it / ptr



3.	check path for public / protected resource
3.2.	check for per-user permissions (admin)

4. 	handle api endpoints

5. 	serve static files or app content

*/
	printf("handle request\n");

	// todo: assert before entering the handle_client call
	if(socket <= 0) {
		printf("client socket error!\n");
		return;
	}

#if 0
	printf("AAA\n");
	// check if logged in here
	char session_token[TOKEN_SIZE] = { 0 };
	int auth = 0;
	login_entry_t * session_ptr = NULL;

	printf("BBB\n");
	if(!extract_sessionid_token(buffer, session_token)) {
		http_send_401(socket);
		close(socket);
		return;
	}

	printf("CCC\n");
	auth = is_logged_in(session_token);
	if(auth) {
		session_ptr = get_session_ptr(session_token, 0);
	}

	printf("DDD\n");
#endif

	// cookie is really irrevelant
	// question is wheter or not that matches a login token
	// has_cookie = extract_sessionid_token(buffer, token);
	if(req->has_cookie) {
		printf("Token: --");
		for(int i = 0; i < TOKEN_SIZE; i++) {
			printf("%x", req->token[i]);
		}
		printf("--\n");		
		printf("\n");
	} else {
		printf("Token: --(null)---\n");
		// no cookie
		// check with whitelist here

#if 0
		// shitty non-working gate
		if(strncmp(req->path, "/login.html", 11) != 0) {
		// if(strncmp(path, "/api/v1/login", 13 != 0)) {
			http_send_302(req->socket, "/login.html");
		}
#endif
	}
	
#if 0
	if(strncmp(req_path, "/api/v1/login", 13) == 0) {
		printf("API V1 LOGIN\n");
		handle_login(socket, token, json_in_data);
		return;
	} else {
		if(!has_cookie) {
			// user is not trying to login
			// forcibly redirect to login.html
			http_send_302(socket, "/login.html");
		}
	}
#endif

	char * buffer = thread_request_buffer;
	
	// state
	int is_api_request_ = (strncmp(req->path, "/api", 4) == 0); // use strncmp or a known cmp
	#if 0
	int http_method = get_http_method_from_string(method);
	if(http_method == -1) {
		http_send_401(socket);
		close(socket);
		return;
	}
	#endif

	if(is_api_request_) {
		printf("# is api request == true\n");
#if 0

		size_t content_length = 0;

		char * p = buffer; // buffer
		char * content_length_header = strstr(p, "Content-Length:");
		p = content_length_header + strlen("Content-Length:");
	
		int len = atoi(p);
		printf("# Content-Length: %i\n", len);
		if(len > 0) {
			content_length = len;
		}

		// if its 0, then we might still want to handle the actual request
		// since not all of them takes json data or other data
		#if 0
		if(len == 0) {
			// something is probably wrong?
			http_send_401(socket);
			close(socket);
			return;
		}
		#endif

		// call api handler with method
		char * content_ptr = (thread_request_buffer + req->request_size) - req->content_size;
#endif
		// handle_api_request(req->socket, 0/*http_method*/, path, has_cookie, token, buffer, content_ptr);
		handle_api_request(req);
		close(req->socket);
		return;
	}

	if(strcmp(req->method, "GET") == 0 && strcmp(req->path, "/favicon.ico") == 0) {
		printf("SEND FAVICON!\n");
		http_send(req->socket, 200, "image/x-icon", (uint8_t*)favicon_icox, sizeof(favicon_icox));
		printf("FAVICON SENT!\n");
		close(req->socket);
		return;
	}

	// valgrind says that ram has issues right now, ill check later
#if 0
	// valgrind complains if we dont copy the data over first
	// says uninitialied value
	char catpath[512] = { 0 };
	if(cat_large_img_path != NULL) {
	memcpy(catpath, cat_large_img_path, strlen(cat_large_img_path));

	// if(strcmp(method, "GET") == 0 && strcmp(path, cat_large_img_path) == 0) {
	if(strcmp(method, "GET") == 0 && strcmp(path, catpath) == 0) {
		// send from ram :)
		http_send(socket, 200, "image/jpeg", (uint8_t*)cat_large_img_data, cat_large_img_data_size);
                printf("CAT RAM SENT!\n");
                close(socket);

                return;
	}
	}
#endif

// we do a lot of strcmp & strncmp
// we also know that many of them are of a certain size (knwon)
// PUT -> P U T
// #define SPLIT_PUT	'P', 'U', 'T'
// then we can use more specific cmp
// #define STRCMP3(a, b) (a[0] == b[0] && a[1] == b[1] && a[2] == b[2])
// #define STRCMP3(s, c0, c1, c2) (s[0] == c0 && s[1] == c1 && s[2] == c2)
//	maybe using sub4 bytes is not great... add a ' ' to the c3? (without checking it)
//
// we also need to sanitize the method
// if((ch < 'A' || ch > 'Z') && ch != '_' && ch != '-') { invalid!!! }
//
// maybe caching the filepath is not what we want.
// it might be better to cache the actual http request instead?
// 
// maybe we can put in async io (posic aio.h) for stream reading files

	// todo: make a testing platform for the api stuff
	// 	build the http requests and test all apis
	// routing plz
	// if(strncmp(path, "/api", 4) == 0) {
	//	handle_api_request();
	// }

#if 0
	if(strcmp(method, "PUT") == 0 && strncmp(path, "/api", 4) == 0) {
		printf("API CALL\n");
		// API CALL maybe
		// todo: build with json library
		char * json_data = "{\"status\":\"ok\",\"data\":{\"uid\":\"00112233\",\"img\":\"/img/cat_black.jpg\"}}";
		size_t json_len = strlen(json_data);

		// http_send(socket, 200, "OK", "application/json", (uint8_t*)json_data, json_len);
		char * cookie = "session_id=admin123";

		http_send_wcookie(socket, 200, cookie, "application/json", (uint8_t*)json_data, json_len);
		close(socket);
		return;
	}
#endif

	char * ext = NULL;
	ext = get_ext(req->path);
	if(ext == NULL) {
		printf("API CALL\n");
		// maybe its an api call...
		// check that or fail
		http_send_401(req->socket);
		close(req->socket);
		return;
	}

	ext++;
	char * mime_type = get_mime_type(ext);
	printf("MIME: %s -> %s\n", ext, mime_type);
	if(mime_type == NULL) {
		http_send_404(req->socket);
		close(req->socket);
		return;
	}

/*
ROOT_DIR = "./www"
ROOT_DIR + "/html/*.html"
ROOT_DIR + "/css/*.css"
ROOT_DIR + "/js/*.js"
ROOT_DIR + "/img/*.jpeg"
ROOT_DIR + "/img/*.png"
ROOT_DIR + "/img/*.svg"
*/

	char * _root_dir = "www";
	char * _sub_dir = NULL;
	char real_path[1024];
	if(strcmp(mime_type, "text/html") == 0) {
		_sub_dir = "html";
	}
	/*
	else if(strcmp(mime_type, "text/javascript") == 0) {
		_sub_dir = "js";
	}
	else if(strcmp(mime_type, "text/css") == 0) {
		_sub_dir = "css";
	}
	else if(strncmp(mime_type, "image/", 6) == 0) {
		_sub_dir = "img";
	} else {
		send_response_404(socket);
		close(socket);
		return;
	}
	*/
	if(_sub_dir != NULL) {
		snprintf(real_path, sizeof(real_path) - 1, 
			"%s/%s/%s", 
			_root_dir,
			_sub_dir,
			req->path + 1);
	} else {
		snprintf(real_path, sizeof(real_path) - 1, 
			"%s/%s", 
			_root_dir,
			req->path + 1);

	}

/*
	if(_root_dir == NULL || _sub_dir == NULL || real_path[0] == 0) {
		printf("ERROR\n");
		send_response_404(socket);
		close(socket);
		return;
	}
*/

	load_and_send_file(req->socket, mime_type, real_path);	
	
#if 0
	// response
	char * response = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Connection: close\r\n"
		"\r\n"
		"<!DOCTYPE html>"
		"<html><head><title>JATS - C HTTP Server</title></head>"
		"<body><h1>Welcome to JATS<h1></br><p>Jensen Asset Tracking System<p></body>"
		"</html>";

	write(socket, response, strlen(response)); 
#endif
	close(req->socket);
}

void db_test(void) {
	printf("DB TEST BEGIN\n");

	// DB_PATH_STR
	int rc;
	sqlite3 * db;
	sqlite3_stmt * stmt;
	char cmd_buf[1024] = { 0 };

	// 1|john_doe|hashed_password|john.doe@example.com|John|Doe
	// 2|gabriel_carlsson|hashed_password|gabriel-carlsson@hotmail.se|Gabriel|Carlsson
	snprintf(cmd_buf, 1024 - 1, "SELECT * FROM users;");

	rc = sqlite3_open(DB_PATH_STR, &db);

	rc = sqlite3_prepare_v2(db, cmd_buf, -1, &stmt, NULL);
	if(rc != SQLITE_OK) {
		printf("prepare call error: %i\n", rc);
		sqlite3_close(db);
		return;
	}

	int i = 1;
	while(sqlite3_step(stmt) == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		// const char * id_zptr = sqlite3_column_text(stmt, 0);
		const char * username_ptr = (const char *)sqlite3_column_text(stmt, 1);
		const char * hash_pw_ptr = (const char *)sqlite3_column_text(stmt, 2);
		const char * email_ptr = (const char *)sqlite3_column_text(stmt, 3);
		const char * firstname_ptr = (const char *)sqlite3_column_text(stmt, 4);
		const char * lastname_ptr = (const char *)sqlite3_column_text(stmt, 5);

		printf("user %-2i: %s\n", id, username_ptr);
		printf("name:  %s %s\n", firstname_ptr, lastname_ptr);
		printf("email: %s\n", email_ptr);
		printf("\n");

		i++;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	printf("DB TEST END\n");
}

#if 0
// TODO: move into http_opts.h
struct option_s {
	const char * name;
	int has_arg;
	int * flag;
	int val;
};
#endif

void print_program_help_msg(const char * argv0) {
#define OPT_MSG "\t%.*s, %s : %s\n"
#define printf_opt(olen, opt_short, opt_long, opt_desc) \
	printf(OPT_MSG, olen, opt_short, opt_long, opt_desc)

	int olen = 10;
	printf("> %s,  HELP:\n", argv0);
	printf_opt(olen, "p", "port", "Set the TCP port number");
	printf_opt(olen, "v", "verbose", "Toggle verbosity ON");
	printf_opt(olen, "h", "help", "Display this message");
	printf_opt(olen, "d", "root_dir", "Set the Root Directory for the Server content");

#undef printf_opt
#undef OPT_MSG
}

int main(int argc, char ** argv) {
	printf("> HTTP SERVER BEGIN\n");
/*
	db_test();
	return 0;
*/

//	ht_test();
//	return 0;


	#define CWD_MAX_SIZE	1024
	char cwd_buffer[CWD_MAX_SIZE] = { 0 };
	char * _cwd = getcwd(cwd_buffer, CWD_MAX_SIZE);

	// TODO: handle vargs input for settings
	int opt = 0;
	int opt_idx = 0;

	// TODO: set global variables or per_context_var?
	int port = PORT;
	int verbose = VERBOSE; 

	#define ROOT_DIR_SIZE	1024
	char root_dir[ROOT_DIR_SIZE] = { 0 };
	size_t root_dir_size = 0;

	// desc:
	static struct option long_options[] = {
		{ "port",    required_argument, 0, 'p' },
		{ "verbose", no_argument,       0, 'v' },
		{ "help",    no_argument,       0, 'h' },
		{ "dir",     required_argument, 0, 'd' },
		{ 0,         0,                 0,  0  }
	};

	while((opt = getopt_long(argc, argv, "p:vhd:", long_options, &opt_idx)) != -1) {
		switch(opt) {
			case 'p':
				int rval = atoi(optarg);
				if(rval != 0) {
					port = atoi(optarg);
				}
				break;
			case 'v':
				verbose = 1;
				break;
			case 'h':
				print_program_help_msg(argv[0]);
				return 0;
			case 'd':{
				char * ptr = optarg;

				// just in case someone does a "-d=XXX"
				// instead of a "--dir=XXX"
				if(*ptr == '=') { ptr++; }

				size_t rlen = strlen(ptr);
				// printf(">>>>> rlen := %li\n", rlen);
				size_t len = (rlen >= (ROOT_DIR_SIZE - 1)) 
					? (ROOT_DIR_SIZE - 1) 
					: rlen;

				if(len != rlen) {
					fprintf(stderr, "> ERR: rel_root_dir len (%li) is too large!\n", rlen);
					return 1;
				}
				
				memcpy(root_dir, ptr, len);
				root_dir[len] = '\0';
				root_dir_size = len;
			} break;
			case '?':
			default:
				fprintf(stderr, "Use --help to see the options.\n");
				return 1;
		}
	}

	printf("> HTTP Server Port = %d\n", port);
	if(verbose) {
		printf("> Verbose Mode Enabled.\n");
	}

#if 1
	// TODO: if rel_root_dir is NULL or length is 0
	// then use the current working directory of the command / program?
	// or should we exit?
	//
	// TODO: we need to fix the directory if we cannot properly find it.
	// ex: "www"  -> ("$pwd" + "/" + www")
	//     "/www" -> ("$pwd" + "/www")
	//
	// TODO: merge cwd + rel root dir into one path plz
	// char rel_root_dir[256] = { 0 };
	{
		int index = 0;
		/*
		if(root_dir[0] != '/') {
			root_dir[index++] = '/';
		}
		*/

		if((root_dir_size + 1) >= ROOT_DIR_SIZE) {
			fprintf(stderr, "> ERR: root directory is too large!\n");
			return 1;
		}

		if(root_dir[0] == 0 || root_dir_size == 0) {
			fprintf(stderr, "> ERR: root directory is NULL\n");
			return 1;
		}

		// expand root_dir plz
		if(root_dir[0] != '/') {
			memmove(root_dir + 1, root_dir, root_dir_size + 1);
			root_dir[0] = '/';
		}

		ctx.root_dir = root_dir;
	}
#endif

	// error prone....
	ctx.root_dir = root_dir;

	printf("root-dir (rel)  = \"%s\"\n", ctx.root_dir);
	printf("root-dir (path) = \"%s\" + \"%s\"\n", _cwd, ctx.root_dir);
	// TODO: set cwd to rel_root_dir plz
	// then we dont have to audit/build all files into a really long path

	init_login_ht();

	// sha256 test
	if(1) {
		char * sha_input = "hello world";
		size_t sha_input_len = strlen(sha_input);

		uint8_t hash[32] = { 0 };
		SHA256_CTX sha_ctx;

		sha256_init(&sha_ctx);
		sha256_update(&sha_ctx, (const uint8_t *)sha_input, sha_input_len);
		sha256_final(&sha_ctx, hash);

		printf("sha256 of \"%s\" = ", sha_input);
		for(int i = 0; i < 32; i++) {
			printf("%02x", hash[i]);
		}
		printf("\n\n");

		// "hello world" -> "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9"
	}

	// parse arguments
	// TODO: handle opts corectly!!
	if(argc < 2) {
		printf("Not enough arguments!\n");
		printf("Please add <root-dir>\n");
		printf("> $http <root-dir>\n");
		exit(1);
	}


	// why do we load only this here
	// and why not store it into ram cache
	// as we call load_file
	{
	cat_large_img_data = malloc(1024 + (1 * 1000 * 1000)); // 1MB why not
	cat_large_img_path = cat_large_img_data;
	cat_large_img_data += 1024;

	{
		char * _path = "www/img/cat_large_img.jpg";
		FILE * fp = fopen(_path, "rb");
		fseek(fp, 0, SEEK_END);
		cat_large_img_data_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		fread(cat_large_img_data, cat_large_img_data_size, 1, fp);
		fclose(fp);

		memcpy(cat_large_img_path, _path + 3, strlen(_path) - 3);
	}
	}

	/*
	char * ext = "js";
	char * mime = get_mime_type(ext);
	printf("ext -> mime\n%s -> %s\n", 
		ext, mime);
	*/

	pid_t server_pid = getpid();
	printf(" ### BEGIN : %i ###\n", server_pid);

	memset(&ctx, 0, sizeof(context_t));
	ctx.client_len = sizeof(ctx.client_addr);

	// memset(&session, 0, sizeof(session_t)); // tmp

	signal(SIGINT, sig_handler);
	
	ctx.server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(ctx.server_socket < 0) {
		printf("Socket creation failed!\n");
		exit(1);
	}

	int sock_opt = 1;
	setsockopt(ctx.server_socket, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt));

	ctx.server_addr.sin_family = AF_INET;
	ctx.server_addr.sin_addr.s_addr = inet_addr("0.0.0.0"); // INADDR_ANY;
	ctx.server_addr.sin_port = htons(port);

	if(bind(ctx.server_socket, (struct sockaddr *)&ctx.server_addr, sizeof(ctx.server_addr)) < 0) {
		printf("Bind failed!\n");
		exit(1);
	}

	if(listen(ctx.server_socket, 10) < 0) {
		printf("Listen failed!\n");
		exit(1);
	}

	printf("Server running on http://localhost:%d\n", port);

	/*
	generate_favicon(favicon_ico);
	printf("favicon data (%zu bytes):\n", sizeof(favicon_ico));
	{
		int offset = 0;
		printf("H: ");
		for(int i = 0; i < FAVICON_HEADER_SIZE; i++) {
			printf("0x%02X ", favicon_ico[offset]);
			offset++;
		}
		printf("\n");

		printf("ID: ");
		for(int i = 0; i < FAVICON_IMAGE_DIRECTORY_SIZE; i++) {
			printf("0x%02X ", favicon_ico[offset]);
			offset++;
		}
		printf("\n");

		printf("DATA: \n");
		for(int i = 0; i < FAVICON_DATA_SIZE; i++) {
			printf("0x%02X ", favicon_ico[offset]);
			if(i > 0 && (i % 15) == 0) { printf("\n"); }
			offset++;
		}
		printf("\n\n");
	}
	*/

	// pthread_barrier_init(&barrieir, NULL, MAX_THREADS);
	pthread_t cli_thread;
	pthread_create(&cli_thread, NULL, cli_worker, NULL);

	pthread_t threads[MAX_THREADS];
	for(int i = 0; i < MAX_THREADS; i++) {
		/*
		pthread_attr_t pt_attr;

		pthread_attr_init(&pt_attr);
		pthread_attr_setdetachstate(&pt_attr, PTHREAD_CREATE_DETACHED);
*/
		pthread_create(&threads[i], NULL, worker, NULL);

//		pthread_attr_destroy(&pt_attr);
//		pthread_detach(threads[i]);
	}

	while(running) {
#if 1
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(ctx.server_socket, &fds);

		struct timeval tv = { .tv_sec = 1, .tv_usec = 0 }; // 1 sec
		int res = select(ctx.server_socket + 1, &fds, NULL, NULL, &tv);

		if(res < 0) {
			// printf("server timeout error\n");
			break;
		}
		
		if(res == 0) {
			// printf("server timeout\n");
			continue;
		}
#endif	

		ctx.client_socket = accept(ctx.server_socket, (struct sockaddr *)&ctx.client_addr, &ctx.client_len);
		if(ctx.client_socket >= 0) {
			task_enqueue(ctx.client_socket);
		}

		// handle client :)
		// task_enqueue(ctx.client_socket);
		// handle_client(ctx.client_socket);
	}

	/* explicit cleanup */
	for(int i = 0; i < MAX_THREADS; i++) {
		(void)pthread_join(threads[i], NULL);
	}

	(void)pthread_join(cli_thread, NULL);

	free_login_ht();

	printf("close server socket\n");
	close(ctx.server_socket);

#if 0
	printf("await barrier\n");
	pthread_barrier_wait(&barrier);
	printf("all threads are finished!\n");
	pthread_barrier_destroy(&barrier);
#endif

	memset(cat_large_img_path, 0, 1024 + (1 * 1000 * 1000));
	free(cat_large_img_path);

	printf(" ### Quit : %i ###\n", server_pid);
	printf("> HTTP SERVER END\n");
	return 0;
}
