#include "customOperators.h"

// operator definitions

ostream &operator<< (ostream &os, Token &t) {
	os << "[" << tokenType2String(t.tokenType) << ' ' << t.s << " (" << t.row << ',' << t.col << ")]";
	return os;
}

void printDefs(SymbolTable *st, unsigned int depth) {
	if (st == NULL) {
		return;
	}
	cout << '\t';
	for (unsigned int i = 0; i < depth; i++) {
		if (i < (depth-1)) {
			cout << '|';
		} else {
			cout << '-';
		}
	}
	if (st->id != BLOCK_NODE_STRING) {
		cout << st->id;
		Type *defType = st->defSite->status.type;
		if (defType != NULL) {
			cout << " : " << defType;
		}
	} else {
		cout << "{}";
	}
	cout << '\n';
	for (vector<SymbolTable *>::iterator childIter = st->children.begin(); childIter != st->children.end(); childIter++) {
		printDefs(*childIter, depth+1);
	}
}

ostream &operator<< (ostream &os, SymbolTable *&st) {
	printDefs(st, 1);
	return os;
}

ostream &operator<< (ostream &os, Type *type) {
	os << ((string)(*type));
	return os;
}

ostream &operator<< (ostream &os, const TypeStatus &status) {
	os << ((string)(*(status.type)));
	return os;
}
