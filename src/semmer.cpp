#include "mainDefs.h"
#include "system.h"

#include "semmer.h"

// symbol table functions

// allocators/deallocators
SymbolTable::SymbolTable(string id, Tree *def) : id(id), defSite(defSite) {}

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
void getUserDefs(Tree *parseme, SymbolTable *st) {
	// base case
	if (parseme == NULL) {
		return;
	}
	// recursive cases
	if (parseme->t.tokenType == TOKEN_Declaration) { // if it's a declaration node
		Token t = parseme->child->next->t;
		if (t.tokenType == TOKEN_EQUALS) { // standard declaration

			// recurse
			getUserDefs(parseme->child->next->next->child, st); // child of StaticTerm
		} else if (t.tokenType == TOKEN_ERARROW) {

			// recurse
			getUserDefs(parseme->child->next->next->child, st); // child of NonEmptyTerms
		} else if (t.tokenType == TOKEN_QualifiedIdentifier) {

			// don't recurse in this case, since there's nowhere deeper to go
		}
	} else { // else if it's not a declaration node, recurse normally
		getUserDefs(parseme->child, st); // down
		getUserDefs(parseme->next, st); // right
	}
}

int sem(Tree *rootParseme, SymbolTable *&stRoot, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp) {
	// local error code
	int semmerErrorCode = 0;

	// populate the symbol table with definitions in the user parseme
	getUserDefs(rootParseme, stRoot);

	// bind identifier use sites to their definitions


	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
