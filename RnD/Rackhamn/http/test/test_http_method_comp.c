#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "macros.h"

#define NUM_INPUTS	1000000
#define NUM_METHODS	9

typedef enum {
	GET, 
	POST, 
	PUT, 
	DELETE, 
	PATCH, 
	HEAD, 
	OPTIONS, 
	TRACE, 
	CONNECT, 
	// -----
	INVALID = -1
} http_method;

typedef struct {
    const char *str;
    size_t len;
    int value;
} method_entry_t;

#define STRLEN(s) (sizeof(s) - 1)
#define SSTR(s) #s
#define HME(x) { SSTR(x), STRLEN(SSTR(x)), (int)(x) }
#define _HME(x) { #x, (sizeof(#x) - 1), (int)x }

static const method_entry_t method_table[NUM_METHODS] = {
// I really wish that C had actual pre-compile (or compile-time) iteration for stuff like this.
// Typing it out is rather annoying when the baseline input array might change.
//
// GEN_LIST(HME, GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS, TRACE, CONNECT)
//
// * in c lingo, maybe something like this *
// MACRO(
// 	APPLY(HME), 
// 	ON(http_method, 0, ENUM_SIZE(http_method) - 1),
// 	SEPARATOR(,)
// )
//
// * or, even easier *
// #define ENUMSN1(x) (ENUM_SIZE(x) - 1)
// #define ENUM_START_ENDSN1(x) x, s, ENUMSN1(x)
// FOREACH_ENUM(APPLY, HME, ENUM_START_ENDSN1(http_method()
//
// A compile time function that can generate code would be SO much nicer to use.
// It would have access to the structs and types and names of all things.
// And also the data for those elements.
//
// @ct: apply_fmt_macro_on("{ %0, %1, %2 }, ", macro, http_methods.elements, 0, http_methods.size - 1)
// 	-> ess. 
// 	for(int i = 0; i < max; i++) {
//		ct_lw_strcat(...);
// 	}
//

// max 16 elements
FOREACH(HME, GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS, TRACE, CONNECT)

#if 0
    { "GET",     3, GET },
    { "POST",    4, POST },
    { "PUT",     3, PUT },
    { "DELETE",  6, DELETE },
    { "PATCH",   5, PATCH },
    { "HEAD",    4, HEAD },
    { "OPTIONS", 7, OPTIONS },
    { "TRACE",   5, TRACE },
    { "CONNECT", 7, CONNECT }
#endif
};

typedef struct input_info_s {
	char str[16];
	int val;
} input_info_t;

// static char input_data[NUM_INPUTS][16]; // Holds test strings
static input_info_t input_data[NUM_INPUTS];

// Util: Format time into SS:MS:US:NS
void print_elapsed(struct timespec start, struct timespec end) {
    uint64_t start_ns = (uint64_t)start.tv_sec * 1000000000 + start.tv_nsec;
    uint64_t end_ns = (uint64_t)end.tv_sec * 1000000000 + end.tv_nsec;
    uint64_t delta = end_ns - start_ns;

    uint64_t ss = delta / 1000000000;
    delta %= 1000000000;
    uint64_t ms = delta / 1000000;
    delta %= 1000000;
    uint64_t us = delta / 1000;
    uint64_t ns = delta % 1000;

    printf("\nTime: %02lu:%03lu:%03lu:%03lu\n", ss, ms, us, ns);
}

// Variant 1: Simple strcmp loop
int match_v1(const char *s) {
    for (int i = 0; i < NUM_METHODS; i++) {
        if (strcmp(s, method_table[i].str) == 0)
            return method_table[i].value;
    }
    return INVALID;
}

// Variant 2: strncmp with null check
int match_v2(const char *s) {
    for (int i = 0; i < NUM_METHODS; i++) {
        if (strncmp(s, method_table[i].str, method_table[i].len) == 0 &&
            s[method_table[i].len] == '\0') {
            return method_table[i].value;
        }
    }
    return INVALID;
}

// Variant 3: switch on first char
int match_v3(const char *s) {
    switch (s[0]) {
        case 'G': if (strcmp(s, "GET") == 0) return GET; break;
        case 'P':
            if (strcmp(s, "POST") == 0) return POST;
            if (strcmp(s, "PUT") == 0) return PUT;
            if (strcmp(s, "PATCH") == 0) return PATCH;
            break;
        case 'D': if (strcmp(s, "DELETE") == 0) return DELETE; break;
        case 'H': if (strcmp(s, "HEAD") == 0) return HEAD; break;
        case 'O': if (strcmp(s, "OPTIONS") == 0) return OPTIONS; break;
        case 'T': if (strcmp(s, "TRACE") == 0) return TRACE; break;
        case 'C': if (strcmp(s, "CONNECT") == 0) return CONNECT; break;
    }
    return INVALID;
}

// Variant 3.2: switch on first char
int match_v3_2(const char *s) {
    switch (s[0]) {
        case 'G': if (memcmp(s, "GET", 3) == 0) return GET; break;
        case 'P':
            if (memcmp(s, "POST", 4) == 0) return POST;
            if (memcmp(s, "PUT", 3) == 0) return PUT;
            if (memcmp(s, "PATCH", 5) == 0) return PATCH;
            break;
        case 'D': if (memcmp(s, "DELETE", 6) == 0) return DELETE; break;
        case 'H': if (memcmp(s, "HEAD", 5) == 0) return HEAD; break;
        case 'O': if (memcmp(s, "OPTIONS", 7) == 0) return OPTIONS; break;
        case 'T': if (memcmp(s, "TRACE", 5) == 0) return TRACE; break;
        case 'C': if (memcmp(s, "CONNECT", 7) == 0) return CONNECT; break;
    }
    return INVALID;
}

// Variant 3.3: switch on first char
int match_v3_3(const char *s) {
    // should prob. check the final value too
    // as long as its any space_char or \0 its ok.
    switch (s[0]) {
        case 'G': 
		if(s[1] == 'E' && s[2] == 'T') return GET; break;
        case 'P':
	    if(s[1] == 'U' && s[2] == 'T') return PUT;
	    if(s[1] == 'O' && s[2] == 'S' && s[3] == 'T') return POST;
	    if(s[1] == 'A' && s[2] == 'T' && s[3] == 'T' && s[4] == 'C' && s[5] == 'H') return PATCH;
            break;
        case 'D': 
	    if(s[1] == 'E' && s[2] == 'L' && s[3] == 'E' && s[4] == 'T' && s[5] == 'E') return DELETE;
	    break;
        case 'H': 
	    if(s[1] == 'E' && s[2] == 'A' && s[3] == 'D') return HEAD;
	    break;
        case 'O': 
	    if(s[1] == 'P' && s[2] == 'T' && s[3] == 'I' && s[4] == 'I' && s[5] == 'O' && s[6] == 'N' && s[7] == 'S') return OPTIONS;
            break;
	case 'T': 
            if(s[1] == 'R' && s[2] == 'A' && s[3] == 'C' && s[4] == 'E') return TRACE; 
	    break;
        case 'C': 
	    if (memcmp(s, "CONNECT", 7) == 0) 
	    if(s[1] == 'O' && s[2] == 'N' && s[3] == 'N' && s[4] == 'E' && s[5] == 'C' && s[6] == 'T') return CONNECT; 
	    break;
    }
    return INVALID;
}



#if 1

// "POST"
// -> 0x50, 0x4F, 0x53, 0x54
// --> 0x54534F50 (LE, X86)

// 4 Bytes, 32-bit
#define U32_4(a,b,c,d) \
	((uint32_t)(a) | ((uint32_t)(b)<<8) | ((uint32_t)(c)<<16) | ((uint32_t)(d)<<24))

#define U32_3(a,b,c) \
	((uint32_t)(a) | ((uint32_t)(b)<<8) | ((uint32_t)(c)<<16))

#define U32_2(a,b) \
	((uint32_t)(a) | ((uint32_t)(b)<<8))

#define U32_1(a) \
	((uint32_t)(a))

// masks
#define MASK_LAST_BYTE	0x00FFFFFF
#define MASK_FIRST_BYTE	0xFFFFFF00
#define MASK_EXACT	0xFFFFFFFF

// direct exact match
#define XYEQ44(x, y, c0, c1, c2, c3, c4, c5, c6, c7) \
	(((x) == U32_4(c0, c1, c2, c3)) && \
	 ((y) == U32_4(c4, c5, c6, c7)))

// exact match with masks
#define XYEQ44MM(x, y, xm, ym, c0, c1, c2, c3, c4, c5, c6, c7) \
	(((x & xm) == (U32_4(c0, c1, c2, c3) & xm) && \
	 ((y & ym) == (U32_4(c4, c5, c6, c7) & ym))

// exact X and masked Y match
#define XYEQ44_M(x, y, ex, ey, mask, c0, c1, c2, c3, c4, c5, c6, c7) \
	((x == ex) && ((y & mask) == (ey & mask)))

// exact X and masked Y match (Alternative)
#define XYEQ44_MA(x, ex, y, ey, mask) \
	((x == ex) && ((y & mask) == (ey & mask)))


#if 0 /* THE IDEA */
	// read the UNSIGNED INTEGER value 
	// and attempt to mask away and mask on it.
	
	unsigned int x, y;
	x = *((unsigned int *)s);
	y = *((unsigned int *)(s + 4));

	// GET
	(x & 0xF) == (unsigned int)"GET "

	// PUT
	(x & 0xF) == (unsigned int)"PUT "

	// POST
	(x)       == (unsigned int)"POST"

	// HEAD
	(x)       == (unsigned int)"HEAD"

	// PATCH
	(x)       == (unsigned int)"PATC"
	(y & 0xFFF) == (unsigned int)"H   "

	// TRACE
	(x)       == (unsigned int)"TRAC"
	(y & 0xFFF) == (unsigned int)"E   "

	// DELETE
	(x)       == (unsigned int)"DELE"
	(y & 0xFF)== (unsigned int)"TE  "

	// OPTIONS
	(x)       == (unsigned int)"OPTI"
	(y & 0xF) == (unsigned int)"ONS "

	// CONNECT
	(x)       == (unsigned int)"CONN"
	(y & 0xF) == (unsigned int)"ECT "
#endif

// HOPE that 8 bytes comes through
int match_v3_4(const char * s) {
#if 0
	// alignment might break this - mayby memcpy
	uint32_t x = *(const uint32_t *)s;
	uint32_t y = *(const uint32_t *)(s + 4);
#else
	// this copy is probably not making us faster --- but it *is* keeping us aligned?
	uint32_t x, y; 
	memcpy(&x, s, 4);
	memcpy(&y, s + 4, 4);
#endif

	

	if((x & 0x00FFFFFFF) == U32_3('G', 'E', 'T')) return GET;
	if((x & 0x00FFFFFF) == U32_3('P', 'U', 'T')) return PUT;
	if((x & 0xFFFFFFFF) == U32_4('P', 'O', 'S', 'T')) return POST;

	// DELETE
	if((x == U32_4('D', 'E', 'L', 'E')) &&
	   ((y & 0x0000FFFF) == U32_2('T', 'E'))) {
		return DELETE;
	}

	// PATCH
	if((x == U32_4('P', 'A', 'T', 'C')) &&
	   ((y & 0x000000FF) == U32_1('H'))) {
		return PATCH;
	}

	// HEAD
	if((x == U32_4('H', 'E', 'A', 'D'))) {
		return HEAD;
	}


	// OPTIONS
	if(XYEQ44_MA(x, U32_4('O', 'P', 'T', 'I'), y, U32_3('O', 'N', 'S'), 0x00FFFFFF)) {
		return OPTIONS;
	}

	// TRACE
	if(XYEQ44_MA(x, U32_4('T', 'R', 'A', 'C'), y, U32_1('E'), 0x000000FF)) {
		return TRACE;
	}

	// CONNECT
	if(XYEQ44_MA(x, U32_4('C', 'O', 'N', 'N'), 
		     y, U32_4('E', 'C', 'T', '\0'),
		     0xFFFFFF00)) {
		// MASK_LAST_BYTE
		return CONNECT;
	}
	/* above is same as this:
	
	uint32_t ex, ey, mask;
	ex = U32_4('C', 'O', 'N', 'N');
	ey = U32_4('E', 'C', 'T', '\0');
	mask = MASK_LAST_BYTE;
	if(x == ex && ((y & mask) == (ey & mask))) {
		return CONNECT;
	}
	*/

	/*
	if(XYEQ44(x, y, 'D', 'E', 'L', 'E', 'T', 'E', 0, 0)) {
		return DELETE;
	}
	*/

	return -1;
}


// --- What are we even doing man ---
// GET
#define S_GET	U32_3('G', 'E', 'T')
// PUT
#define S_PUT	U32_3('P', 'U', 'T')
// POST
#define S_POST	U32_4('P', 'O', 'S', 'T')
// HEAD
#define S_HEAD	U32_4('H', 'E', 'A', 'D')
// DELETE
#define S_DELE	U32_4('D', 'E', 'L', 'E')
#define S_TE	U32_2('T', 'E')
// PATCH
#define S_PATC	U32_4('P', 'A', 'T', 'C')
#define S_H	U32_1('H')
// OPTIONS
#define S_OPTI	U32_4('O', 'P', 'T', 'I')
#define S_ONS	U32_3('O', 'N', 'S')
// TRACE
#define S_TRAC	U32_4('T', 'R', 'A', 'C')
#define S_E	U32_1('E')
// CONNECT
#define S_CONN	U32_4('C', 'O', 'N', 'N')
#define S_ECT	U32_3('E', 'C', 'T')

// HOPE that 8 bytes comes through
int match_v3_5(const char * s) {


#if 0
	// alignment might break this - mayby memcpy
	uint32_t x = *(const uint32_t *)s;
	uint32_t y = *(const uint32_t *)(s + 4);
#else
	// this copy is probably not making us faster --- but it *is* keeping us aligned?
	uint32_t x, y; 
	memcpy(&x, s, 4);
	memcpy(&y, s + 4, 4);
#endif

	// single WORD first
	switch(x & 0x00FFFFFF) {
		case U32_3('G', 'E', 'T'): return GET;
		case U32_3('P', 'U', 'T'): return PUT;
	}

	switch(x & 0xFFFFFFFF) {
		case U32_4('P', 'O', 'S', 'T'): return POST;
		case U32_4('H', 'E', 'A', 'D'): return HEAD;
	}

#if 0
	// 2 WORD second
	uint64_t k = ((uint64_t)x) | ((uint64_t)y << 32);
	switch(k) {
		case ((uint64_t)U32_4('D', 'E', 'L', 'E') | ((uint64_t)U32_2('T', 'E') << 32)):
			return DELETE;
		case ((uint64_t)U32_4('P', 'A', 'T', 'C') | ((uint64_t)U32_1('H') << 32)):
			return PATCH;
		case ((uint64_t)U32_4('O', 'P', 'T', 'I') | ((uint64_t)U32_3('O', 'N', 'S') << 32)):
			return OPTIONS;
		case ((uint64_t)U32_4('T', 'R', 'A', 'C') | ((uint64_t)U32_1('E') << 32)):
			return TRACE;
		case ((uint64_t)U32_4('C', 'O', 'N', 'N') | ((uint64_t)U32_3('E', 'C', 'T') << 32)):
			return CONNECT;
	}
#else

	switch(x) {
		case S_DELE: { if (y == S_TE)  { return DELETE;  } } break;
		case S_PATC: { if (y == S_H)   { return PATCH;   } } break;
		case S_OPTI: { if (y == S_ONS) { return OPTIONS; } } break;
		case S_TRAC: { if (y == S_E)   { return TRACE;   } } break;
		case S_CONN: { if (y == S_ECT) { return CONNECT; } } break;
	}

#endif

	return -1;	
}


// 8-Bytes, 64-bit
#define U64_8(a,b,c,d,e,f,g,h) \
	((uint64_t)(a) | ((uint64_t)(b)<<8) | ((uint64_t)(c)<<16) | ((uint64_t)(d)<<24) | \
	 ((uint64_t)(e)<<32) | ((uint64_t)(f)<<40) | ((uint64_t)(g)<<48) | ((uint64_t)(h)<<52))

#define U64_7(a,b,c,d,e,f,g) \
	((uint64_t)(a) | ((uint64_t)(b)<<8) | ((uint64_t)(c)<<16) | ((uint64_t)(d)<<24) | \
	 ((uint64_t)(e)<<32) | ((uint64_t)(f)<<40) | ((uint64_t)(g)<<48))

#define U64_6(a,b,c,d,e,f) \
	((uint64_t)(a) | ((uint64_t)(b)<<8) | ((uint64_t)(c)<<16) | ((uint64_t)(d)<<24) | \
	 ((uint64_t)(e)<<32) | ((uint64_t)(f)<<40))

#define U64_5(a,b,c,d,e) \
	((uint64_t)(a) | ((uint64_t)(b)<<8) | ((uint64_t)(c)<<16) | ((uint64_t)(d)<<24) | \
	 ((uint64_t)(e)<<32))

#define U64_4(a,b,c,d) \
	((uint64_t)(a) | ((uint64_t)(b)<<8) | ((uint64_t)(c)<<16) | ((uint64_t)(d)<<24))

#define U64_3(a,b,c) \
	((uint64_t)(a) | ((uint64_t)(b)<<8) | ((uint64_t)(c)<<16))

#define U64_2(a,b) \
	((uint64_t)(a) | ((uint64_t)(b)<<8))

#define U64_1(a) \
	((uint64_t)(a))


/* '_' == masked out or 0	
"GET_____"
"PUT_____"
"POST____"
"HEAD____"
"DELETE__"
"PATCH___"
"OPTIONS_"
"TRACE___"
"CONNECT_"
*/
int match_v3_6(const char * s) {
	uint64_t k = 0;

	// unaligned load, x86_64 might say its ok?
	k = *(const uint64_t *)s;
	/*
	memcpy(((unsigned int *)&k), s, 4);
	memcpy(((unsigned int *)&k) + 1, s + 4, 4);	
	*/

	switch(k) {
		case U64_3('G', 'E', 'T'): return GET;
		case U64_3('P', 'U', 'T'): return PUT;
		case U64_4('P', 'O', 'S', 'T'): return POST;
		case U64_4('H', 'E', 'A', 'D'): return HEAD;
		case U64_5('P', 'A', 'T', 'C', 'H'): return PATCH;
		case U64_5('T', 'R', 'A', 'C', 'E'): return TRACE;
		case U64_6('D', 'E', 'L', 'E', 'T', 'E'): return DELETE; 
		case U64_7('O', 'P', 'T', 'I', 'O', 'N', 'S'): return OPTIONS;
		case U64_7('C', 'O', 'N', 'N', 'E', 'C', 'T'): return CONNECT;
	}

	return -1;
}


// "branchless"
int match_v3_7(const char * s) {
        uint64_t k = 0;

	k = *(const uint64_t *)s;
	/*
        memcpy(((unsigned int *)&k), s, 4);
        memcpy(((unsigned int *)&k) + 1, s + 4, 4);
	*/

	// the values should already be computed somewhere before running the code
	static const uint64_t GET_v = U64_3('G', 'E', 'T');
	static const uint64_t PUT_v = U64_3('P', 'U', 'T');
	static const uint64_t POST_v = U64_4('P', 'O', 'S', 'T');
	static const uint64_t HEAD_v = U64_4('H', 'E', 'A', 'D');
	static const uint64_t PATCH_v = U64_5('P', 'A', 'T', 'C', 'H');
	static const uint64_t TRACE_v = U64_5('T', 'R', 'A', 'C', 'E');
	static const uint64_t DELETE_v = U64_6('D', 'E', 'L', 'E', 'T', 'E');
	static const uint64_t OPTIONS_v = U64_7('O', 'P', 'T', 'I', 'O', 'N', 'S');
	static const uint64_t CONNECT_v = U64_7('C', 'O', 'N', 'N', 'E', 'C', 'T');
       
	// compute match as all 0 or all 1 bits
	uint64_t m_get = -(k == GET_v);
	uint64_t m_put = -(k == PUT_v);
	uint64_t m_post = -(k == POST_v);
	uint64_t m_head = -(k == HEAD_v);
	uint64_t m_patch = -(k == PATCH_v);
	uint64_t m_trace = -(k == TRACE_v);
	uint64_t m_delete = -(k == DELETE_v);
	uint64_t m_options = -(k == OPTIONS_v);
	uint64_t m_connect = -(k == CONNECT_v);

	// combo enum mask
	int res =
		(m_get & GET) |
		(m_put & PUT) |
		(m_post & POST) |
		(m_head & HEAD) |
		(m_patch & PATCH) |
		(m_trace & TRACE) |
		(m_delete & DELETE) |
		(m_options & OPTIONS) |
		(m_connect & CONNECT);	

	uint64_t match = 
		m_get | m_put |
		m_post | m_head |
		m_patch | m_trace |
		m_delete |
		m_options | m_connect;

	return match ? res : -1;
        // return -1;
}



int match_v3_8(const char * s) {
// string -> 64-bit -> HEX (LE)
#define Z64H_GET 	0x0000000000544547
#define Z64H_PUT	0x0000000000545550
#define Z64H_POST	0x0000000054534F50
#define Z64H_HEAD	0x0000000044414548
#define Z64H_PATCH	0x0000004843544150
#define Z64H_TRACE	0x0000004543415254
#define Z64H_DELETE	0x00004554454C4544
#define Z64H_OPTIONS	0x00534E4F4954504F
#define Z64H_CONNECT	0x005443454E4E4F43

	const uint64_t k = *(const uint64_t *)s;

#if 0
	// compute match as all 0 or all 1 bits
        uint64_t m_get = -(k == Z64H_GET);
        uint64_t m_put = -(k == Z64H_PUT);
        uint64_t m_post = -(k == Z64H_POST);
        uint64_t m_head = -(k == Z64H_HEAD);
        uint64_t m_patch = -(k == Z64H_PATCH);
        uint64_t m_trace = -(k == Z64H_TRACE);
        uint64_t m_delete = -(k == Z64H_DELETE);
        uint64_t m_options = -(k == Z64H_OPTIONS);
        uint64_t m_connect = -(k == Z64H_CONNECT);

        // combo enum mask
        int res =
                (m_get & GET) |
                (m_put & PUT) |
                (m_post & POST) |
                (m_head & HEAD) |
                (m_patch & PATCH) |
                (m_trace & TRACE) |
                (m_delete & DELETE) |
                (m_options & OPTIONS) |
                (m_connect & CONNECT);

        uint64_t match =
                m_get | m_put |
                m_post | m_head |
                m_patch | m_trace |
                m_delete |
                m_options | m_connect;
#else
	#define RES(x) (-(k == Z64H_##x) & x)
	int res = 
		// (-(k == Z64H_GET) & GET) |
		RES(GET) |
		RES(PUT) |
		RES(POST) |
		RES(HEAD) |
		RES(PATCH) |
		RES(TRACE) |
		RES(DELETE) |
		RES(OPTIONS) |
		RES(CONNECT);
	#undef RES

	#define KEQ(x) (k == Z64H_##x)
	int match = 
		KEQ(GET) |
		KEQ(PUT) |
		KEQ(POST) | 
		KEQ(HEAD) |
		KEQ(PATCH) |
		KEQ(TRACE) |
		KEQ(DELETE) |
		KEQ(OPTIONS) |
		KEQ(CONNECT);
	#undef KEQ

#endif
        return match ? res : -1;
}


#include <emmintrin.h> // SSE2
int match_v3_9(const char * s) {
	// SSE2 version	
	uint64_t k = *((const uint64_t *)s);
	uint32_t lo = (uint32_t)(k);
	uint32_t hi = (uint32_t)(k >> 32); // not sure
	
	__m128i key = _mm_set_epi32(hi, lo, hi, lo);

	static const uint64_t lut[9] = {
		0x0000000000544547ULL, // GET
		0x0000000000545550ULL, // PUT
		0x0000000054534F50ULL, // POST
		0x0000000044414548ULL, // HEAD
		0x0000004843544150ULL, // PATCH
		0x0000004543415254ULL, // TRACE
		0x00004554454C4544ULL, // DELETE
		0x00534E4F4954504FULL, // OPTIONS
		0x005443454E4E4F43ULL  // CONNECT
	};
	
	static const int mlut[9] = {
		GET, PUT, 
		POST, HEAD,
		PATCH, TRACE,
		DELETE,
		OPTIONS, CONNECT
	};

	/*
	uint64_t val;
	uint32_t vlo, vhi;
	__m128i candidate, cmp;
	int mask;
	*/

	for(int i = 0; i < 9; ++i) {
		uint64_t val = lut[i];
		uint32_t vlo = (uint32_t)(val);
		uint32_t vhi = (uint32_t)(val >> 32);

		__m128i candidate = _mm_set_epi32(vhi, vlo, vhi, vlo);
		__m128i cmp = _mm_cmpeq_epi32(key, candidate);
		int mask = _mm_movemask_epi8(cmp);

		if(mask == 0xFFFF) {
			return mlut[i];
		}
	}

	return -1;
}

// unrolled sse2
int match_v3_10(const char * s) {
	uint64_t k = *(const uint64_t *)s;

	uint32_t lo = (uint32_t)(k);
	uint32_t hi = (uint32_t)(k >> 32);
	__m128i key = _mm_set_epi32(hi, lo, hi, lo);

#define CMP_TAG(hex, tag) 					\
do { 								\
	uint32_t vlo = (uint32_t)(hex); 			\
	uint32_t vhi = (uint32_t)((hex) >> 32); 		\
	__m128i cand = _mm_set_epi32(vhi, vlo, vhi, vlo); 	\
	__m128i cmp = _mm_cmpeq_epi32(key, cand); 		\
	if (_mm_movemask_epi8(cmp) == 0xFFFF) return (tag); 	\
} while(0)

	CMP_TAG(0x0000000000544547ULL, GET);
	CMP_TAG(0x0000000000545550ULL, PUT);
	CMP_TAG(0x0000000054534F50ULL, POST);
	CMP_TAG(0x0000000044414548ULL, HEAD);
	CMP_TAG(0x0000004843544150ULL, PATCH);
	CMP_TAG(0x0000004543415254ULL, TRACE);
	CMP_TAG(0x00004554454C4544ULL, DELETE);
	CMP_TAG(0x00534E4F4954504FULL, OPTIONS);
	CMP_TAG(0x005443454E4E4F43ULL, CONNECT);

#undef CMP_TAG

	return -1;
}

// unrolled sse2 version 2
int match_v3_11(const char * s) {
	uint32_t lo, hi;
	__m128i key, cond, cmp;

	uint64_t k = *(const uint64_t *)s; // could be removed

	lo = (uint32_t)(k);
	hi = (uint32_t)(k >> 32);
	key = _mm_set_epi32(hi, lo, hi, lo);

#define CMP_TAG(hex, tag) 					\
do { 								\
	lo = (uint32_t)(hex); 					\
	hi = (uint32_t)((hex) >> 32); 				\
	cond = _mm_set_epi32(hi, lo, hi, lo); 			\
	cmp = _mm_cmpeq_epi32(key, cond); 			\
	if (_mm_movemask_epi8(cmp) == 0xFFFF) return (tag); 	\
} while(0)

	CMP_TAG(0x0000000000544547ULL, GET);
	CMP_TAG(0x0000000000545550ULL, PUT);
	CMP_TAG(0x0000000054534F50ULL, POST);
	CMP_TAG(0x0000000044414548ULL, HEAD);
	CMP_TAG(0x0000004843544150ULL, PATCH);
	CMP_TAG(0x0000004543415254ULL, TRACE);
	CMP_TAG(0x00004554454C4544ULL, DELETE);
	CMP_TAG(0x00534E4F4954504FULL, OPTIONS);
	CMP_TAG(0x005443454E4E4F43ULL, CONNECT);

#undef CMP_TAG

	return -1;
}



#endif



#if 0
// Flat Trie
typedef struct {
    char ch;
    uint8_t is_end;   // true if this node ends a valid word
    uint8_t value;    // method ID if is_end == 1
    uint16_t next;    // offset to children
} trie_node_t;

static const trie_node_t trie[] = {
    // 0: 'C' -> CONNECT, child at 1
    { 'C', 0, 0, 1 },   // CONNECT /0
    { 'O', 0, 0, 2 },
    { 'N', 0, 0, 3 },
    { 'N', 0, 0, 4 },
    { 'E', 0, 0, 5 },
    { 'C', 0, 0, 6 },
    { 'T', 1, 8, 0 },  // CONNECT ends here (ID 8)

    // 7: 'D' -> DELETE, child at 8
    { 'D', 0, 0, 8 },
    { 'E', 0, 0, 9 },
    { 'L', 0, 0, 10 },
    { 'E', 0, 0, 11 },
    { 'T', 0, 0, 12 },
    { 'E', 1, 3, 0 },

    // 13: 'G' -> GET, child at 14
    { 'G', 0, 0, 14 },
    { 'E', 0, 0, 15 },
    { 'T', 1, 0, 0 },

    // 16: 'H' -> HEAD, child at 17
    { 'H', 0, 0, 17 },
    { 'E', 0, 0, 18 },
    { 'A', 0, 0, 19 },
    { 'D', 1, 5, 0 },

    // 20: 'O' -> OPTIONS, child at 21
    { 'O', 0, 0, 21 },
    { 'P', 0, 0, 22 },
    { 'T', 0, 0, 23 },
    { 'I', 0, 0, 24 },
    { 'O', 0, 0, 25 },
    { 'N', 0, 0, 26 },
    { 'S', 1, 6, 0 },

    // 27: 'P' -> PATCH or POST, child at 28
    { 'P', 0, 0, 28 },
    { 'A', 0, 0, 29 }, // PATCH
    { 'T', 0, 0, 30 },
    { 'C', 0, 0, 31 },
    { 'H', 1, 4, 0 },
    { 'O', 0, 0, 33 }, // POST
    { 'S', 0, 0, 34 },
    { 'T', 1, 1, 0 },

    // 35: 'T' -> TRACE, child at 36
    { 'T', 0, 0, 36 },
    { 'R', 0, 0, 37 },
    { 'A', 0, 0, 38 },
    { 'C', 0, 0, 39 },
    { 'E', 1, 7, 0 },

    // 40: 'P' -> PUT (repeated P branch for completeness)
    { 'P', 0, 0, 41 },
    { 'U', 0, 0, 42 },
    { 'T', 1, 2, 0 }
};


int match_v4(const char *s) {
    int i = 0, node = 0;

    static const int node_max = (int)(sizeof(trie) / sizeof(trie_node_t));
    while (s[i] && node < node_max) {
	if (trie[node].ch == s[i]) {
		i++;
            	if (s[i] == '\0' && trie[node].is_end) {
                	return trie[node].value;
		}

	            node = trie[node].next;
	} else {
           	node++;
       	}
    }

    return -1;
}
#endif


#if 1
#include <emmintrin.h> // SSE2

static const char _methods[9][8] = {
	"GET",
	"POST",
	"PUT",
	"DELETE",
	"PATCH",
	"HEAD",
	"OPTIONS",
	"TRACE",
	"CONNECT"
};

int match_v5(const char * s) {
	__m128i input = _mm_setzero_si128();
	__builtin_memcpy(&input, s, 8);

	for(int i = 0; i < 9; i++) {
		__m128i method = _mm_loadu_si128((const __m128i *)&_methods[i]);
		__m128i cmp = _mm_cmpeq_epi8(input, method);
		int mask = _mm_movemask_epi8(cmp);
		// full match (if shorter, refine mask logic)
		if(mask == 0xFF || (mask >> 8) == 0) {
			return i;
		}
	}

	return -1;
}
#endif



// Benchmark wrapper
void benchmark(int (*func)(const char *), const char *label) {
    struct timespec start, end;
    int matches = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NUM_INPUTS; i++) {
        // if (func(input_data[i]) != INVALID)
	 if((func(input_data[i].str) == input_data[i].val) && (input_data[i].val != -1)) {
            matches++;
	 }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    int asserted_matches = 500000;
    printf("[%s]\nMatches: %d\nOut of : %d\t", label, matches, asserted_matches);
    print_elapsed(start, end);
    printf("\n");
}

// Input generator
void generate_inputs() {
    const char *junk_words[] = {
        "FETCH", "GOT", "SEND", "RECEIVE", "UPDATE", "REMOVE", "CREATE", "DROP", "FOO", "BAR"
    };
	
    int num_junk = sizeof(junk_words) / sizeof(junk_words[0]);

    srand(42); // Deterministic

    for (int i = 0; i < NUM_INPUTS; i++) {
        if (i % 2 == 0) {
            // Copy known method
            int j = rand() % NUM_METHODS;
            strncpy(input_data[i].str, method_table[j].str, 15);
            input_data[i].str[15] = '\0';
	    input_data[i].val = method_table[j].value;
        } else {
            // Copy random junk
            int j = rand() % num_junk;
            strncpy(input_data[i].str, junk_words[j], 15);
            input_data[i].str[15] = '\0';
	    input_data[i].val = -1;
        }
    }
}

#include <inttypes.h>
void test_dump(void) {
	const uint64_t GET_v = U64_3('G', 'E', 'T');
	const uint64_t PUT_v = U64_3('P', 'U', 'T');
	const uint64_t POST_v = U64_4('P', 'O', 'S', 'T');
	const uint64_t HEAD_v = U64_4('H', 'E', 'A', 'D');
	const uint64_t PATCH_v = U64_5('P', 'A', 'T', 'C', 'H');
	const uint64_t TRACE_v = U64_5('T', 'R', 'A', 'C', 'E');
	const uint64_t DELETE_v = U64_6('D', 'E', 'L', 'E', 'T', 'E');
	const uint64_t OPTIONS_v = U64_7('O', 'P', 'T', 'I', 'O', 'N', 'S');
	const uint64_t CONNECT_v = U64_7('C', 'O', 'N', 'N', 'E', 'C', 'T');

	printf("### 64-bit strings (LE) ###\n");
	printf("GET    : 0x%.16lX\n", GET_v);
	printf("PUT    : 0x%.16lX\n", PUT_v);
	printf("POST   : 0x%.16lX\n", POST_v);
	printf("HEAD   : 0x%.16lX\n", HEAD_v);
	printf("PATCH  : 0x%.16lX\n", PATCH_v);
	printf("TRACE  : 0x%.16lX\n", TRACE_v);
	printf("DELETE : 0x%.16lX\n", DELETE_v);
	printf("OPTIONS: 0x%.16lX\n", OPTIONS_v);
	printf("CONNECT: 0x%.16lX\n", CONNECT_v);
	printf("\n");

	// printf("GET: 0x%016" PRIX64 "\n", U64_3('G', 'E', 'T'));
}

int main(void) {
    generate_inputs();

    benchmark(match_v1, "strcmp");
    benchmark(match_v2, "strncmp + len");
    benchmark(match_v3, "switch (strcmp)");
    benchmark(match_v3_2, "switch (memcmp)");
    benchmark(match_v3_3, "switch + if-N");
    benchmark(match_v3_4, "unsigned integer comparison");
    benchmark(match_v3_5, "switch, unsigned integer + mask index");
    benchmark(match_v3_6, "switch, uint64_t + mask?");
    benchmark(match_v3_7, "branchless uint64 + mask");
    benchmark(match_v3_8, "branchless uint64 + define");
    benchmark(match_v3_9, "sse2 + lut");
    benchmark(match_v3_10, "sse2 + unrolled");
    benchmark(match_v3_11, "sse2 + unrolled + reduced");
    // benchmark(match_v4, "trie");
    benchmark(match_v5, "simd");

    // test_dump();

    return 0;
}

