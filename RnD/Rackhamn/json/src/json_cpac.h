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

#define JSON_TOKEN_ARRAY	0
#define JSON_TOKEN_OBJECT	1
#define JSON_TOKEN_STRING	2
#define JSON_TOKEN_NUMBER	3
#define JSON_TOKEN_BOOL		4
#define JSON_TOKEN_TRUE		5
#define JSON_TOKEN_FALSE	6
#define JSON_TOKEN_NULL		7
#define JSON_TOKEN_LBRACE	8  // {
#define JSON_TOKEN_RBRACE	9  // }
#define JSON_TOKEN_LBRACKET	10 // [
#define JSON_TOKEN_RBRACKET	11 // ]
#define JSON_TOKEN_COLON	12 // :
#define JSON_TOKEN_COMMA	13 // ,
#define JSON_TOKEN_EOF		14
#define JSON_TOKEN_ERR		15
typedef uint8_t json_type_t; // old: token_type_t

// predefine structs as types, otherwise C complains and we cant to mishmash
// typedef struct json_element_s json_element_t;
typedef struct json_value_s json_value_t;

struct json_token_s {
	json_type_t type;
	uint8_t * start; // if we use arena, maybe use [ base_ptr, offset ]
	size_t len;
};
typedef struct json_token_s json_token_t;

struct json_member_s {
	char * key;
	json_value_t * value;
};
typedef struct json_member_s json_member_t;

struct json_value_s {
	json_type_t type;
	union {
		double number;
		// char * cstr;
		// utf8c * str8;
		uint8_t boolean;
		// struct json_array { json_value ** items; size_t count }
		// struct json_object { json_element ** elements; size_t count }
		struct {
			char * chars;
			size_t length;
		} string;
		struct {
			json_value_t ** items;
			size_t count;
		} array;
		struct {
			json_member_t * members;
			size_t count;
		} object;
	};
};

struct json_result_s {
	json_value_t * root;
	char * err;
};
typedef struct json_result_s json_result_t;

void json_skip_whitespace(char **s) {
	while(**s == ' ' || **s == '\n' || **s == '\t' || **s == '\r') {
		(*s)++;
	}
}

json_value_t * json_parse_lit(arena_t * arena, char ** s, char * text, json_type_t type, int bvalue); 
json_value_t * json_parse_string(arena_t * arena, char ** str);
json_value_t * json_parse_value(arena_t * arena, char ** str);
void json_dump_results(json_result_t node, char ** str);

json_value_t * json_parse_number(arena_t * arena, char **s) {
	char * end;
	double num = strtod(*s, &end);
	if(end == *s) {
		return NULL;
	}

	json_value_t * val = arena_alloc(arena, sizeof(json_value_t));
	if(val == NULL) {
		return NULL;
	}

	val->type = JSON_TOKEN_NUMBER;
	val->number = num;

	*s = end;

	return val;
}

json_value_t * json_parse_string(arena_t * arena, char **s) {
	if(**s != '"') {
		return NULL;
	}
	(*s)++;

	char * start = *s;
	while(**s && **s != '"') {
		(*s)++;
	}
	if(**s != '"') {
		return NULL;
	}

	size_t len = *s - start;
	char * cpy = arena_alloc(arena, len + 1);
	if(cpy == NULL) {
		return NULL;
	}

	memcpy(cpy, start, len);
	cpy[len] = '\0';

	json_value_t * val = arena_alloc(arena, sizeof(json_value_t));
	if(val == NULL) {
		return NULL;
	}

	val->type = JSON_TOKEN_STRING;
	val->string.chars = cpy;
	val->string.length = len;

	(*s)++;

	return val;
}

json_value_t * json_parse_array(arena_t * arena, char **s) {
	if(**s != '[') {
		return NULL;
	}
	(*s)++;
	json_skip_whitespace(s);

	json_value_t **items = NULL;
	size_t count = 0;

	if(**s == ']') {
		(*s)++;
	} else {
		while(1) {
			json_skip_whitespace(s);
			json_value_t * item = json_parse_value(arena, s);
			if(item == NULL) {
				return NULL;
			}

			void * new_items = arena_alloc(arena, sizeof(json_value_t *) * (count + 1));
			if(new_items == NULL) {
				return NULL;
			}
			memcpy(new_items, items, sizeof(json_value_t *) * count);
			items = new_items;
			items[count++] = item;

			json_skip_whitespace(s);
			if(**s == ']') {
				(*s)++;
				break;
			} else if(**s == ',') {
				(*s)++;
			} else {
				return NULL;
			}
		}
	}

	json_value_t * val = arena_alloc(arena, sizeof(json_value_t));
	if(val == NULL) {
		return NULL;
	}

	val->type = JSON_TOKEN_ARRAY;
	val->array.items = items;
	val->array.count = count;

	return val;
}

json_value_t * json_parse_object(arena_t * arena, char ** s) {
	if(**s != '{') return NULL;
	(*s)++;
	json_skip_whitespace(s);

	json_member_t * members = NULL;
	size_t count = 0;

	if(**s == '}') {
		(*s)++;
	} else {
		while(1) {
			json_skip_whitespace(s);
			json_value_t * key = json_parse_string(arena, s);

			if(key == NULL) {
				return NULL;
			}
			
			json_skip_whitespace(s);
			if(**s != ':') {
				return NULL;
			}

			(*s)++;
			json_skip_whitespace(s);

			json_value_t * val = json_parse_value(arena, s);
			if(val == NULL) {
				return NULL;
			}

			void * new_members = arena_alloc(arena, sizeof(json_member_t) * (count + 1));
			if(new_members == NULL) {
				return NULL;
			}
			// idk bout this
			memcpy(new_members, members, sizeof(json_member_t) * count);
			members = new_members;

			members[count].key = key->string.chars;
			members[count].value = val;
			count++;

			json_skip_whitespace(s);
			if(**s == '}') {
				(*s)++;
				break;
			} else if(**s == ',') {
				(*s)++;
			} else {
				return NULL;
			}
		}
	}

	json_value_t * obj = arena_alloc(arena, sizeof(json_value_t));
	if(obj == NULL) {
		return NULL;
	}

	obj->type = JSON_TOKEN_OBJECT;
	obj->object.members = members;
	obj->object.count = count;

	return obj;
}

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

	val->type = type;
	if(type == JSON_TOKEN_BOOL || type == JSON_TOKEN_TRUE || type == JSON_TOKEN_FALSE) {
		val->boolean = bvalue;
	}

	(*s) += len;

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
		return json_parse_lit(arena, s, "true", JSON_TOKEN_TRUE, 1);
	}
	if(**s == 'f') {
		return json_parse_lit(arena, s, "false", JSON_TOKEN_FALSE, 0);
	}
	if((**s >= '0' && **s <= '9') || **s == '-') {
		return json_parse_number(arena, s);
	}
	return NULL;
}

json_result_t json_parse(arena_t * arena, char * str) {
	char * s = str;

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
	/*
	json_token_t tok;
	printf("output:\n");
	do {
		tok = json_next_token(ptr);
		// printf("Token: %d, Value: '%.*s'\n", 
		printf("%d: \'.*s\'\n", 
			tok.type, (int)tok.len, tok.start);
	} while(tok.type != JSON_TOKEN_EOF && tok.type != JSON_TOKEN_ERR);
	*/

	arena_t arena;
	arena_create(&arena, 4096 * 4);
	json_result_t result = json_parse(&arena, ptr);

	printf("result: %p, %p\n", result.root, result.err);

	arena_destroy(&arena);
}

#endif /* JSON_CPAC_H */
