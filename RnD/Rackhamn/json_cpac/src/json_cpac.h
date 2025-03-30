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
//
//	needs a json_serialize function to write data into json utf8 buffer
// 	we might as well do a pretty print function

// JSON_TOKEN_ARRAY
// JSON_TOKEN_OBJECT
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

// predefine structs as types, otherwise C complains and we cant to mishmash
typedef struct json_element_s json_element_t;
typedef struct json_value_s json_value_t;

struct json_token_s {
	token_type_t type;
	uint8_t * start; // if we use arena, maybe use [ base_ptr, offset ]
	size_t len;
};
typedef struct json_token_s json_token_t;

struct json_value_s {
	token_type_t type;
	union {
		double number;
		char * cstr;
		// utf8c * str8;
		uint8_t boolean;
		// struct json_array { json_value ** items; size_t count }
		// struct json_object { json_element ** elements; size_t count }
	};
};

struct json_element_s {
	uint8_t * key; // maybe str8 key or string_int
	json_value_t * value;
};

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

json_token_t * json_parse() {
	return NULL;
}

struct json_result_s {
	// ...
};

json_result_t json_parse_string(arena_t * arena, char * str);
void json_dump_results(json_result_t, char * str);



// lit: literal == null, true, false
json_value_t * json_parse_lit(arena_t * arena, char ** s, char * text, json_type_t type, int bvalue) {
	size_t len = strlen(text);
	if(strncmp(*s, text, len) != 0) {
		return NULL;
	}

	json_value_t * val = arena_alloc(arena, sizeof(json_value_t));
	if(val == NULL) {
		return NULL;
	}
	if(type == JSON_TOKEN_BOOL) {
		val->boolean = bvalue;
	}
	*s += len;
	return val;
}

json_value_t * json_parse_value(arena_t * arena, char ** s) {
	json_skip_whitespace(s);
	if(**s == '"') {
		return json_parse_string(arena, s);
	}
	if(**s == '{') {
		return json_parse_object(arena, s);
	}
	if(**s == '[') {
		return json_parse_array(arena, s);
	}
	if(**s == 'n') {
		return json_parse_lit(arena, s, "null", JSON_TOKEN_NULL, 0);
	}
	if(**s == 't') {
		return json_parse_lit(arena, s, "true", JSON_TOKEN_BOOL, 1);
	}
	if(**s == 'f') {
		return json_parse_lit(arena, s, "false", JSON_TOKEN_BOOL, 0);
	}
	if((**s >= '0' && **s <= '9') || **s == '-') {
		return parse_number(arena, s);
	}
	return NULL;
}

json_result_t json_parse(arena_t * arena, char * str) {
	char * s = src;
	json_value_t * root = json_parse_value(arena, &s);
	if(root == NULL) {
		return (json_result_t) { .root = NULL, .err = "parse error" };
	}

	json_skip_whitespace(&s);
	if(*s != '\0') {
		return (json_result_t) { .root = NULL, .err = "trailing data" };
	}

	return (json_result_t) { .root = root, .err = NULL };
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

	printf("input: \"%s\"\n", (char*)data);
	uint8_t * ptr = (uint8_t*)data;
	json_token_t tok;

	printf("output:\n");
	do {
		tok = json_next_token(ptr);
		// printf("Token: %d, Value: '%.*s'\n", 
		printf("%d: \'.*s\'\n", 
			tok.type, (int)tok.len, tok.start);
	} while(tok.type != JSON_TOKEN_EOF && tok.type != JSON_TOKEN_ERR);
}


#endif /* JSON_CPAC_H */
