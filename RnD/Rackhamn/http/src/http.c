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


// global state - plz make threaded...
context_t ctx;
uint8_t favicon_ico[FAVICON_BUF_SIZE] = { 0 };

uint8_t favicon_ex[] = {
// header
0x00, 0x00, 		// reserved 0
0x01, 0x00, 		// image type ico
0x01, 0x00, 		// num images
// direntry
0x10, 			// width 16
0x10,  			// height 16
0x10, 			// num colors (16)
0x00, 			// reserved 0
0x01, 0x00, 		// color planes 1
0x04, 0x00,		// bits per pixel 4
0x28, 0x01, 0x00, 0x00, // size of data (0x128 = 296 bytes)
0x16, 0x00, 0x00, 0x00, // offset of data (0x16 = 22 bytes from 0)
// data
// bmp info header
0x28, 0x00, 0x00, 0x00, // header size (40 bytes)
0x10, 0x00, 0x00, 0x00, // width 16
0x20, 0x00, 0x00, 0x00, // height 16
0x01, 0x00, 		// planes
0x04, 0x00, 		// bpp 4
0x00, 0x00, 0x00, 0x00,	// compression none
0x80, 0x00, 0x00, 0x00, // image size (128 bytes)
0x00, 0x00, 0x00, 0x00, // x px per m
0x00, 0x00, 0x00, 0x00, // y px per m
0x10, 0x00, 0x00, 0x00, // colors 16
0x00, 0x00, 0x00, 0x00, // important colors 0
			// <--- 56 bytes
// 4bpp pixels (BGR_) - 2 pixels per byte!
// color palette
0x00, 0x00, 0x00, 0x00, 
0xff, 0x00, 0x00, 0x00, 
0xff, 0xff, 0xff, 0x00, 
0x00, 0xff, 0x00, 0x00, 
0x00, 0x00, 0xff, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00,
// index data
0x02, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x23, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x22,
0x42, 0x22, 0x22, 0x22, 
0x22, 0x22, 0x22, 0x21,
// AND mask - 64 bytes
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00,       
};

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
	pid_t tid = gettid();
	printf("Thread Worker Start: %i\n", tid);
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
	printf("Thread Worker End: %i\n", tid);
	return NULL;
}
#endif

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

struct mime_type_s {
	size_t ext_size;
	const char * ext;
	const char * mime;
};
typedef struct mime_type_s mime_type_t;

#define MIME_TYPE(ext, mime) (mime_type_t){sizeof(ext), ext, mime}

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
	MIME_TYPE("zip", "application/zip"),
	MIME_TYPE("pdf", "application/pdf"),
	MIME_TYPE("xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetxml.sheet"), // wow...
};

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

char * response_404 = "HTTP/1.1 404 File Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
size_t response_404_len = STRLEN(response_404);

char * response_500 = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nInternal Server Error";
size_t response_500_len = STRLEN(response_500);

void send_response_500(int socket) {
	write(socket, response_500, response_500_len);
}

void send_response_404(int socket) {
	write(socket, response_404, response_404_len);
}

void load_and_send_file(int socket, char * mime, char * path) {
	int fd = open(path, O_RDONLY);
	if(fd < 0) {
		write(socket, response_404, response_404_len);
		return;
	}

	struct stat st;
	if(fstat(fd, &st) == -1) {
		close(fd);
		write(socket, response_500, response_500_len);
		return;
	}

	char header[512];
	size_t header_len = snprintf(header, sizeof(header),
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %zu\r\n"
			"Connection: close\r\n"
			"\r\n",
			mime, st.st_size);

	write(socket, header, header_len);

	char buffer[1024];
	ssize_t n;

	while((n = read(fd, buffer, sizeof(buffer))) > 0) {
		write(socket, buffer, n);
	}

	close(fd);
}

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

	// dump request
	printf("\n ### CLIENT REQUEST : BEGIN ###\n");
	printf("%s\n", buffer);
	printf(" ### CLIENT REQUEST : END ###\n");

	char method[16];
	char path[512];
	char version[16];
	char file_path[1024];

	sscanf(buffer, "%s %s %s", method, path, version);
	printf("method: %s\n", method);
	printf("path: %s\n", path);
	printf("version: %s\n", version);


	if(strcmp(method, "GET") == 0 && strcmp(path, "/favicon.ico") == 0) {
		printf("SEND FAVICON!\n");
		// hack special	
		char response[1024];

		snprintf(response, sizeof(response),
			"%s\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %zu\r\n"
			"\r\n",

			"HTTP/1.1 200 OK",
			"image/x-icon",
			sizeof(favicon_icox));

		printf("response: %s\n", response);

		write(socket, response, strlen(response));
		write(socket, favicon_icox, sizeof(favicon_icox));

		printf("FAVICON SENT!\n");
		close(socket);

		return;
	}

#if 0
	if(strcmp(method, "GET") == 0 && strcmp(path, cat_large_img_path) == 0) {
		// send from ram :)
		char response[1024];

                snprintf(response, sizeof(response),
                        "%s\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %zu\r\n"
                        "\r\n",

                        "HTTP/1.1 200 OK",
                        "image/jpeg",
                        cat_large_img_data_size);

                printf("response: %s\n", response);

                write(socket, response, strlen(response));
                write(socket, cat_large_img_data, cat_large_img_data_size);

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
		char response[4096];
		char * json_response = "{\"status\":\"ok\",\"data\":{\"uid\":\"00112233\",\"img\":\"/img/cat_black.jpg\"}}";

		int response_len = 0;
		response_len = snprintf(response, sizeof(response),
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: %zu\r\n"
			"\r\n"
			"%s",
			strlen(json_response),
			json_response);

		write(socket, response, response_len);
		close(socket);
		return;
	}
	
	char * ext = NULL;
	ext = get_ext(path);
	if(ext == NULL) {
		printf("API CALL\n");
		// maybe its an api call...
		// check that or fail
		char * response = "HTTP/1.1 401 Bad Request\r\n\r\n";
		write(socket, response, strlen(response));
		close(socket);
		return;
	}

	ext++;
	char * mime_type = get_mime_type(ext);
	printf("MIME: %s -> %s\n", ext, mime_type);
	if(mime_type == NULL) {
		send_response_404(socket);
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

	// parse arguments
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
