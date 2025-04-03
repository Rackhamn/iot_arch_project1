#ifndef JSON_DUMP_H
#define JSON_DUPM_H

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

// predefs
void json_dump(json_result_t result);
void json_dump_value(json_value_t * val, int indent);
void json_dump_object(json_value_t * val, int indent);
void json_dump_array(json_value_t * val, int indent);
void json_dump_string(char * str);
void json_print_indent(int level);

// impls.
void json_print_indent(int level) {
	while(level--) {
		// printf(" ");
		putchar(' ');
		putchar(' ');
	}
}

void json_dump_string(char * str) {
	char * s = str;

	printfcol(FCC_GREEN, "{STRING}");
	putchar('"');
	while(*s) {
		if(*s == '"' || *s == '\\') {
			putchar('\\');
			putchar(*s);
		} else if(*s >= 0 && *s < 32) {
			printf("\\u%04X", *s);
		} else {
			putchar(*s);
		}
		*s++;
	}
	putchar('"');
}

void json_dump_array(json_value_t * val, int indent) {
	printfcol(FCC_GREEN, "{ARRAY}");
	printf("[\n");

	for(size_t i = 0; i < val->array.count; i++) {
		json_print_indent(indent + 1);
		json_dump_value(val->array.items[i], indent + 1);

		if(i + 1 < val->array.count) {
			putchar(',');
		}

		putchar('\n');
	}

	json_print_indent(indent);
	putchar(']');
}

void json_dump_object(json_value_t * val, int indent) {
	printfcol(FCC_GREEN, "{OBJECT}");
	printf("{\n");

	for(size_t i = 0; i < val->object.count; i++) {
		json_print_indent(indent + 1);
		json_dump_string(val->object.members[i].key);

		printf(": ");

		json_dump_value(val->object.members[i].value, indent + 1);
		if(i + 1 < val->object.count) {
			putchar(',');
		}

		putchar('\n');
	}

	json_print_indent(indent);
	putchar('}');
}

void json_dump_value(json_value_t * val, int indent) {
	switch(val->type) {
		case JSON_TOKEN_NULL:
			printfcol(FCC_GREEN, "{LIT}");
			printf("null"); 
			break;
		case JSON_TOKEN_BOOL:
		       	printfcol(FCC_GREEN, "{LIT}");	
			printf(val->boolean ? "true" : "false"); 
			break;
		case JSON_TOKEN_TRUE:
			printfcol(FCC_GREEN, "{LIT}");
			printf("true");
			break;
		case JSON_TOKEN_FALSE:
			printfcol(FCC_GREEN, "{LIT}");
			printf("false");
			break;
		case JSON_TOKEN_NUMBER:
			printfcol(FCC_GREEN, "{NUMBER}");
			printf("%g", val->number);
			break;
		case JSON_TOKEN_STRING:
			json_dump_string(val->string.chars);
			break;
		case JSON_TOKEN_ARRAY:
			json_dump_array(val, indent);
			break;
		case JSON_TOKEN_OBJECT: 
			json_dump_object(val, indent);
			break;
		default:
			printfcol(FCC_RED, "<err>");
			break;

	}
}

void json_dump(json_result_t result) {
	if(result.root == NULL) {
		printf("Invalid JSON Data: %s\n", 
			result.err ? result.err : "(unknown error)");
		return;
	}

	json_dump_value(result.root, 0);
	putchar('\n');

#if 0
	// get the "children" key and list the N children strings
	json_value_t * children = NULL;
	children = json_find_key("children");
	if(children) {
		if(children->type == JSON_TOKEN_ARRAY) {
			uint16_t n = 0;
			
		}
	}	
#endif
}

#endif /* JSON_DUMP_H */
