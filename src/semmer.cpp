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
void getUserDefs(Tree *parseme, SymbolTable *st, vector<SymbolTable *> &importList) {
	// base case
	if (parseme == NULL) {
		return;
	}
	// log the current symbol environment in the parseme
	parseme->env = st;
	// recursive cases
	if (parseme->t.tokenType == TOKEN_Block) { // if it's a block node
		// allocate the new definition node
		SymbolTable *newDef = new SymbolTable(BLOCK_NODE_STRING, parseme);
		// ... and link it in
		*st *= newDef;
		// recurse
		getUserDefs(parseme->child, newDef, importList); // child of Block
	} else if (parseme->t.tokenType == TOKEN_Declaration) { // if it's a declaration node
		Token t = parseme->child->next->t;
		if (t.tokenType == TOKEN_EQUALS) { // standard declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(parseme->child->t.s, parseme);
			// ... and link it in
			*st *= newDef;
			// recurse
			getUserDefs(parseme->child, newDef, importList); // child of Declaration
		} else if (t.tokenType == TOKEN_ERARROW) { // flow-through declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(parseme->child->t.s, parseme);
			// ... and link it in
			*st *= newDef;
			// recurse
			getUserDefs(parseme->child, newDef, importList); // child of Declaration
		} else if (t.tokenType == TOKEN_QualifiedIdentifier) { // import declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(qi2String(parseme->child->next), parseme);
			// ... and link it in
			*st *= newDef;
			// also, since it's an import declaration, log it to the import list
			importList.push_back(newDef);
			// don't recurse in this case, since there's nowhere deeper to go
		}
	} else { // else if it's not a declaration node
		// recurse normally
		getUserDefs(parseme->child, st, importList); // down
		getUserDefs(parseme->next, st, importList); // right
	}
}

void printDefs(SymbolTable *&st, unsigned int depth) {
	if (st == NULL) {
		return;
	}
	cout << "\t";
	for (unsigned int i = 0; i < depth; i++) {
		if (i < (depth-1)) {
			cout << "|";
		} else {
			cout << "-";
		}
	}
	if (st->id != BLOCK_NODE_STRING) {
		cout << st->id;
	} else {
		cout << "{}";
	}
	cout << "\n";
	for (vector<SymbolTable *>::iterator childIter = st->children.begin(); childIter != st->children.end(); childIter++) {
		printDefs(*childIter, depth+1);
	}
}

void printDefs(SymbolTable *&st) {
	printDefs(st, 1);
}

// main semming function; makes no assumptions about stRoot's value; it's just a return parameter
int sem(Tree *rootParseme, SymbolTable *&stRoot, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp) {
	// local error code
	int semmerErrorCode = 0;

	// initialize the symbol table root with a global block node
	stRoot = genDefaultDefs();

	// populate the symbol table with definitions from the user parseme
	vector<SymbolTable *> importList;
	getUserDefs(rootParseme, stRoot, importList);

	VERBOSE( printDefs(stRoot); )

	// bind identifier use sites to their definitions


	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
