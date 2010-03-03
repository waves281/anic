#ifndef _LEXER_H_
#define _LEXER_H_

#include "mainDefs.h"
#include "constantDefs.h"
#include "system.h"

#include "../tmp/lexerStruct.h"

class Token {
	public:
		int tokenType;
		string s;
		string fileName;
		int row;
		int col;
};

vector<Token> *lex(ifstream *in, const char *fileName);

// post-includes

#include "../tmp/lexerStruct.h"

#endif
