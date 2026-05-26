#ifndef __CLOX2_TOKEN_H__
#define __CLOX2_TOKEN_H__

#include <string.h>

#include <common/arena.h>

typedef struct {
	int line;
	int column;
} TokenLocation;

typedef struct {
	int length;
	char chars[];
} Token;

static Token* makeToken(arena_t* arena, const char* text, int length) {
	if (arena == NULL || text == NULL || length <= 0) {
		return NULL;
	}
	Token* token = arena_alloc(arena, sizeof(*token) + length + 1);
	if (token == NULL) {
		return NULL;
	}
	token->length = length;
	strncpy(token->chars, text, length);
	token->chars[length] = '\0';
	return token;
}

#endif // __CLOX2_TOKEN_H__
