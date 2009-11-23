#ifndef _LEXER_STRUCT_H_
#define _LEXER_STRUCT_H_

#include "constantDefs.h"

struct lexerNodeStruct {
	char *tokenType;
	int toState;
};
typedef struct lexerNodeStruct LexerNode;

#endif
