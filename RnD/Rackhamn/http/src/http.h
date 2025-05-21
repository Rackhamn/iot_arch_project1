#ifndef HTTP_H
#define HTTP_H

#include <unistd.h>
#include <arpa/inet.h>

// These values can be set when compiling,
// and certain ones can be set when running the program.
#ifndef KIB
// 1024 bytes
#define KIB			1024
#endif

#ifndef MIB
// 1024 * 1024 bytes
#define MIB			1048576
#endif

// -----

// Current Working Directory Path Size
#ifndef CWD_MAX_SIZE
#define CWD_MAX_SIZE		1024
#endif

// Content Root Directory Path Size
#ifndef ROOT_DIR_MAX_SIZE
#define ROOT_DIR_MAX_SIZE	1024
#endif

// (currently not implemented)
#ifndef MAX_FILE_CACHE_SIZE
#define MAX_FILE_CACHE_SIZE	16
#endif

// -----

// For controlling the http_request struct size
#ifndef TOKEN_SIZE
#define TOKEN_SIZE	32
#endif

#ifndef METHOD_SIZE
#define METHOD_SIZE	16
#endif

#ifndef PATH_SIZE
#define PATH_SIZE	128
#endif

#ifndef BODY_SIZE
#define BODY_SIZE	1024
#endif

#ifndef HEADERS_SIZE
#define HEADERS_SIZE	1024
#endif

#ifndef FILE_PATH_SIZE
#define FILE_PATH_SIZE	1024
#endif

#ifndef HTTP_VERSION_SIZE
#define HTTP_VERSION_SIZE	16
#endif

// -----

#ifndef PORT
#define PORT			8080
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE		8192
#endif

#ifndef MAX_THREADS
#define MAX_THREADS		4
#endif

// Formerly "MAX_SESSIONS"
#ifndef MAX_QUEUE
#define MAX_QUEUE		128
#endif

#ifndef VERBOSE
#define VERBOSE			0
#endif

// -----

// from DB
struct user_s {
	char username[64];
	char password_hash[128];
	unsigned int id; // db.user_id
};
typedef struct user_s user_t;

struct http_request_s {
	int socket;

	int has_cookie;
	int is_logged_in;
	unsigned char token[TOKEN_SIZE];

	size_t request_size;
	size_t content_size;

	// http request data info
	// should be pointers instead
	// or string(offset, length) structs
	char method[METHOD_SIZE];
	char path[PATH_SIZE];
	char http_version[HTTP_VERSION_SIZE];
	
	char body[BODY_SIZE];
	char headers[HEADERS_SIZE];
	
	// for resources, is the lit. filesystem path
	char file_path[FILE_PATH_SIZE];
};
typedef struct http_request_s http_request_t;


// HTTP Server Context Struct
struct context_s {
	int server_socket;
	struct sockaddr_in server_addr;

	int client_socket;
	struct sockaddr_in client_addr;
	socklen_t client_len;

	// dont really like that this is a ptr - who owns it and where?!
	// use a fixed array?
	char * root_dir; // relative to running directory!!!
};
typedef struct context_s context_t;

// each thread could keep a LRU table of logged in users
struct login_entry_s {
	unsigned long hash;
	time_t created_at; // expiration
	user_t user;
	unsigned char token[TOKEN_SIZE]; 
};
typedef struct login_entry_s login_entry_t;

struct login_hashtable_s {
	int capacity;
	int size;
	login_entry_t * entry;
};
typedef struct login_hashtable_s login_ht_t;

enum HTTP_METHODS {
	// idempotent: making the same request multiple times results in the same effect as making it once.
	POST,	// Submit data to a resource or API call. not idempotent
	GET,	// Retrieve a resource. idempotent
	PUT,	// Replace a resource or create if not exist. idempotent
	DELETE,	// Remove a resource. idempotent (depending on impl.)
	PATCH,	// Apply a partial update to a resource. not always idempotent
	HEAD,	// Like GET, but only retrieves headers (no body). useful for checking if resource exists.
	OPTIONS,// Ask the server what methods are supported for a resource. (ex: CORS preflight request)
	TRACE,	// Echo the recieved request. for debugging, rarely used.
	CONNECT	// Estrablish a tunnel to the server (ex: HTTPS over proxy) Used by browsers and proxies.
};

char * get_http_method_string(int http_method);
size_t get_http_method_string_size(int http_method);
char * get_mime_type(char * ext);

int extract_sessionid_token(char * buffer, unsigned char * token);

void handle_request(http_request_t * req); 
// void handle_api_request(int socket, int method, char * req_path, int has_cookie, unsigned char * token, char * buffer, char * json_in_data)
void handle_api_request(http_request_t * req);

int run_server();

#endif /* HTTP_H */
