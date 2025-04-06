#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "http.h"

// global state
context_t ctx;

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
	MIME_TYPE("fav", "image/x-icon"),
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
		cleanup_and_exit(1);
	}
}

int main() {
	/*
	char * ext = "js";
	char * mime = get_mime_type(ext);
	printf("ext -> mime\n%s -> %s\n", 
		ext, mime);
	*/

	memset(&ctx, 0, sizeof(context_t));
	ctx.client_len = sizeof(ctx.client_addr);

	signal(SIGINT, sig_handler);


	ctx.server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(ctx.server_socket < 0) {
		printf("Socket creation failed!\n");
		exit(1);
	}

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

	while(1) {
		ctx.client_socket = accept(ctx.server_socket, (struct sockaddr *)&ctx.client_addr, &ctx.client_len);
		if(ctx.client_socket < 0) {
			printf("Accept failed!\n");
			continue;
		}

		// handle client :)
	}

	close(ctx.server_socket);

	return 0;
}
