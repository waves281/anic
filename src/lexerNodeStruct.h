#ifndef LEXER_STRUCT_NODE_H
#define LEXER_STRUCT_NODE_H

struct lexerNodeStruct {
	bool valid;
	int tokenType;
	int toState;
};
typedef struct lexerNodeStruct LexerNode;

#endif
