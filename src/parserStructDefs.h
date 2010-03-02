#ifndef PARSER_STRUCT_DEFS_H
#define PARSER_STRUCT_DEFS_H

// action definitions
#define ACTION_SHIFT 1
#define ACTION_REDUCE 2
#define ACTION_ACCEPT 3
#define ACTION_GOTO 4
#define ACTION_ERROR 5

struct parserNodeStruct {
	int action; // the action to take in this situation (ACTION_ defines above)
	unsigned int n; // either the state to go to (SHIFT/GOTO) or the rule to reduce by (REDUCE)
};
typedef struct parserNodeStruct ParserNode;

#endif
