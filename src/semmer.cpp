#include "mainDefs.h"
#include "system.h"

#include "semmer.h"

// symbol table functions

// allocators/deallocators
SymbolTable::SymbolTable(string id, Tree *def) : id(id), def(def) {}

// concatenators
SymbolTable &SymbolTable::operator*=(SymbolTable *st) {
	children.push_back(st);
	if (st != NULL) {
		st->parent = this;
		return *st;
	} else {
		return *this;
	}
}

// main semantic analysis functions

SymbolTable *genStdDefs() {
	// standard root
	SymbolTable *retVal = new SymbolTable(STANDARD_LIBRARY_PREFIX, NULL);
	// standard types
	*retVal *= new SymbolTable("node", NULL);
	*retVal *= new SymbolTable("byte", NULL);
	*retVal *= new SymbolTable("int", NULL);
	*retVal *= new SymbolTable("float", NULL);
	*retVal *= new SymbolTable("bool", NULL);
	*retVal *= new SymbolTable("char", NULL);
	*retVal *= new SymbolTable("string", NULL);
	// standard streams
	*retVal *= new SymbolTable("in", NULL);
	*retVal *= new SymbolTable("out", NULL);
	*retVal *= new SymbolTable("err", NULL);
	// standard library
	// standard containers
	*retVal *= new SymbolTable("stack", NULL);
	*retVal *= new SymbolTable("map", NULL);
	// standard filters
	*retVal *= new SymbolTable("filter", NULL);
	*retVal *= new SymbolTable("sort", NULL);
	// standard generators
	*retVal *= new SymbolTable("gen", NULL);
	// return the compiled standard list to the caller
	return retVal;
}

// populates the SymbolTable by recursively scanning the given parseme for Declaration nodes
void populateDefs(Tree *parseme, SymbolTable *st) {
	if (parseme->t.tokenType == TOKEN_Declaration) {

	}
}

int sem(Tree *rootParseme, SymbolTable *&stRoot, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp) {
	// local error code
	int semmerErrorCode = 0;

	// populate the symbol table with definitions in the user parseme
	populateDefs(rootParseme, stRoot);


	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
