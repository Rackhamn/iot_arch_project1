#ifndef JSON_DUMP_H
#define JSON_DUPM_H

#include <stdarg.h>
#include "json_cpac.h"

#define DEBUG

// void printfcol(char * fmt, ...) {
#define FCC_RED		31
#define FCC_GREEN	32

#if !defined(DEBUG) 
#define printfcol(fcc, ...)
#else
#define printfcol(fcc, ...) \
	printf("\e[1;%dm", fcc); \
	printf(__VA_ARGS__); \
	printf("\e[0m");
#endif

struct json_string_builder_s {
	arena_t * arena;
	char * start;
	size_t offset;
};
typedef struct json_string_builder_s json_sb_t;

// predefs

char* json_write(arena_t * sb, json_result_t result, size_t * out_len);
void json_write_value(json_sb_t * sb, json_value_t * val, int indent);
void json_write_object(json_sb_t * sb, json_value_t * val, int indent);
void json_write_array(json_sb_t * sb, json_value_t * val, int indent);
void json_write_string(json_sb_t * sb, char * str);
void json_write_indent(json_sb_t * sb, int level);

void json_sb_put(json_sb_t * sb, char c) {
	if(sb->offset < sb->arena->size) {
		sb->start[sb->offset++] = c;
	}
}

void json_sb_puts(json_sb_t * sb, char * s) {
	while(*s) {
		json_sb_put(sb, *s);
		s++;
	}
}

void json_sb_indent(json_sb_t * sb, int level) {
	while(level--) {
		json_sb_puts(sb, "  ");
	}
}

// max 128 chars!
void json_sb_printf(json_sb_t * sb, const char * fmt, ...) {
	char temp[128];
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(temp, sizeof(temp), fmt, ap);
	va_end(ap);

	for(int i = 0; i < len; i++) {
		json_sb_put(sb, temp[i]);
	}
}


// impls.
void json_write_string(json_sb_t * sb, char * s) {
	// printfcol(FCC_GREEN, "{STRING}");
	json_sb_put(sb, '"');
	while(*s) {
		if(*s == '"' || *s == '\\') {
			json_sb_put(sb, '\\');
			json_sb_put(sb, *s);
		} else if(*s >= 0 && *s < 32) {
			json_sb_printf(sb, "\\u%04X", *s);
		} else {
			json_sb_put(sb, *s);
		}
		*s++;
	}
	json_sb_put(sb, '"');
}

void json_write_array(json_sb_t * sb, json_value_t * val, int indent) {
	// printfcol(FCC_GREEN, "{ARRAY}");
	json_sb_put(sb, '[');
	json_sb_put(sb, '\n');

	for(size_t i = 0; i < val->array.count; i++) {
		json_sb_indent(sb, indent + 1);

		json_write_value(sb, val->array.items[i], indent + 1);

		if(i + 1 < val->array.count) {
			json_sb_put(sb, ',');
		}

		json_sb_put(sb, '\n');
	}

	json_sb_indent(sb, indent);
	json_sb_put(sb, ']');
}

void json_write_object(json_sb_t * sb, json_value_t * val, int indent) {
	// printfcol(FCC_GREEN, "{OBJECT}");
	json_sb_put(sb, '{');
	json_sb_put(sb, '\n');
	for(size_t i = 0; i < val->object.count; i++) {
		json_sb_indent(sb, indent + 1);
		json_write_string(sb, val->object.members[i].key);

		json_sb_puts(sb, ": ");

		json_write_value(sb, val->object.members[i].value, indent + 1);
		if(i + 1 < val->object.count) {
			json_sb_put(sb, ',');
		}

		json_sb_put(sb, '\n');
	}

	json_sb_indent(sb, indent);
	json_sb_put(sb, '}');
}

void json_write_value(json_sb_t * sb, json_value_t * val, int indent) {
	switch(val->type) {
		case JSON_TOKEN_NULL:
			// printfcol(FCC_GREEN, "{LIT}");
			json_sb_puts(sb, "null");
			break;
		case JSON_TOKEN_BOOL:
		       	// printfcol(FCC_GREEN, "{LIT}");	
			json_sb_puts(sb, val->boolean ? "true" : "false"); 
			break;
		case JSON_TOKEN_TRUE:
			// printfcol(FCC_GREEN, "{LIT}");
			json_sb_puts(sb, "true");
			break;
		case JSON_TOKEN_FALSE:
			// printfcol(FCC_GREEN, "{LIT}");
			json_sb_puts(sb, "false");
			break;
		case JSON_TOKEN_NUMBER:
			// printfcol(FCC_GREEN, "{NUMBER}");
			json_sb_printf(sb, "%g", val->number);
			break;
		case JSON_TOKEN_STRING:
			json_write_string(sb, val->string.chars);
			break;
		case JSON_TOKEN_ARRAY:
			json_write_array(sb, val, indent);
			break;
		case JSON_TOKEN_OBJECT: 
			json_write_object(sb, val, indent);
			break;
		default:
			// printfcol(FCC_RED, "<err>");
			break;

	}
}


char * json_write(arena_t * arena, json_result_t result, size_t * out_len) {
	if(result.root == NULL) {
		printf("Invalid JSON Data: %s\n", 
			result.err ? result.err : "(unknown error)");
		return NULL;
	}

	json_sb_t sb = {
		.arena = arena,
		.start = (char *)arena->data + arena->offset,
		.offset = 0
	};

	json_write_value(&sb, result.root, 0);
	json_sb_put(&sb, '\n');
	json_sb_put(&sb, '\0');
	
	*out_len = sb.offset - 1;
	return sb.start;
}

#endif /* JSON_DUMP_H */
