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

#include <ctype.h>
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

int hex_decode(unsigned char * out, char * hex) {
	size_t i = 0;
	while(hex[i] && hex[i + 1]) {
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

	// turns out that the string is of odd length
	if(hex[i]) {
		return -1;
	}

	return (int)(i / 2);
}
#endif

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

__thread char * thread_request_buffer;
__thread size_t thread_request_buffer_size;
__thread char * thread_response_buffer;
__thread size_t thread_response_buffer_size;

int task_queue[MAX_QUEUE];
int task_front = 0;
int task_rear = 0;
int task_count = 0;

volatile sig_atomic_t running = 1;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
// pthread_barrier_t barrier;

pthread_mutex_t login_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t login_cond = PTHREAD_COND_INITIALIZER;

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

		handle_request(client);		
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
		hprintf(socket, "Path=/; HttpOnly; SameSite=Strict; Secure;\r\n");
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

// todo: write this out pls
struct http_request_s {
	// data ptr
	char * data;
	int base_offset;
};








// from DB
struct user_s {
	char username[64];
	char password_hash[128];
};
typedef struct user_s user_t;


// each thread could keep a LRU table of logged in users
struct login_entry_s {
	unsigned long hash;
	time_t created_at; // expiration
	user_t user;
	unsigned char token[32]; 
};
typedef struct login_entry_s login_entry_t;

struct login_hashtable_s {
	int capacity;
	int size;
	login_entry_t * entry;
};
typedef struct login_hashtable_s login_ht_t;

// global
login_ht_t login_ht;

void init_login_ht() {
	memset(&login_ht, 0, sizeof(login_ht));

	int max_planned_entries = 16; // lowball
	login_ht.capacity = max_planned_entries * 4;
	login_ht.size = 0;
	login_ht.entry = malloc(sizeof(login_entry_t) * login_ht.capacity);

	if(login_ht.entry == NULL) {
//		exit(1);
	}
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
	int index = hash % ht->capacity;
	
	if(ht->entry[index].hash == 0 || ht->entry[index].hash == hash) {
		ht->entry[index].hash = hash;
		return index;
	}

	// find next empty index - incremental search
	int start = index;
	int end = ht->capacity;
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
	int index = hash % ht->capacity;
	
	if(ht->entry[index].hash == hash) {
		return index;
	}

	// find next empty index - incremental search
	int start = index;
	int end = ht->capacity;
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
	int index = login_ht_get_hash_index(ht, hash);
	if(index >= 0) {
		memset(&ht->entry[index], 0, sizeof(login_entry_t));
		return 0;
	}

	return 1; // err
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

void handle_login(int socket, char token[32], char * json_data) {
	user_t tmp_user;

	// move into per thread plz
	arena_t json_arena;
	arena_create(&json_arena, 8192);

	printf("# json data:\n%s\n", json_data);

	json_result_t result = json_parse(&json_arena, json_data);

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
	char sha_input[128] = { 0 };
	memcpy(sha_input, username_value->string.chars, strlen(username_value->string.chars));
	sha_input_len = strlen(sha_input);

	// TODO: use thread local version
        SHA256_CTX sha_ctx;
	sha256_init(&sha_ctx);
	sha256_update(&sha_ctx, (const uint8_t *)sha_input, sha_input_len);
	sha256_final(&sha_ctx, (uint8_t *)token);

	unsigned long hash = hash_djb2(token, 32);
	printf("# hash = %08lX\n", hash);
	int index = login_ht_get_hash_index(&login_ht, hash);
	if(index == -1) {
		printf("hash not in hashtable\n");
		index = login_ht_insert_hash(&login_ht, hash);
		printf("try %i\n", index);
	}

	if(index >= 0) {
		login_ht.entry[index].hash = hash;
		memcpy(login_ht.entry[index].token, token, 32);
		printf("# login_ht index == %i\n", index);
	}


	char cookie[64] = "sessionID=";

	char token_hex[32];
	hex_encode(token_hex, token, 32);

	memcpy(cookie + 10, token, 32);
	cookie[42] = '\0';
	http_send_wcookie(socket, 200, cookie, NULL, NULL, 0);

//	http_send_200(socket);
#if 0
	char * username = json_get(json_data, "username");
	char * password = json_get(json_data, "password");

	user_t user = get_db_user_from_username(username);
	if(user && verify_pasword(password, user->password_hash)) {
		char token[32] = generate_session_token(user);
		store_token(token, user->username);
	
		unsigned long hash = hash_djb2(token, 32);
		int idx = login_ht_insert_hash(&login_ht, hash);
		if(idx < 0) {
			respond_xxx(); // internal server error
			return;
		}

		login_ht.entry[idx].hash = hash;
		memcpy(login_ht.entry[idx].user, user, sizeof(user));
		memcpy(login_ht.entry[idx].token, token, 32);

		// wcookie sessionId=%s, token
		respond_200();
	} else {
		respond_401();
	}
#endif

	arena_destroy(&json_arena);
}



int extract_sessionid_token(char * buffer, char * token) {
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
	if(len == 0) {
		return 0;
	}

	if(len > 32) {
		len = 32;
	}

	for(int i = 0; i < len; i++) {
		token[i] = session_start[i];
	}

	return 1;
}

// thread_response_buffer, thread_request_buffer
void handle_api_request(int socket, char * req_path, char * buffer, char * json_in_data) {
	// should already have been done!!!!
	// figure out which action/ route to take
	int method = 0;
	int api_version = 0;

	char * in_buf = thread_request_buffer;
	char * out_buf = thread_response_buffer;

	if(json_in_data == NULL) {
		http_send_401(socket);
		return;
	}

	int has_cookie = 0;
	unsigned char token[32] = { 0 };

	// grab the sessionID data from the request buffer - not safe
	// cookie = "sessionID="
	//
	has_cookie = extract_sessionid_token(buffer, token);
	if(has_cookie) {
		printf("Token: ");
		for(int i = 0; i < 32; i++) {
			printf("%x", token[i]);
		}
		printf("\n");
	}

	if(strncmp(req_path, "/api/v1/login", 13) == 0) {
		printf("API V1 LOGIN\n");
		handle_login(socket, token, json_in_data);
		return;
	}

	// check if logged in

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


	printf("HELP\n");

	char * json_data = "{\"status\":\"ok\",\"data\":{\"uid\":\"00112233\",\"img\":\"/img/cat_black.jpg\"}}";
	size_t json_len = strlen(json_data);

	// http_send(socket, 200, "OK", "application/json", (uint8_t*)json_data, json_len);
	char * cookie = "session_id=admin123";

	http_send_wcookie(socket, 200, cookie, "application/json", (uint8_t*)json_data, json_len);
	
	close(socket);

	return;
}

// file request - standard OK for all
void handle_request(int socket) {
	// todo: assert before entering the handle_client call
	if(socket <= 0) {
		printf("client socket error!\n");
		return;
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
	
	int bytes_read = read(socket, buffer, buffer_size - 1);//sizeof(buffer) - 1);
	if(bytes_read < 0) {
		printf("client read error\n");
		close(socket);
		return;
	}
	buffer[bytes_read] = '\0';

#if 1
	// dump request - use verbose flags!
	printf("\n ### CLIENT REQUEST : BEGIN ###\n");
	printf("%s\n", buffer);
	printf(" ### CLIENT REQUEST : END ###\n");
#endif

	// do before handle_request and handle_api_request
	// expose to next function.
	char method[16] = { 0 };
	char path[512] = { 0 };
	char version[16] = { 0 };
	char file_path[1024] = { 0 };

	// not safe - plz do an actual parse
	// use known cmp for method
	// use whitespace to parse for request path
	// 	also handle %xx into ascii char. what about utf8?
	// use until CLRF to parse for http version
	sscanf(buffer, "%s %s %s", method, path, version);
	printf("method: %s\n", method);
	printf("path: %s\n", path);
	printf("version: %s\n", version);

	
	// state
	int is_api_request_ = (strncmp(path, "/api", 4) == 0); // use strncmp or a known cmp
	
	if(is_api_request_) {
	//int method_ = get_method(method_str);
		size_t content_length = 0;

		char * p = buffer;
		char * content_length_header = strstr(p, "Content-Length:");
		p = content_length_header + strlen("Content-Length:");
	
		int len = atoi(p);
		printf("# Content-Length: %i\n", len);
		if(len > 0) {
			content_length = len;
		}

		if(len == 0) {
			// something is probably wrong?
			http_send_401(socket);
			close(socket);
			return;
		}

		// call api handler with method
		char * eorp = (buffer + bytes_read) - content_length;
		handle_api_request(socket, path, buffer, eorp);
		close(socket);
		return;
	}

	if(strcmp(method, "GET") == 0 && strcmp(path, "/favicon.ico") == 0) {
		printf("SEND FAVICON!\n");
		http_send(socket, 200, "image/x-icon", (uint8_t*)favicon_icox, sizeof(favicon_icox));
		printf("FAVICON SENT!\n");
		close(socket);
		return;
	}

#if 1
	// valgrind complains if we dont copy the data over first
	// says uninitialied value
	char catpath[512] = { 0 };
	memcpy(catpath, cat_large_img_path, strlen(cat_large_img_path));

	// if(strcmp(method, "GET") == 0 && strcmp(path, cat_large_img_path) == 0) {
	if(strcmp(method, "GET") == 0 && strcmp(path, catpath) == 0) {
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

//	ht_test();
//	return 0;

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
	return 0;
}
