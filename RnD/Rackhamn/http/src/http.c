#include <stdio.h>
#include <stdlib.h>

#include "http.h"




enum HTTP_METHODS = {
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

inline 
const char * get_http_method_string(int http_method) {
	const char * string = "";
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
		default:
		string = "";
		break;
	}	

	return string;
}

inline
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


