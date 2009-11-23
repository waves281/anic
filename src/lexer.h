#ifndef _LEXER_STRUCT_H_
#define _LEXER_STRUCT_H_

#include "constantDefs.h"

struct lexerNodeStruct {
	char *tokenType;
	int toState;
};
typedef struct lexerNodeStruct LexerNode;

struct tokenListStruct {
	char *tokenType;
	char s;
	struct tokenListStruct *next;
};
typedef struct tokenListStruct TokenList;

TokenList *lex(ifstream *in);

#endif
