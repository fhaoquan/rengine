/*
 * Disclaimer: I'm not really making an effort at this point in time
 * to be fully compliant with the JSON specs.
 * For now I just need my own tools to work.
 * 
 * TODO: Run through Valgrind.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "json.h"
#include "lexer.h"
#include "hash.h"
#include "utils.h"

char *json_escape(const char *in, char *out, size_t len) {
	size_t i = 0;
	assert(len > 0);
	while(in[0]) {
		if(i == len - 1) {
			out[i - 1] = 0;
			return &out[i - 1];
		} else {
			
			switch(in[0]) {
				case '\"':
				case '\\':
				case '/':
				case '\b':
				case '\f':
				case '\n':
				case '\r':
				case '\t': {					
					out[i++] = '\\';
					if(i == len - 1) {
						out[i - 1] = 0;
						return &out[i - 1];
					}
					
					switch(in[0]) {
						case '\"': out[i++] = '"'; break;
						case '\\': out[i++] = '\\'; break;
						case '/': out[i++] = '/'; break;
						case '\b': out[i++] = 'b'; break;
						case '\f': out[i++] = 'f'; break;
						case '\n': out[i++] = 'n'; break;
						case '\r': out[i++] = 'r'; break;
						case '\t': out[i++] = 't'; break;
					}
					
				} break;
				/* FIXME: \uXXXX cases for unicode characters */
				default: {
					out[i++] = in[0];
				} break;
			}
			
		}
		in++;
	}
	out[i] = '\0';
	return out;
}

struct json_value *json_read(const char *filename) {
	struct json_value *v = NULL;
	char *text = my_readfile(filename);	
	if(!text)
		return NULL;
	v = json_parse(text);
	free(text);
	return v;
}

static void json_free_member(const char *key, void *val) {
	json_free(val);
}

void json_free(struct json_value *v) {
	switch(v->type) {
		case j_string:
		case j_number: free(v->value); break;
		case j_object: {
			struct hash_tbl *h = v->value;
			 ht_free(h, json_free_member);
		} break;
		case j_array: {
			/* FIXME: */
		}
		default: break;
	}
	free(v);
}

#define J_TRUE  256
#define J_FALSE 257
#define J_NULL  258

static struct json_value *json_parse_value(struct lexer *lx);
static struct json_value *json_parse_array(struct lexer *lx);
static struct json_value *json_parse_object(struct lexer *lx);

static struct lx_keywords json_keywds[] = {
	{"true", J_TRUE},
	{"false", J_FALSE},
	{"null", J_NULL},
	{NULL, 0}};

struct json_value *json_parse(const char *text) {
	struct lexer * lx = lx_create(text, "{}[]-:,", json_keywds);
	struct json_value *v = json_parse_object(lx);
	lx_free(lx);
	return v;
}

static struct json_value *json_parse_object(struct lexer *lx) {
	
	struct hash_tbl *h = ht_create (16);
	struct json_value *v = malloc(sizeof *v);
	v->type = j_object;
	v->value = h;
	v->next = NULL;
	
	if(!lx_expect(lx, '{')) {
		fprintf(stderr, "error:%d: %s\n", lx_lineno(lx), lx_text(lx));
		return NULL;
	}
	do {
		char *key;
		struct json_value *value;
		lx_accept(lx, LX_STRING);
		key = strdup(lx_text(lx));
		lx_accept(lx, ':');		
		value = json_parse_value(lx);
		if(!value)
			return NULL;
		
		ht_insert (h, key, value);
		
		free(key);	
		
	} while(lx_accept(lx, ','));
	if(!lx_expect(lx, '}')) {
		fprintf(stderr, "error:%d: %s\n", lx_lineno(lx), lx_text(lx));
		return NULL;
	}
		
	return v;
}

static struct json_value *json_parse_array(struct lexer *lx) {
	
	struct json_value *v = malloc(sizeof *v);
	v->type = j_array;
	v->value = NULL;
	v->next = NULL;
	
	struct json_value *tail = NULL;
	
	if(!lx_expect(lx, '[')) {
		fprintf(stderr, "error:%d: %s\n", lx_lineno(lx), lx_text(lx));
		return NULL;
	}
	do {		
		struct json_value *value = json_parse_value(lx);
		if(!value) 
			return NULL;
		if(v->value) {
			assert(tail);
			tail->next = value;			
		} else {
			v->value = value;
		}	
		tail = value;
		
	} while(lx_accept(lx, ','));
	if(!lx_expect(lx, ']')) {
		fprintf(stderr, "error:%d: %s\n", lx_lineno(lx), lx_text(lx));
		return NULL;
	}
	
	return v;
}

static struct json_value *json_parse_value(struct lexer *lx) {
	if(lx_sym(lx) == '{')
		return json_parse_object(lx);	
	else if(lx_sym(lx) == '[')
		return json_parse_array(lx);
	else {
		struct json_value *v = malloc(sizeof *v);
		v->type = j_null;
		v->value = NULL;
		v->next = NULL;
		if(lx_sym(lx) == LX_NUMBER) {
			v->type = j_number;
			v->value = strdup(lx_text(lx));
		} else if(lx_sym(lx) == LX_STRING) {
			v->type = j_string;
			v->value = strdup(lx_text(lx));
		} else if(lx_sym(lx) == J_TRUE) {
			v->type = j_true;
		} else if(lx_sym(lx) == J_FALSE) {
			v->type = j_false;
		}else if(lx_sym(lx) == J_NULL) {
			v->type = j_null;
		} else {
			if(isprint(lx_sym(lx))) {
				printf("%3d: OPERATOR: %c\n", lx_lineno(lx), lx_sym(lx));
			} else {
				fprintf(stderr, "error:%d: Unexpected symbol type %d\n", lx_lineno(lx), lx_sym(lx));
			}
			return NULL;
		}
		lx_getsym(lx);	
		
		return v;
	}
}

static void json_dump_L(struct json_value *v, int L) {
	char buffer[256];
	switch(v->type) {
		case j_string: {
			printf("\"%s\"", json_escape(v->value, buffer, sizeof buffer));
		} break;
		case j_number: printf("%s", (const char *)v->value); break;
		case j_object: {
			struct hash_tbl *h = v->value;
			const char *key = ht_next (h, NULL);			
			puts("{");
			while(key) {
				printf("%*c\"%s\" : ", L*2, ' ', json_escape(key, buffer, sizeof buffer));
				json_dump_L(ht_find(h, key), L + 1);
				key = ht_next(h, key);
				if(key) 
					puts(",");
			}
			printf("}");
		} break;
		case j_array: {
			struct json_value *m = v->value;
			puts("[");
			while(m) {
				printf("%*c", L*2, ' ');
				json_dump_L(m, L + 1);
				m = m->next;
				if(m)
					puts(",");
			}
			printf("]");
		} break;
		case j_true: {
			printf("true");
		} break;
		case j_false: {
			printf("false");
		} break;
		case j_null: {
			printf("null");
		} break;
		default: break;
	}
}

void json_dump(struct json_value *v) {
	json_dump_L(v, 0);
}

/*
gcc -o json -DJTEST ../json.c ../lexer.c ../hash.c ../utils.c
*/
#ifdef JTEST
int main(int argc, char *argv[]) {
	struct json_value *j;
	if(argc < 2) 
		return EXIT_FAILURE;
	j = json_read(argv[1]);
	if(!j) {
		fprintf(stderr, "Unable to parse %s\n", argv[1]);
		return EXIT_FAILURE;
	} else {
		json_dump(j);
		json_free(j);
		return EXIT_SUCCESS;
	}
}
#endif