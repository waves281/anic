#ifndef _GENNER_H_
#define _GENNER_H_

#include "globalDefs.h"
#include "constantDefs.h"
#include "driver.h"

#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "semmer.h"

// CodeTree category specifiers
#define CATEGORY_LABEL 0
#define CATEGORY_CONST 1
#define CATEGORY_TEMP 2
#define CATEGORY_UNOP 3
#define CATEGORY_BINOP 4
#define CATEGORY_RESERVE 5
#define CATEGORY_UNRESERVE 6
#define CATEGORY_COPY 7
#define CATEGORY_FLOW 8

// forward declarations
class CodeTree;

// core CodeTree class

class CodeTree {
	public:
		// data members
		int category; // the category that this CodeTree belongs to
		// allocators/deallocators
		virtual ~CodeTree() = 0;
		// operators
		virtual operator string() = 0;
};

// main code generation function

int gen(Tree *treeRoot, SymbolTable *stRoot, CodeTree *&codeRoot);

#endif
