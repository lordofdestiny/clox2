#ifndef __CLOX2_TOKEN_H__
#define __CLOX2_TOKEN_H__

typedef struct {
	int line;
	int column;
} TokenLocation;

typedef struct {
	char* chars;
	int length;
} Token;

#endif // __CLOX2_TOKEN_H__
