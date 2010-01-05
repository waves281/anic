#ifndef _LEXER_H_
#define _LEXER_H_

#include "mainDefs.h"
#include "constantDefs.h"
#include "system.h"

struct lexerNodeStruct {
	int valid;
	int tokenType;
	int toState;
};
typedef struct lexerNodeStruct LexerNode;

class Token {
	public:
		int tokenType;
		string s;
		string fileName;
		int row;
		int col;
};

vector<Token> *lex(ifstream *in, const char *fileName, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp);

// post-includes

#include "../var/lexerStruct.h"

#endif
