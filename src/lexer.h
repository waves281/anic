#ifndef _LEXER_H_
#define _LEXER_H_

#include "customOperators.h"
#include "constantDefs.h"

struct lexerNodeStruct {
	int valid;
	char *tokenType;
	int toState;
};
typedef struct lexerNodeStruct LexerNode;

class Token {
	public:
		char *tokenType;
		vector<char> s;
		int row;
		int col;
};

vector<Token> *lex(ifstream *in, char *fileName);

#endif
