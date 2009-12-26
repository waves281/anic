#include "mainDefs.h"
#include "system.h"
#include "customOperators.h"

#include "semmer.h"

// semmer-global variables

int semmerErrorCode;
bool semmerEventuallyGiveUp;

// symbol table functions

// allocators/deallocators
SymbolTable::SymbolTable(string id, Tree *defSite) : id(id), defSite(defSite), parent(NULL) {}

// concatenators
SymbolTable &SymbolTable::operator*=(SymbolTable *st) {
	// first, check for conflicting bindings
	if (st != NULL && st->id[0] != '_') { // if this is not a special system-level binding
		// per-symbol loop
		for (vector<SymbolTable *>::iterator childIter = children.begin(); childIter != children.end(); childIter++) {
			if ((*childIter)->id == st->id) { // if we've found a conflict
				Token curDefToken;
				if (st->defSite != NULL) { // if there is a definition site for the current symbol
					curDefToken = st->defSite->t;
				} else { // otherwise, it must be a standard definition, so make up the token as if it was
					curDefToken.fileName = STANDARD_LIBRARY_STRING;
					curDefToken.row = 0;
					curDefToken.col = 0;
				}
				Token prevDefToken;
				if ((*childIter)->defSite != NULL) { // if there is a definition site for the previous symbol
					prevDefToken = (*childIter)->defSite->t;
				} else { // otherwise, it must be a standard definition, so make up the token as if it was
					prevDefToken.fileName = STANDARD_LIBRARY_STRING;
					prevDefToken.row = 0;
					prevDefToken.col = 0;
				}
				printSemmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"redefinition of '"<<st->id<<"'",*this);
				printSemmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)",*this);
				return *this;
			} // if there's a conflict
		} // for per-symbol loop
	} // if this is not a special system-level binding

	// binding is now known to be conflict-free, so log it normally
	children.push_back(st);
	if (st != NULL) {
		st->parent = this;
		return *st;
	} else {
		return *this;
	}
}

// main semantic analysis functions

void catStdTypes(SymbolTable *&stRoot) {
	*stRoot *= new SymbolTable("node", NULL);
	*stRoot *= new SymbolTable("byte", NULL);
	*stRoot *= new SymbolTable("int", NULL);
	*stRoot *= new SymbolTable("float", NULL);
	*stRoot *= new SymbolTable("bool", NULL);
	*stRoot *= new SymbolTable("char", NULL);
	*stRoot *= new SymbolTable("string", NULL);
}

void catStdLib(SymbolTable *&stRoot) {
	// standard root
	SymbolTable *stdLib = new SymbolTable(STANDARD_LIBRARY_STRING, NULL);
	// standard streams
	*stdLib *= new SymbolTable("in", NULL);
	*stdLib *= new SymbolTable("out", NULL);
	*stdLib *= new SymbolTable("err", NULL);
	// standard library
	// standard containers
	*stdLib *= new SymbolTable("stack", NULL);
	*stdLib *= new SymbolTable("map", NULL);
	// standard filters
	*stdLib *= new SymbolTable("filter", NULL);
	*stdLib *= new SymbolTable("sort", NULL);
	// standard generators
	*stdLib *= new SymbolTable("gen", NULL);
	// concatenate the library to the root
	*stRoot *= stdLib;
}

SymbolTable *genDefaultDefs() {
	// generate the root block node
	SymbolTable *stRoot = new SymbolTable(BLOCK_NODE_STRING, NULL);
	// concatenate in the standard types
	catStdTypes(stRoot);
	// concatenate in the standard library
	catStdLib(stRoot);
	// finally, return the genrated default symtable
	return stRoot;
}

// populates the SymbolTable by recursively scanning the given parseme for Declaration nodes
void getUserIdentifiers(Tree *parseme, SymbolTable *st, vector<SymbolTable *> &importList, vector<Tree *> &instanceList) {
	// base case
	if (parseme == NULL) {
		return;
	}
	// log the current symbol environment in the parseme
	parseme->env = st;
	// recursive cases
	// if it's a non-import QualifiedIdentifier
	if (parseme->t.tokenType == TOKEN_QualifiedIdentifier && !(parseme->back != NULL && parseme->back->t.tokenType == TOKEN_AT)) {
		// log this identifier use case
		instanceList.push_back(parseme);
		// *don't* recurse any deeper in this QualifiedIdentifier
	} else if (parseme->t.tokenType == TOKEN_Block) { // if it's a block node
		// allocate the new definition node
		SymbolTable *newDef = new SymbolTable(BLOCK_NODE_STRING, parseme);
		// ... and link it in
		*st *= newDef;
		// recurse
		getUserIdentifiers(parseme->child, newDef, importList, instanceList); // child of Block
	} else if (parseme->t.tokenType == TOKEN_Declaration) { // if it's a declaration node
		Token t = parseme->child->next->t;
		if (t.tokenType == TOKEN_EQUALS) { // standard declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(parseme->child->t.s, parseme);
			// ... and link it in
			*st *= newDef;
			// recurse
			getUserIdentifiers(parseme->child, newDef, importList, instanceList); // child of Declaration
		} else if (t.tokenType == TOKEN_ERARROW) { // flow-through declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(parseme->child->t.s, parseme);
			// ... and link it in
			*st *= newDef;
			// recurse
			getUserIdentifiers(parseme->child, newDef, importList, instanceList); // child of Declaration
		} else if (t.tokenType == TOKEN_QualifiedIdentifier) { // import declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(qi2String(parseme->child->next), parseme);
			// ... and link it in
			*st *= newDef;
			// also, since it's an import declaration, log it to the import list
			importList.push_back(newDef);
			// recurse
			getUserIdentifiers(parseme->child, newDef, importList, instanceList); // child of Declaration
		}
	} else { // else if it's not a declaration node
		// recurse normally
		getUserIdentifiers(parseme->child, st, importList, instanceList); // down
		getUserIdentifiers(parseme->next, st, importList, instanceList); // right
	}
}

void subImportDecls(SymbolTable *stRoot, vector<SymbolTable *> importList) {

}

// main semming function; makes no assumptions about stRoot's value; it's just a return parameter
int sem(Tree *rootParseme, SymbolTable *&stRoot, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp) {

	// initialize error code
	semmerErrorCode = 0;
	semmerEventuallyGiveUp = eventuallyGiveUp;

	// initialize the symbol table root with the default definitions
	stRoot = genDefaultDefs();

	// populate the symbol table with definitions from the user parseme, and log the used imports/id instances
	vector<SymbolTable *> importList;
	vector<Tree *> instanceList;
	getUserIdentifiers(rootParseme, stRoot, importList, instanceList);

	// substitute import declarations
	subImportDecls(stRoot, importList);

	VERBOSE( cout << stRoot; )

	// bind identifier use sites to their definitions, checking for errors


	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
