#ifndef JSON_CPAC_H
#define JSON_CPAC_H

#include "string8.h"
// TODO(@Rackhamn):
// 	use arena 
// 	use string8 (utf8 lib)
// + parse "U+1234" string into special split codepoint
// 	and handle endianess on it for uint16_t conversion!
// 	LE: 0x34, 0x12
// 	BE: 0x12, 0x34

#define JSON_TOKEN_STRING	0
#define JSON_TOKEN_NUMBER	1
#define JSON_TOKEN_TRUE		2
#define JSON_TOKEN_FALSE	3
#define JSON_TOKEN_NULL		4
#define JSON_TOKEN_LBRACE	5  // {
#define JSON_TOKEN_RBRACE	6  // }
#define JSON_TOKEN_LBRACKET	7  // [
#define JSON_TOKEN_RBRACKET	8  // ]
#define JSON_TOKEN_COLON	9  // :
#define JSON_TOKEN_COMMA	10 // ,
#define JSON_TOKEN_EOF		11
#define JSON_TOKEN_ERR		12
typedef uint8_t token_type_t;

struct json_token_s {
	token_type_t type;
	uint8_t * start; // if we use arena, maybe use [ base_ptr, offset ]
	size_t len;
};
typedef struct json_token_s json_token_t;


struct json_cpac_s {
	// arena_t * arena;
};
typedef struct json_cpac_s json_cpac_t;

// not implemented
json_token_t json_next_token(uint8_t * ptr) {
	json_token_t tok;

	tok.type = 0;
	tok.start = ptr;
	tok.len = 0;

	return tok;
}

void json_cpac_test() {
	/* Expected Output:
	 *  5, '{'
	 *  0, 'name'
	 *  9, ':'
	 *  0, 'admin'
	 * 10, ','
	 *  0, 'age'
	 *  9, ':'
	 *  1, '30'
	 * 10, ','
	 *  0, 'tag'
	 *  9, ':'
	 *  0, '😐'
	 *  6, '}'
	 *
	 * num tokens: 13
	*/
	uint8_t * data = "{\"name\":\"admin\",\"age\":30,\"tag\":\"😐\"}";

	uint8_t * ptr = (uint8_t*)data;
	json_token_t tok;
	do {
		tok = json_next_token(ptr);
		printf("Token: %d, Value: '%.*s'\n", 
			tok.type, (int)tok.len, tok.start);
	} while(tok.type != JSON_TOKEN_EOF && tok.type != JSON_TOKEN_ERR);
}


#endif /* JSON_CPAC_H */
