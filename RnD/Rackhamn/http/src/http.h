#ifndef HTTP_H
#define HTTP_H

#include <unistd.h>
#include <arpa/inet.h>

#ifndef PORT
#define PORT		8080
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE	4096
#endif

#ifndef MAX_THREADS
#define MAX_THREADS	4
#endif


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

inline const char * get_http_method_string(int http_method);
inline size_t get_http_method_string_size(int http_method);
inline const char * get_mime_type(const char * ext);

int run_server();

#endif /* HTTP_H */
