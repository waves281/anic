#ifndef _SEMMER_H_
#define _SEMMER_H_

#include "mainDefs.h"
#include "constantDefs.h"

#include "lexer.h"
#include "../var/lexerStruct.h"

#include "parser.h"
#include "../var/parserStruct.h"

class SymbolTable {
	public:
		// data members
		string id;
		Tree *def; // NULL means standard
		SymbolTable *parent;
		vector<SymbolTable *> children;
		// allocators/deallocators
		SymbolTable(string id, Tree *def, SymbolTable *parent);
		// concatenators
		SymbolTable &operator*=(SymbolTable *st);
};

SymbolTable *sem(Tree *rootParseme, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp);

#endif
