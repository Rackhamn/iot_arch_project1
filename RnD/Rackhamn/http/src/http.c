#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
// #define _GNU_SOURCE
#include <unistd.h> // gettid()
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "../../arena/arena.h"

#include "sha256.h"
#include "http.h"

#include "favicon.h"

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
	arena_t * arena;
	uint32_t num_files;
	fc_entry_t files[MAX_FILE_CACHE_SIZE];
};
typedef struct file_cache_s fc_t;


char * cat_large_img_data = NULL;
char * cat_large_img_path = NULL;
size_t cat_large_img_data_size = 0;

struct session_s {
	uint8_t id[64]; // session_id <- cookie data
	uint32_t user_id;
};
typedef struct session_s session_t;

session_t session; // tmp

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



#ifndef THREAD_POOL_H
#define THREAD_POOL_H
// glibc hasnt added a weapper for gettid on my system...
// so we now have to do this.
pid_t gettid(void) {
	return syscall(SYS_gettid);
}

// thread local specific data
// if hash is to be used outside of a given thread
// then the data needs to be copied over to a global array / other thread data
// TODO: make struct thread_ctx plz
__thread pid_t thread_id;
__thread SHA256_CTX thread_sha256_ctx;
__thread uint8_t thread_hash[32];

int task_queue[MAX_QUEUE];
int task_front = 0;
int task_rear = 0;
int task_count = 0;

volatile sig_atomic_t running = 1;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_barrier_t barrier;

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

void * worker(void * arg) {
	thread_id = gettid();

	sha256_init(&thread_sha256_ctx);
	memset(thread_hash, 0, sizeof(thread_hash[0]) * 32);

	/*
	sha256_init(&sha_ctx);
	sha256_update(&sha_ctx, (const uint8_t *)sha_input, sha_input_len);
	sha256_final(&sha_ctx, hash);
	*/

	printf("Thread Worker Start: %i\n", thread_id);
	while(1) {
		int client = task_dequeue();
		
		if(client < 0) {
			break;
		}

		handle_client(client);		
	}

#if 0
	printf("Thread await barrier: %i\n", tid);
	pthread_barrier_wait(&barrier);
#endif
	printf("Thread Worker End: %i\n", thread_id);
	return NULL;
}
#endif

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

// TODO: move into code indexed mapped table
#define STRLEN(s) (sizeof(s) - 1)

void http_send_401(int socket) {
	dprintf(socket, "HTTP/1.1 401 Bad Request\r\n\r\n");
}

void http_send_404(int socket) {
	dprintf(socket, "HTTP/1.1 404 File Not Found\r\n\r\n");
}

void http_send_500(int socket) {
	dprintf(socket, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
}

// remove response plz - get from code
void http_send(int socket, int status_code, char * mime_type, uint8_t * data, size_t data_size) {
	dprintf(socket, "HTTP/1.1 %d %s\r\n", status_code, get_status_code_str(status_code));
	dprintf(socket, "Content-Type: %s\r\n", mime_type);
	dprintf(socket, "Content-Length: %zu\r\n", data_size);
	dprintf(socket, "\r\n");

	if(data != NULL) {
		write(socket, data, data_size);
	}
}

void http_send_wcookie(int socket, int status_code, char * cookie_str, char * mime_type, uint8_t * data, size_t data_size) {
	dprintf(socket, "HTTP/1.1 %d %s\r\n", status_code, get_status_code_str(status_code));
	dprintf(socket, "Content-Type: %s\r\n", mime_type);
	dprintf(socket, "Content-Length: %zu\r\n", data_size);

	if(cookie_str != NULL) {
		dprintf(socket, "Set-Cookie: %s\r\n", cookie_str);
		dprintf(socket, "Path=/; HttpOnly; SameSite=Strict; Secure;\r\n");
	}

	dprintf(socket, "\r\n");

	if(data != NULL) {
		write(socket, data, data_size);
	}
}

// dont like this function
// send_static_file(socket, path)
// 	if in ram, mime is known and data is too - hashtable by path
//	if on disk, load file into memory then send
void load_and_send_file(int socket, char * mime_type, char * path) {
	int fd = open(path, O_RDONLY);
	if(fd < 0) {
		// if mime_type == html -> send page_404.html instead ;)
		http_send_404(socket);
		return;
	}

	struct stat st;
	if(fstat(fd, &st) == -1) {
		close(fd);
		http_send_500(socket);
		return;
	}

	http_send(socket, 200, mime_type, NULL, st.st_size);

	char buffer[1024];
	ssize_t n;
	while((n = read(fd, buffer, sizeof(buffer))) > 0) {
		write(socket, buffer, n);
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

void handle_client(int socket) {
	if(socket <= 0) {
		printf("client socket error!\n");
		return;
	}
	// maybe this size has to be bigger?
	// or in an incrementing buffer?
	char buffer[BUFFER_SIZE] = { 0 };
	int bytes_read = read(socket, buffer, sizeof(buffer) - 1);
	if(bytes_read < 0) {
		printf("client read error\n");
		close(socket);
		return;
	}
	buffer[bytes_read] = '\0';

#if 1
	// dump request
	printf("\n ### CLIENT REQUEST : BEGIN ###\n");
	printf("%s\n", buffer);
	printf(" ### CLIENT REQUEST : END ###\n");
#endif

	char method[16];
	char path[512];
	char version[16];
	char file_path[1024];

	// not safe - plz do an actual parse
	// use known cmp for method
	// use whitespace to parse for request path
	// 	also handle %xx into ascii char. what about utf8?
	// use until CLRF to parse for http version
	sscanf(buffer, "%s %s %s", method, path, version);
	printf("method: %s\n", method);
	printf("path: %s\n", path);
	printf("version: %s\n", version);

	// use strncmp or a known cmp
	if(strcmp(method, "GET") == 0 && strcmp(path, "/favicon.ico") == 0) {
		printf("SEND FAVICON!\n");
		http_send(socket, 200, "image/x-ixon", (uint8_t*)favicon_icox, sizeof(favicon_icox));
		printf("FAVICON SENT!\n");
		close(socket);
		return;
	}

#if 1
	if(strcmp(method, "GET") == 0 && strcmp(path, cat_large_img_path) == 0) {
		// send from ram :)
		http_send(socket, 200, "image/jpeg", (uint8_t*)cat_large_img_data, cat_large_img_data_size);
                printf("CAT RAM SENT!\n");
                close(socket);

                return;
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

	// routing plz
	if(strcmp(method, "PUT") == 0 && strncmp(path, "/api", 4) == 0) {
		printf("API CALL\n");
		// API CALL maybe
		char * json_data = "{\"status\":\"ok\",\"data\":{\"uid\":\"00112233\",\"img\":\"/img/cat_black.jpg\"}}";
		size_t json_len = strlen(json_data);

		// http_send(socket, 200, "OK", "application/json", (uint8_t*)json_data, json_len);
		char * cookie = "session_id=admin123";

		http_send_wcookie(socket, 200, cookie, "application/json", (uint8_t*)json_data, json_len);
		close(socket);
		return;
	}
	
	char * ext = NULL;
	ext = get_ext(path);
	if(ext == NULL) {
		printf("API CALL\n");
		// maybe its an api call...
		// check that or fail
		http_send_401(socket);
		close(socket);
		return;
	}

	ext++;
	char * mime_type = get_mime_type(ext);
	printf("MIME: %s -> %s\n", ext, mime_type);
	if(mime_type == NULL) {
		http_send_404(socket);
		close(socket);
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
			path + 1);
	} else {
		snprintf(real_path, sizeof(real_path) - 1, 
			"%s/%s", 
			_root_dir,
			path + 1);

	}

/*
	if(_root_dir == NULL || _sub_dir == NULL || real_path[0] == 0) {
		printf("ERROR\n");
		send_response_404(socket);
		close(socket);
		return;
	}
*/

	load_and_send_file(socket, mime_type, real_path);	
	
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
	close(socket);
}

int main(int argc, char ** argv) {

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

	memset(&session, 0, sizeof(session_t)); // tmp

	signal(SIGINT, sig_handler);


	char cwd_buffer[256] = { 0 };
	char * _cwd = getcwd(cwd_buffer, 256);

	// TODO: merge cwd + rel root dir into one path plz
	char rel_root_dir[256] = { 0 };
	// trash
	{
		int index = 0;
		if(argv[1][0] != '/') {
			rel_root_dir[index++] = '/';
		}
		
		ctx.root_dir = argv[1];
		size_t root_dir_len = strlen(ctx.root_dir);
		memcpy(rel_root_dir + index, ctx.root_dir, root_dir_len);
		index += root_dir_len;

		// depending on the website structure we might not need this at all
#if 0
		if(rel_root_dir[index] != '/') {
			rel_root_dir[index++] = '/';
			rel_root_dir[index++] = '\0';
		}
#endif
		ctx.root_dir = rel_root_dir;
	}

	printf("root-dir (rel)  = \"%s\"\n", ctx.root_dir);
	printf("root-dir (path) = \"%s\" + \"%s\"\n", _cwd, ctx.root_dir);

	ctx.server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(ctx.server_socket < 0) {
		printf("Socket creation failed!\n");
		exit(1);
	}

	int opt = 1;
	setsockopt(ctx.server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	ctx.server_addr.sin_family = AF_INET;
	ctx.server_addr.sin_addr.s_addr = inet_addr("0.0.0.0"); // INADDR_ANY;
	ctx.server_addr.sin_port = htons(PORT);

	if(bind(ctx.server_socket, (struct sockaddr *)&ctx.server_addr, sizeof(ctx.server_addr)) < 0) {
		printf("Bind failed!\n");
		exit(1);
	}

	if(listen(ctx.server_socket, 10) < 0) {
		printf("Listen failed!\n");
		exit(1);
	}

	printf("Server running on http://localhost:%d\n", PORT);

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

	// pthread_barrier_init(&barrier, NULL, MAX_THREADS);
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
	return 0;
}
