#ifndef _LEXER_H_
#define _LEXER_H_

#include "constantDefs.h"

struct lexerNodeStruct {
	int valid;
	char *tokenType;
	int toState;
};
typedef struct lexerNodeStruct LexerNode;

struct tokenStruct {
	char *tokenType;
	char s[MAX_TOKEN_LENGTH];
	int row;
	int col;
};
typedef struct tokenStruct Token;

class LexerError {
	private:
		char *e;
		char faultChar;
		int row;
		int col;
	public:
		LexerError(char *e, char faultChar, int row, int col) {
			this->e = e;
		}
		char *getError() {
			return e;
		}
		char getFaultChar() {
			return faultChar;
		}
		int getRow() {
			return row;
		}
		int getCol() {
			return col;
		}
};

vector<Token> *lex(ifstream *in, char *fileName);

#endif
