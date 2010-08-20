#include "outputOperators.h"

// operator definitions

ostream &operator<< (ostream &os, Token &t) {
	os << "[" << tokenType2String(t.tokenType) << ' ' << t.s << " (" << t.row << ',' << t.col << ")]";
	return os;
}

ostream &operator<< (ostream &os, Tree *&tree) {
	os << ((string)(*tree));
	return os;
}

ostream &operator<< (ostream &os, SymbolTree *st) {
	os << ((string)(*st));
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
