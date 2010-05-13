#ifndef _LEXER_H_
#define _LEXER_H_

#include "mainDefs.h"
#include "constantDefs.h"
#include "system.h"

#include "../tmp/lexerStruct.h"

class Token {
	public:
		// data members
		int tokenType;
		string s;
		string fileName;
		int row;
		int col;
		// allocators/deallocators
		Token(int tokenType = TOKEN_STD);
		Token(int tokenType, string &s, string &fileName, int row, int col);
		Token(const Token &otherToken);
		~Token();
		// operators
		Token &operator=(Token &otherToken);
};

vector<Token> *lex(ifstream *in, string &fileName);

// post-includes

#include "../tmp/lexerStruct.h"

#endif
