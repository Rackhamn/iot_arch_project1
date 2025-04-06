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

#include "http.h"

// TODO:
// 	add CLI interface 
// 		change the debug print / log level
// 		and let task & threads use LOG_printf(level, fmt, ...)

// global state - plz make threaded...
context_t ctx;

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
	printf("Thread Worker End: %i\n", tid);
	return NULL;
}

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

	size_t mime_table_size = sizeof(mime_type_table);
	for(size_t i = 0; i < mime_table_size; i++) {
		if(strncmp(ext, mime_type_table[i].ext, mime_type_table[i].ext_size) == 0) {
			return (char*)mime_type_table[i].mime;
		}
	}

	return "application/octect-stream";
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

	ctx.root_dir = argv[1];

	ctx.server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(ctx.server_socket < 0) {
		printf("Socket creation failed!\n");
		exit(1);
	}

	int opt = 1;
	setsockopt(ctx.server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	ctx.server_addr.sin_family = AF_INET;
	ctx.server_addr.sin_addr.s_addr = INADDR_ANY;
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

	pthread_t threads[MAX_THREADS];
	for(int i = 0; i < MAX_THREADS; i++) {
		pthread_create(&threads[i], NULL, worker, NULL);
		pthread_detach(threads[i]);
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

	/* explicit cleanup
	for(int i = 0; i < MAX_THREADS; i++) {
		(void)pthread_join(threads[i], NULL);
	}
	*/
	
	printf("close server socket\n");
	close(ctx.server_socket);

	printf(" ### Quit : %i ###\n", server_pid);
	return 0;
}
