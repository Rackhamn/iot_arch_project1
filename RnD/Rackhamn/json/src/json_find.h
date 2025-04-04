#ifndef JSON_FIND_H
#define JSON_FIND_H

#include "json_cpac.h"

/*
 * recursive first match lookup from root (root, "theme")
 * first match by path (root, "movie.info.theme")
 *
*/

json_value_t * json_find_by_path(json_value_t * obj, char * path) {
	if(obj == NULL || obj->type != JSON_TOKEN_OBJECT || path == NULL) {
		return NULL;
	}

	char buf[256] = { 0 };
	strncpy(buf, path, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';

	char * token = strtok(buf, ".");
	while(token) {
		if(obj->type != JSON_TOKEN_OBJECT) {
			return NULL;
		}

		json_value_t * next = NULL;
		for(size_t i = 0; i < obj->object.count; i++) {
			if(strcmp(obj->object.members[i].key, token) == 0) {
				next = obj->object.members[i].value;
				break;
			}
		}

		// key not found
		if(next == NULL) {
			return NULL;
		}

		obj = next;
		token = strtok(NULL, ".");
	}

	return obj;
}

#endif /* JSON_FIND_H */
