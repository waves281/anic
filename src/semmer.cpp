#include "semmer.h"

#include "customOperators.h"

// semmer-global variables

int semmerErrorCode;
bool semmerEventuallyGiveUp;

Type *nullType = new StdType(STD_NULL);
Type *errType = new ErrorType();

// SymbolTable functions

// allocators/deallocators
SymbolTable::SymbolTable(int kind, string id, Tree *defSite) : kind(kind), id(id), defSite(defSite), parent(NULL) {}

SymbolTable::SymbolTable(SymbolTable &st) {
	*this = st;
}

SymbolTable::~SymbolTable() {
	// delete all of the child nodes
	for (vector<SymbolTable *>::iterator childIter = children.begin(); childIter != children.end(); childIter++) {
		delete *childIter;
	}
}

// deep-copy assignment operator
SymbolTable &SymbolTable::operator=(SymbolTable &st) {
	kind = st.kind;
	id = st.id;
	defSite = st.defSite;
	parent = st.parent;
	for (vector<SymbolTable *>::iterator childIter = st.children.begin(); childIter != st.children.end(); childIter++) {
		// copy the child node
		SymbolTable *child = new SymbolTable(**childIter);
		// fix the child's parent pointer to point up to this node
		child->parent = this;
		// finally, log the child in the copied child into the children list
		children.push_back(child);
	}
	return *this;
}

// concatenators
SymbolTable &SymbolTable::operator*=(SymbolTable *st) {
	// first, check for conflicting bindings
	if (st != NULL && st->id[0] != '_' && st->id[0] != '=') { // if this is not a special system-level binding
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
				semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"redefinition of '"<<st->id<<"'");
				semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
				delete st;
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

// Type functions
bool Type::baseEquals(Type &otherType) {return (suffix == otherType.suffix && suffix == otherType.suffix);}
bool Type::baseSendable(Type &otherType) { return (suffix == SUFFIX_LATCH && (otherType.suffix == SUFFIX_LATCH || otherType.suffix == SUFFIX_STREAM));}
Type::~Type() {}
void Type::delatch() {
	if (suffix == SUFFIX_LATCH) {
		suffix = SUFFIX_CONSTANT;
	} else if (suffix == SUFFIX_STREAM) {
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_LATCH;
		}
	} else if (suffix == SUFFIX_ARRAY) {
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_CONSTANT;
		}
	}
}
Type::operator bool() {return (category != CATEGORY_ERRORTYPE);}
bool Type::operator!() {return (category == CATEGORY_ERRORTYPE);}
bool Type::operator!=(Type &otherType) {return (!operator==(otherType));};

// TypeList functions
// constructor works on ParamList and TypeList
TypeList::TypeList(Tree *tree, Tree *&recall) {
	category = CATEGORY_TYPELIST;
	Tree *treeCur = tree;
	for(;;) { // invariant: treeCur is either ParamList or TypeList
		Tree *base; // NonArraySuffixedIdentifier or FilterType
		if (*treeCur == TOKEN_ParamList) { // if we're working with a ParamList
			base = treeCur->child->child->child; // NonArraySuffixedIdentifier or FilterType
		} else { // otherwise we're working with a TypeList
			base = treeCur->child->child; // NonArraySuffixedIdentifier or FilterType
		}
		Tree *typeSuffix = base->next; // TypeSuffix
		// derive the suffix and depth first, since it's more convenient to do so
		int suffixVal;
		int depthVal = 0;
		if (typeSuffix->child == NULL) {
			suffixVal = SUFFIX_CONSTANT;
		} else if (*(typeSuffix->child) == TOKEN_SLASH) {
			suffixVal = SUFFIX_LATCH;
		} else if (*(typeSuffix->child) == TOKEN_StreamTypeSuffix) {
			suffixVal = SUFFIX_STREAM;
			Tree *sts = typeSuffix->child; // StreamTypeSuffix
			for(;;) {
				depthVal++;
				// advance
				if (sts->child->next != NULL) {
					sts = sts->child->next; // StreamTypeSuffix
				} else {
					break;
				}
			}
		} else if (*(typeSuffix->child) == TOKEN_ArrayTypeSuffix) {
			suffixVal = SUFFIX_ARRAY;
			Tree *ats = typeSuffix->child; // ArrayTypeSuffix
			for(;;) {
				depthVal++;
				// advance
				if (ats->child->next != NULL) {
					ats = ats->child->next; // ArrayTypeSuffix
				} else {
					break;
				}
			}
		}
		// construct the type
		Type *curType;
		if (*base == TOKEN_FilterType) { // if it's a regular filter type
			TypeList *from;
			TypeList *to;
			Tree *baseCur = base->child->next; // TypeList or RetList
			if (*baseCur == TOKEN_TypeList) {
				from = new TypeList(baseCur, recall); // TypeList
				// advance
				baseCur = baseCur->next; // RetList or RSQUARE
			} else {
				from = new TypeList();
			}
			if (*baseCur == TOKEN_RetList) { // yes, if, as opposed to else if (see above)
				to = new TypeList(baseCur->child->next, recall); // TypeList
			} else {
				to = new TypeList();
			}
			curType = new FilterType(from, to, suffixVal, depthVal);
		} else if (*base == TOKEN_NonArraySuffixedIdentifier) { // if it's an identifier (object) type
			curType = getTypeSuffixedIdentifier(base);
		}
		// commit the type to the list
		list.push_back(curType);
		// advance
		if (treeCur->child->next != NULL) {
			treeCur = treeCur->child->next->next;
		} else {
			break;
		}
	}
}
TypeList::TypeList(Type *type) {
	category = CATEGORY_TYPELIST;
	list.push_back(type);
}
TypeList::TypeList() {category = CATEGORY_TYPELIST;}
TypeList::~TypeList() {
	for (vector<Type *>::iterator iter = list.begin(); iter != list.end(); iter++) {
		if (**iter != *nullType && **iter != *errType) {
			delete (*iter);
		}
	}
}
bool TypeList::isComparable() {return (list.size() == 1 && (list[0])->isComparable());}
bool TypeList::operator==(Type &otherType) {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (list.size() != otherTypeCast->list.size()) {
			return false;
		}
		vector<Type *>::iterator iter1 = list.begin();
		vector<Type *>::iterator iter2 = otherTypeCast->list.begin();
		while (iter1 != list.end() && iter2 != otherTypeCast->list.end()) {
			if (**iter1 != **iter2) {
				return false;
			}
			iter1++;
			iter2++;
		}
		return true;
	} else {
		return false;
	}
}
Type &TypeList::operator>>(Type &otherType) {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (list.size() != otherTypeCast->list.size()) {
			return *errType;
		}
		vector<Type *>::iterator iter1 = list.begin();
		vector<Type *>::iterator iter2 = otherTypeCast->list.begin();
		while (iter1 != list.end() && iter2 != otherTypeCast->list.end()) {
			if (! (**iter1 >> **iter2)) {
				return *errType;
			}
			iter1++;
			iter2++;
		}
		return *nullType;
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return *errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (*this >> *(otherTypeCast->from)) {
			return *(otherTypeCast->to);
		} else {
			return *errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*this >> **iter) {
				return otherType;
			}
		}
		return *errType;
	} else if (otherType.category == CATEGORY_ERRORTYPE) {
		return otherType;
	}
}
TypeList::operator string() {
	string acc;
	for (vector<Type *>::iterator iter = list.begin(); iter != list.end(); iter++) {
		acc += (string)(**iter);
		if ((iter+1) != list.end()) {
			acc += ", ";
		}
	}
	return acc;
}

// ErrorType functions
ErrorType::ErrorType() {category = CATEGORY_ERRORTYPE;}
bool ErrorType::isComparable() {return false;}
bool ErrorType::operator==(Type &otherType) {
	if (otherType.category == CATEGORY_ERRORTYPE) {
		return (this == &otherType);
	} else {
		return false;
	}
}
Type &ErrorType::operator>>(Type &otherType) {return *this;}
ErrorType::operator string() {
	return "error";
}

// StdType functions
StdType::StdType(int kind, int suffix, int depth) : kind(kind) {category = CATEGORY_STDTYPE; this->suffix = suffix; this->depth = depth;}
StdType::~StdType() {category = CATEGORY_STDTYPE;}
bool StdType::isComparable() {return (kind >= STD_MIN_COMPARABLE && kind <= STD_MAX_COMPARABLE && (suffix == SUFFIX_CONSTANT || suffix == SUFFIX_LATCH));}
bool StdType::operator==(Type &otherType) {
	if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		return (kind == otherTypeCast->kind && baseEquals(otherType));
	} else {
		return false;
	}
}
Type &StdType::operator>>(Type &otherType) {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*this >> *(otherTypeCast->list[0]))) {
			return *nullType;
		} else {
			return *errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		if (isComparable() && otherTypeCast->isComparable() && kind <= otherTypeCast->kind && baseSendable(otherType)) {
			return *nullType;
		} else {
			return *errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (*this >> *(otherTypeCast->from)) {
			return *(otherTypeCast->to);
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*this >> **iter) {
				return otherType;
			}
		}
		return *errType;
	} else if (otherType.category == CATEGORY_ERRORTYPE) {
		return otherType;
	}
}
StdType::operator string() {
	switch(kind) {
		// null type
		case STD_NULL:
			return "null";
		// standard types
		case STD_INT:
			return "int";
		case STD_FLOAT:
			return "float";
		case STD_BOOL:
			return "bool";
		case STD_CHAR:
			return "char";
		case STD_STRING:
			return "string";
		// prefix operators
		case STD_NOT:
			return "!";
		case STD_COMPLEMENT:
			return "~";
		case STD_DPLUS:
			return "++";
		case STD_DMINUS:
			return "--";
		// infix operators
		case STD_DOR:
			return "||";
		case STD_DAND:
			return "&&";
		case STD_OR:
			return "|";
		case STD_XOR:
			return "^";
		case STD_AND:
			return "&";
		case STD_DEQUALS:
			return "==";
		case STD_NEQUALS:
			return "!=";
		case STD_LT:
			return "<";
		case STD_GT:
			return ">";
		case STD_LE:
			return "<=";
		case STD_GE:
			return ">=";
		case STD_LS:
			return "<<";
		case STD_RS:
			return ">>";
		case STD_TIMES:
			return "*";
		case STD_DIVIDE:
			return "/";
		case STD_MOD:
			return "%";
		// multi operators
		case STD_PLUS:
			return "+";
		case STD_MINUS:
			return "-";
		// can't happen
		default:
			return "";
	}
}

// FilterType functions
FilterType::FilterType(Type *from, Type *to, int suffix, int depth) {
	category = CATEGORY_FILTERTYPE; this->suffix = suffix; this->depth = depth;
	if (from->category == CATEGORY_TYPELIST) {
		from = (TypeList *)from;
	} else {
		from = new TypeList(from);
	}
	if (to->category == CATEGORY_TYPELIST) {
		to = (TypeList *)from;
	} else {
		to = new TypeList(from);
	}
}
FilterType::FilterType(Type *from, int suffix, int depth) : to(new TypeList()) {
	category = CATEGORY_FILTERTYPE; this->suffix = suffix; this->depth = depth;
	if (from->category == CATEGORY_TYPELIST) {
		from = (TypeList *)from;
	} else {
		from = new TypeList(from);
	}
}
FilterType::~FilterType() {
	if (*from != *nullType && *from != *errType) {
		delete from;
	}
	if (*to != *nullType && *to != *errType) {
		delete to;
	}
}
bool FilterType::isComparable() {return false;}
bool FilterType::operator==(Type &otherType) {
	if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		return (*from == *(otherTypeCast->from) && *to == *(otherTypeCast->to) && baseEquals(otherType));
	} else {
		return false;
	}
}
Type &FilterType::operator>>(Type &otherType) {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*this >> *(otherTypeCast->list[0]))) {
			return *nullType;
		} else {
			return *errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return *errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		// check if the target accepts this filter as input
		if (*this >> *(otherTypeCast->from)) {
			return *(otherTypeCast->to);
		}
		// otherwise, check if if this filter is sendable to the target
		if (*this == otherType && baseSendable(otherType)) {
			return otherType;
		} else {
			return *errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		// if the target accepts this filter as input
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*this >> **iter) {
				return otherType;
			}
		}
		return *errType;
	} else if (otherType.category == CATEGORY_ERRORTYPE) {
		return otherType;
	}
}
FilterType::operator string() {
	string acc("[");
	acc += (string)(*from);
	acc += " --> ";
	acc += (string)(*to);
	acc += "]";
	return acc;
}

// ObjectType functions
// constructor works only if base->defSite is Declaration->TypedStaticTerm->Node->Object
ObjectType::ObjectType(SymbolTable *base, Tree *&recall, int suffix, int depth) : base(base) {
	category = CATEGORY_OBJECTTYPE; this->suffix = suffix; this->depth = depth;
	// build the list of constructors
	Tree *cs = base->defSite/*Declaration*/->child->next->next/*TypedStaticTerm*/->child/*Node*/->child/*Object*/->child->next/*Constructors*/;
	for (Tree *c = cs->child; c != NULL; c = (c->next->next != NULL) ? c->next->next->child : NULL) {
		// derive the constructor's type
		TypeList *curConsType;
		if (*(c->child->next) == TOKEN_LSQUARE) {
			curConsType = new TypeList();
		} else if (*(c->child->next) == TOKEN_NonRetFilterHeader) {
			Tree *paramList = c/*Constructor*/->child->next/*NonRetFilterHeader*/->child->next/*ParamList*/;
			curConsType = new TypeList(paramList, recall);
		}
		// check if there's already a constructor of this type
		vector<TypeList *>::iterator iter = constructorTypes.begin();
		while (iter != constructorTypes.end()) {
			if (**iter == *curConsType) {
				break;
			}
			// advance
			iter++;
		}
		if (iter == constructorTypes.end()) { // if there were no conflicts, add the constructor's type to the list
			constructorTypes.push_back(curConsType);
		} else { // otherwise, flag the conflict as an error
			Token curDefToken = c->child->t;
			semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate constructor of type "<<(string)(*curConsType)<<" in "<<base->id);
		}
	}
	// build the list of members
	for (Tree *pipe = cs->next->child; pipe != NULL; pipe = (pipe->next != NULL) ? pipe->next->next->child : NULL) {
		if (*(pipe->child) == TOKEN_Declaration) {
			memberNames.push_back(pipe->child->child->t.s); // ID
			Type *childType = getTypeDeclaration(pipe->child);
			memberTypes.push_back(childType);
		}
	}
}
ObjectType::~ObjectType() {
	for (vector<TypeList *>::iterator iter = constructorTypes.begin(); iter != constructorTypes.end(); iter++) {
		if (**iter != *nullType && **iter != *errType) {
			delete (*iter);
		}
	}
	for (vector<Type *>::iterator iter = memberTypes.begin(); iter != memberTypes.end(); iter++) {
		if (**iter != *nullType && **iter != *errType) {
			delete (*iter);
		}
	}
}
bool ObjectType::isComparable() {return false;}
bool ObjectType::operator==(Type &otherType) {
	if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		if (base->id == otherTypeCast->base->id && constructorTypes.size() == otherTypeCast->constructorTypes.size() && memberTypes.size() == otherTypeCast->memberTypes.size()) {
			// verify that constructors match
			vector<TypeList *>::iterator consIter1 = constructorTypes.begin();
			vector<TypeList *>::iterator consIter2 = otherTypeCast->constructorTypes.begin();
			while(consIter1 != constructorTypes.end() && consIter2 != otherTypeCast->constructorTypes.end()) {
				if (**consIter1 != **consIter2) {
					return false;
				}
				// advance
				consIter1++;
				consIter2++;
			}
			// verify that regular members match
			vector<Type *>::iterator memberIter1 = memberTypes.begin();
			vector<Type *>::iterator memberIter2 = otherTypeCast->memberTypes.begin();
			while(memberIter1 != memberTypes.end() && memberIter2 != otherTypeCast->memberTypes.end()) {
				if (**memberIter1 != **memberIter2) {
					return false;
				}
				// advance
				memberIter1++;
				memberIter2++;
			}
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}
Type &ObjectType::operator>>(Type &otherType) {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*this >> *(otherTypeCast->list[0]))) {
			return *nullType;
		} else {
			return *errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return *errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		return (*this >> *(otherTypeCast->from));
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		// check if the target accepts this object as input
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*this >> **iter) {
				return otherType;
			}
		}
		// otherwise, check if if this object is sendable to the target
		if (*this == otherType && baseSendable(otherType)) {
			return otherType;
		} else {
			return *errType;
		}
	} else if (otherType.category == CATEGORY_ERRORTYPE) {
		return otherType;
	}
}
ObjectType::operator string() {return base->id;}

// typing status block functions
TypeStatus::TypeStatus() : type(nullType), recall(NULL) {}
TypeStatus::TypeStatus(Type *type, Tree *recall) : type(type), recall(recall) {}
TypeStatus::~TypeStatus() {}
TypeStatus::operator Type *() {return type;}
TypeStatus::operator Tree *() {return recall;}
TypeStatus::operator bool() {return (*type != *errType);}
TypeStatus &TypeStatus::operator=(TypeStatus &otherStatus) {type = otherStatus.type; recall = otherStatus.recall; return *this;}
TypeStatus &TypeStatus::operator=(Type *otherType) {type = otherType; return *this;}
Type &TypeStatus::operator*() {
	if (type != NULL) {
		return (*type);
	} else {
		return (*errType);
	}
}
bool TypeStatus::operator==(Type &otherType) {return (*type == otherType);}
bool TypeStatus::operator!=(Type &otherType) {return (*type != otherType);}

// Main semantic analysis functions

void catStdNodes(SymbolTable *&stRoot) {
	*stRoot *= new SymbolTable(KIND_STD, "node");
	*stRoot *= new SymbolTable(KIND_STD, "int");
	*stRoot *= new SymbolTable(KIND_STD, "float");
	*stRoot *= new SymbolTable(KIND_STD, "bool");
	*stRoot *= new SymbolTable(KIND_STD, "char");
	*stRoot *= new SymbolTable(KIND_STD, "string");
}

void catStdOps(SymbolTable *&stRoot) {
	// prefix
	*stRoot *= new SymbolTable(KIND_STD, "!");
	*stRoot *= new SymbolTable(KIND_STD, "~");
	*stRoot *= new SymbolTable(KIND_STD, "++");
	*stRoot *= new SymbolTable(KIND_STD, "--");
	// infix
	*stRoot *= new SymbolTable(KIND_STD, "||");
	*stRoot *= new SymbolTable(KIND_STD, "&&");
	*stRoot *= new SymbolTable(KIND_STD, "|");
	*stRoot *= new SymbolTable(KIND_STD, "^");
	*stRoot *= new SymbolTable(KIND_STD, "&");
	*stRoot *= new SymbolTable(KIND_STD, "==");
	*stRoot *= new SymbolTable(KIND_STD, "!=");
	*stRoot *= new SymbolTable(KIND_STD, "<");
	*stRoot *= new SymbolTable(KIND_STD, ">");
	*stRoot *= new SymbolTable(KIND_STD, "<=");
	*stRoot *= new SymbolTable(KIND_STD, ">=");
	*stRoot *= new SymbolTable(KIND_STD, "<<");
	*stRoot *= new SymbolTable(KIND_STD, ">>");
	*stRoot *= new SymbolTable(KIND_STD, "*");
	*stRoot *= new SymbolTable(KIND_STD, "/");
	*stRoot *= new SymbolTable(KIND_STD, "%");
	// multi
	*stRoot *= new SymbolTable(KIND_STD, "+");
	*stRoot *= new SymbolTable(KIND_STD, "-");
}

void catStdLib(SymbolTable *&stRoot) {
	// standard root
	SymbolTable *stdLib = new SymbolTable(KIND_STD, STANDARD_LIBRARY_STRING);

	// system nodes
	// streams
	*stdLib *= new SymbolTable(KIND_STD, "in");
	*stdLib *= new SymbolTable(KIND_STD, "out");
	*stdLib *= new SymbolTable(KIND_STD, "err");
	// control nodes
	*stdLib *= new SymbolTable(KIND_STD, "rand");
	*stdLib *= new SymbolTable(KIND_STD, "delay");

	// standard library
	// containers
	*stdLib *= new SymbolTable(KIND_STD, "stack");
	*stdLib *= new SymbolTable(KIND_STD, "map");
	// filters
	*stdLib *= new SymbolTable(KIND_STD, "filter");
	*stdLib *= new SymbolTable(KIND_STD, "sort");
	// generators
	*stdLib *= new SymbolTable(KIND_STD, "gen");

	// concatenate the library to the root
	*stRoot *= stdLib;
}

SymbolTable *genDefaultDefs() {
	// generate the root block node
	SymbolTable *stRoot = new SymbolTable(KIND_BLOCK, BLOCK_NODE_STRING);
	// concatenate in the standard types
	catStdNodes(stRoot);
	// concatenate in the standard operators
	catStdOps(stRoot);
	// concatenate in the standard library
	catStdLib(stRoot);
	// finally, return the generated default symtable
	return stRoot;
}

// recursively extracts the appropriate nodes from the given tree and appropriately populates the passed containers
void buildSt(Tree *tree, SymbolTable *st, vector<SymbolTable *> &importList) {
	// base case
	if (tree == NULL) {
		return;
	}
	// log the current symbol environment in the tree
	tree->env = st;
	// recursive cases
	if (*tree == TOKEN_Block || *tree == TOKEN_Object) { // if it's a block-style node
		// allocate the new block definition node
		SymbolTable *blockDef = new SymbolTable(KIND_BLOCK, BLOCK_NODE_STRING, tree);
		// finally, link the block node into the main trunk
		*st *= blockDef;
		// recurse
		buildSt(tree->child, blockDef, importList); // child of Block
	} else if (*tree == TOKEN_FilterHeader || *tree == TOKEN_NonRetFilterHeader) { // if it's a filter header node
		// locate the corresponding block and create an st node for it
		Tree *block = tree->next; // Block
		// allocate the new block definition node
		SymbolTable *blockDef = new SymbolTable(KIND_BLOCK, BLOCK_NODE_STRING, block);
		// finally, link the block node into the main trunk
		*st *= blockDef;
		// parse out the header's parameter declarations and add them to the st
		Tree *pl = tree->child->next; // ParamList or RetList
		if (*pl == TOKEN_ParamList) { // if there is a parameter list to process
			Tree *param = pl->child; // Param
			for (;;) { // per-param loop
				// allocate the new parameter definition node
				SymbolTable *paramDef = new SymbolTable(KIND_PARAM, param->child->next->t.s, param);
				// ... and link it into the block node
				*blockDef *= paramDef;
				// advance
				if (param->next != NULL) {
					param = param->next->next->child; // Param
				} else {
					break;
				}
			} // per-param loop
		} // if there is a parameter list to process
		// recurse
		buildSt(block->child, blockDef, importList); // child of Block
	} else if (*tree == TOKEN_Constructor) { // if it's a constructor node
		// allocate the new definition node
		SymbolTable *newDef = new SymbolTable(KIND_STATIC_DECL, tree->child->t.s, tree);
		// ... and link it in
		*st *= newDef;
		// recurse
		buildSt(tree->child, newDef, importList); // child of Constructor
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Declaration) { // if it's a declaration node
		Tree *bnc = tree->child->next;
		if (*bnc == TOKEN_EQUALS) { // standard static declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_STATIC_DECL, tree->child->t.s, tree);
			// ... and link it in
			*st *= newDef;
			// recurse
			buildSt(tree->child, newDef, importList); // child of Declaration
		} else if (*bnc == TOKEN_ERARROW) { // flow-through declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_THROUGH_DECL, tree->child->t.s, tree);
			// ... and link it in
			*st *= newDef;
			// recurse
			buildSt(tree->child, newDef, importList); // child of Declaration
		} else if (*bnc == TOKEN_SuffixedIdentifier) { // import declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_IMPORT, IMPORT_DECL_STRING, tree);
			// ... and link it in
			*st *= newDef;
			// also, since it's an import declaration, log it to the import list
			importList.push_back(newDef);
			// recurse
			buildSt(tree->child, newDef, importList); // child of Declaration
		}
	} else { // else if it's not a declaration node
		// recurse normally
		buildSt(tree->child, st, importList); // down
		buildSt(tree->next, st, importList); // right
	}
}

// forward declarations
SymbolTable *bindId(string &id, SymbolTable *env);

// binds qualified identifiers in the given symtable environment; returns the tail of the binding
// also, updates id to contain the portion of the identifier that could not be bound
// returns NULL if no binding whatsoever can be found
SymbolTable *bindId(Type *inType, string &id, SymbolTable *env) {
	// base case
	if (env == NULL) {
		return NULL;
	}
	// recall identifier case
	if (id[0] == '.' && id[1] == '.') {

		if (inType != NULL) { // if there is an input type
			// update id to be the rest of the recall identifier
			string idRest = "";
			if (id.length() >= 3) { // if this recall identifier has a rest-part
				idRest = id.substr(3, id.length()-3); // get the rest of the recall identifier string
			}
			id = idRest;

			// return the binding of the current input type, since that's what the recall identifier implicitly binds to
			if (*inType == *nullType) { // if the incoming type is null, we can't bind to it, so return an error
				return NULL;
			} else { // else if it's any other type, bind to it using its string representation
// LOL
				string recString = *inType; // statically get the string representation
				return bindId(recString, env); // statically bind to the specific standard node
			}
		} else { // else if there is no input type
			return NULL;
		}
	}
	// recursive case
	string tip = idHead(id);
	string idCur = idTail(id);
	SymbolTable *stCur = NULL;
	// scan the current environment's children for a latch point
	for (vector<SymbolTable *>::iterator latchIter = env->children.begin(); latchIter != env->children.end(); latchIter++) {
		if ((*latchIter)->id == tip) { // if we've found a latch point

			// verify that the latching holds for the rest of the identifier
			stCur = *latchIter;
			while (!idCur.empty()) { // while there's still more of the identifier to match

				// find a static match in the current st node's children
				SymbolTable *match = NULL;
				for (vector<SymbolTable *>::iterator stcIter = stCur->children.begin(); stcIter != stCur->children.end(); stcIter++) {
					string idCurHead = idHead(idCur);
					if ((*stcIter)->id == idCurHead) { // if the identifiers are the same, we have a match
						match = *stcIter;
						goto matchOK;

					// as a special case, look one block level deeper, since nested defs must be block-delimited
					} else if (stCur->kind != KIND_BLOCK && (*stcIter)->kind == KIND_BLOCK) {
						for (vector<SymbolTable *>::iterator blockIter = (*stcIter)->children.begin(); blockIter != (*stcIter)->children.end(); blockIter++) {
							if ((*blockIter)->id[0] != '_' && (*blockIter)->id == idCurHead) { // if the identifiers are the same, we have a match
								match = *blockIter;
								goto matchOK;
							}
						}
					}

				} matchOK: ; // match verification loop

				if (match != NULL) { // if we have a match for this part of the rest
					// advance to the matched st node
					stCur = match;
					// advance to the next token in the id
					idCur = idTail(idCur);
				} else { // else if we don't have a match for this part of the rest
					break;
				}

			}
			// no need to look through the rest of the children; we've already found the correctly named one on this level
			break;
		}
	} // per-latch point loop

	// if we've verified the entire identifier, return the tail of the latch point
	if (stCur != NULL && idCur.empty()) {
		id.clear(); // clear id to signify that the latching is complete
		return stCur; // return the tail of the latch point
	}

	// otherwise, compare the current binding with the alternate higher-level one
	// but first, scan up to the next block level, since jumping to the enclosing identifier is a waste of time
	SymbolTable *recurseSt = env->parent;
	while (recurseSt != NULL && recurseSt->kind != KIND_BLOCK) {
		recurseSt = recurseSt->parent;
	}
	string altId = id;
	if (stCur != NULL) { // if there was some sort of binding, update id to be the rest of it
		id = idCur;
	}

	// do the recursion
	SymbolTable *altResult = bindId(inType, altId, recurseSt);

	// check if the recursive solution is better
	if (altResult != NULL  && ((altId.length() < id.length()) || stCur == NULL)) { // if the other binding succeeded and is better, return it
		id = altId;
		return altResult;
	} else { // otherwise, return the local binding
		return stCur;
	}
}

// wrappers for the above function
SymbolTable *bindId(string &id, SymbolTable *env) {
	return bindId(NULL, id, env);
}


void subImportDecls(vector<SymbolTable *> importList) {
	bool stdExplicitlyImported = false;

	for(;;) { // per-change loop
		// per-import loop
		vector<SymbolTable *> newImportList;
		for (vector<SymbolTable *>::iterator importIter = importList.begin(); importIter != importList.end(); importIter++) {
			// extract the import path out of the iterator
			string importPath = sid2String((*importIter)->defSite->child->next);
			// standard import special-casing
			if (importPath == "std") { // if it's the standard import
				if (!stdExplicitlyImported) { // if it's the first standard import, flag it as handled and let it slide
					(*importIter)->id = STANDARD_IMPORT_DECL_STRING;
					stdExplicitlyImported = true;
					continue;
				}
			}
			// otherwise, try to find a non-std binding for this import
			SymbolTable *binding = bindId(importPath, *importIter);
			if (binding != NULL && importPath.empty()) { // if we found a complete static binding
				// check to make sure that this import doesn't cause a binding conflict
				string importPathTip = binding->id; // must exist if binding succeeed
				// per-parent's children loop (parent must exist, since the root is a block st node)
				vector<SymbolTable *>::iterator childIter = (*importIter)->parent->children.begin();
				while (childIter != (*importIter)->parent->children.end()) {
					if ((*childIter)->id[0] != '_' && (*childIter)->id == importPathTip) { // if there's a conflict
						Token curDefToken = (*importIter)->defSite->child->next->child->t; // child of Identifier
						Token prevDefToken;
						if ((*childIter)->defSite != NULL) { // if there is a definition site for the previous symbol
							prevDefToken = (*childIter)->defSite->t;
						} else { // otherwise, it must be a standard definition, so make up the token as if it was
							prevDefToken.fileName = STANDARD_LIBRARY_STRING;
							prevDefToken.row = 0;
							prevDefToken.col = 0;
						}
						semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"name conflict in importing '"<<importPathTip<<"'");
						semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (conflicting definition was here)");
						goto nextImport;
					}
					// advance
					childIter++;
				}
				// there was no conflict, so just deep-copy the binding in place of the import placeholder node
				**importIter = *binding;
			} else { // else if no binding could be found
				newImportList.push_back(*importIter); // log the failed node for rebinding during the next round
			}
			nextImport: ;
		} // per-import loop
		if (newImportList.size() == importList.size()) { // if the import table has stabilized
			for (vector<SymbolTable *>::iterator importIter = newImportList.begin(); importIter != newImportList.end(); importIter++) {
				Token curToken = (*importIter)->defSite->t;
				string importPath = sid2String((*importIter)->defSite->child->next);
				semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve import '"<<importPath<<"'");
			}
			break;
		} else { // else if the import table hasn't stabilized yet, do another substitution round on the failed binding list
			importList = newImportList;
		}
	} // per-change loop
}

Type *getStType(SymbolTable *st) {
	if (st->defSite != NULL && st->defSite->type != NULL) { // if there is already a type logged for this st node
		return st->defSite->type;
	} else { // else if we need to derive a type ourselves
// LOL
		return errType;
	}
}

// typing function definitions

// reports errors
TypeStatus getTypeSuffixedIdentifier(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	string id = sid2String(tree); // string representation of this identifier
	string idCur = id; // a destructible copy for the recursion
	SymbolTable *st = bindId(inStatus, idCur, tree->env);
	if (st != NULL) { // if we found a binding
		type = getStType(st);
	} else { // else if we couldn't find a binding
		Token curToken = tree->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve '"<<id<<"'");
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypePrefixOrMultiOp(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *pomocc = tree->child->child;
	Type *subType = getTypePrimary(tree->next, inStatus);
	if (*pomocc == TOKEN_NOT) {
		if (*subType == STD_BOOL) {
			type = subType;
		}
	} else if (*pomocc == TOKEN_COMPLEMENT) {
		if (*subType == STD_INT) {
			type = subType;
		}
	} else if (*pomocc == TOKEN_DPLUS) {
		if (*subType == STD_INT) {
			type = subType;
		}
	} else if (*pomocc == TOKEN_DMINUS) {
		if (*subType == STD_INT) {
			type = subType;
		}
	} else if (*pomocc == TOKEN_PLUS) {
		if (*subType == STD_INT || *subType == STD_FLOAT) {
			type = subType;
		}
	} else if (*pomocc == TOKEN_MINUS) {
		if (*subType == STD_INT || *subType == STD_FLOAT) {
			type = subType;
		}
	}
	GET_TYPE_FOOTER;
}

// reports errors for TOKEN_SLASH case
TypeStatus getTypePrimary(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *primaryc = tree->child;
	if (*primaryc == TOKEN_SuffixedIdentifier) {
		type = getTypeSuffixedIdentifier(primaryc, inStatus);
	} else if (*primaryc == TOKEN_SLASH) { // if it's a delatched term
		Tree *subSI = primaryc->next; // SuffixedIdentifier
		Type *subType = getTypeSuffixedIdentifier(subSI, inStatus); // SuffixedIdentifier
		if (*subType != TYPE_ERROR) { // if we derived a subtype
			if (subType->suffix != SUFFIX_NONE) { // if the derived type is a latch or a stream
				// copy the subtype
				type = new Type(*subType);
				// down-level the type
				type->delatch();
			} else { // else if the derived type isn't a latch or stream (and thus can't be delatched), error
				Token curToken = primaryc->t;
				semmerError(curToken.fileName,curToken.row,curToken.col,"delatching non-latch, non-stream '"<<sid2String(subSI)<<"'");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<*inStatus<<")");
			}
		}
	} else if (*primaryc == TOKEN_PrimLiteral) {
		type = getTypePrimLiteral(primaryc, inStatus);
	} else if (*primaryc == TOKEN_PrefixOrMultiOp) {
		type = getTypePrefixOrMultiOp(primaryc, inStatus);
	} else if (*primaryc == TOKEN_LBRACKET) {
		type = getTypeExp(primaryc->next, inStatus);
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeBracketedExp(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *exp = tree->child->next; // Exp
	return getTypeExp(exp, inStatus);
	GET_TYPE_FOOTER;
}

// reports errors
TypeStatus getTypeExp(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *expc = tree->child;
	if (*expc == TOKEN_Primary) {
		type = getTypePrimary(expc, inStatus);
	} else if (*expc == TOKEN_Exp) {
		Tree *expLeft = expc;
		Tree *op = expLeft->next;
		Tree *expRight = op->next;
		Type *typeLeft = getTypeExp(expLeft, inStatus);
		Type *typeRight = getTypeExp(expRight, inStatus);
		switch (op->t.tokenType) {
			case TOKEN_DOR:
			case TOKEN_DAND:
				if (*typeLeft == STD_BOOL && *typeRight == STD_BOOL) {
					type = typeLeft;
				}
				break;
			case TOKEN_OR:
			case TOKEN_XOR:
			case TOKEN_AND:
				if (*typeLeft == STD_INT && *typeRight == STD_INT) {
					type = typeLeft;
				}
				break;
			case TOKEN_DEQUALS:
			case TOKEN_NEQUALS:
			case TOKEN_LT:
			case TOKEN_GT:
			case TOKEN_LE:
			case TOKEN_GE:
				if (typeLeft->isComparable() && typeRight->isComparable()) {
					type = new Type(STD_BOOL);
				}
				break;
			case TOKEN_LS:
			case TOKEN_RS:
				if (*typeLeft == STD_INT && *typeRight == STD_INT) {
					type = typeLeft;
				}
				break;
			case TOKEN_TIMES:
			case TOKEN_DIVIDE:
			case TOKEN_MOD:
			case TOKEN_PLUS:
			case TOKEN_MINUS:
				if ( (*typeLeft == STD_INT || *typeLeft == STD_FLOAT) &&
						(*typeRight == STD_INT || *typeRight == STD_FLOAT) ) {
					if (*typeRight != STD_FLOAT) {
						type = typeLeft;
					} else {
						type = typeRight;
					}
				}
				break;
			default: // can't happen
				typeLeft = NULL;
				typeRight = NULL;
				break;
		} // switch
	} // if
	// if we couldn't resolve a type
	if (type == NULL) {
		Token curToken = tree->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve expression's type");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypePrimOpNode(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *ponc = tree->child->child; // the operator token itself
	// generate the type based on the specific operator it is
	switch (ponc->t.tokenType) {
		case TOKEN_NOT:
			type = new Type(STD_NOT);
			break;
		case TOKEN_COMPLEMENT:
			type = new Type(STD_COMPLEMENT);
			break;
		case TOKEN_DPLUS:
			type = new Type(STD_DPLUS);
			break;
		case TOKEN_DMINUS:
			type = new Type(STD_DMINUS);
			break;
		case TOKEN_DOR:
			type = new Type(STD_DOR);
			break;
		case TOKEN_DAND:
			type = new Type(STD_DAND);
			break;
		case TOKEN_OR:
			type = new Type(STD_OR);
			break;
		case TOKEN_XOR:
			type = new Type(STD_XOR);
			break;
		case TOKEN_AND:
			type = new Type(STD_AND);
			break;
		case TOKEN_DEQUALS:
			type = new Type(STD_DEQUALS);
			break;
		case TOKEN_NEQUALS:
			type = new Type(STD_NEQUALS);
			break;
		case TOKEN_LT:
			type = new Type(STD_LT);
			break;
		case TOKEN_GT:
			type = new Type(STD_GT);
			break;
		case TOKEN_LE:
			type = new Type(STD_LE);
			break;
		case TOKEN_GE:
			type = new Type(STD_GE);
			break;
		case TOKEN_LS:
			type = new Type(STD_LS);
			break;
		case TOKEN_RS:
			type = new Type(STD_RS);
			break;
		case TOKEN_TIMES:
			type = new Type(STD_TIMES);
			break;
		case TOKEN_DIVIDE:
			type = new Type(STD_DIVIDE);
			break;
		case TOKEN_MOD:
			type = new Type(STD_MOD);
			break;
		case TOKEN_PLUS:
			type = new Type(STD_PLUS);
			break;
		case TOKEN_MINUS:
			type = new Type(STD_MINUS);
			break;
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypePrimLiteral(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *plc = tree->child;
	if (*plc == TOKEN_INUM) {
		type = new Type(STD_INT);
	} else if (*plc == TOKEN_FNUM) {
		type = new Type(STD_FLOAT);
	} else if (*plc == TOKEN_CQUOTE) {
		type = new Type(STD_CHAR);
	} else if (*plc == TOKEN_SQUOTE) {
		type = new Type(STD_STRING);
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeBlock(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *pipeCur = tree->child->next->child; // Pipe
	bool pipeTypesValid = true;
	while(pipeCur != NULL) {
		// try to get a type for this pipe
		Type *resultType = getTypePipe(pipeCur, inStatus);
		// if we failed to find a type, flag this fact
		if (*resultType == TYPE_ERROR) {
			pipeTypesValid = false;
		}
		// advance
		if (pipeCur->next != NULL) {
			pipeCur = pipeCur->next->next->child; // Pipe
		} else {
			pipeCur = NULL;
		}
	}
	// if we managed to derive a type for all of the enclosed pipes
	if (pipeTypesValid) {
		// set the result type to the input type being mapped to the null type
		type = new FilterType(inStatus, nullType);
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeFilterHeader(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Type *fromType = nullType;
	Type *toType = nullType;

	Tree *treeCur = tree->child->next; // ParamList
	if (*treeCur == TOKEN_ParamList) {
		fromType = getTypeParamList(treeCur, inStatus);
		treeCur = treeCur->next; // RetList or RSQUARE
	}
	if (*treeCur == TOKEN_RetList) {
		fromType = getTypeRetList(treeCur, inStatus);
	}
	if (*fromType != TYPE_ERROR && *toType != TYPE_ERROR) {
		type = new FilterType(fromType, toType);
	}
	GET_TYPE_FOOTER;
}

// reports errors
TypeStatus getTypeFilter(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *tc = tree->child; // FilterHeader or Block
	Type *headerType = nullType;
	if (*tc == TOKEN_FilterHeader) { // if there is a header, derive its type
		headerType = getTypeFilterHeader(tc, inStatus);
	}
	if (*headerType != TYPE_ERROR) { // if we end up with a non-erroneous type for the header
// LOL
	} else { // else if we derived an erroneous type for the header
		Token curToken = tc->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve node header type");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}

	GET_TYPE_FOOTER;
}

TypeStatus getTypeObjectBlock(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
// LOL
	GET_TYPE_FOOTER;
}

TypeStatus getTypeTypeList(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
// LOL
	GET_TYPE_FOOTER;
}

TypeStatus getTypeParamList(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
// LOL
	GET_TYPE_FOOTER;
}

TypeStatus getTypeRetList(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
// LOL
	GET_TYPE_FOOTER;
}

// reports errors
TypeStatus getTypeNodeInstantiation(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *netl = tree->child->next; // TypeList
	type = getTypeTypeList(netl, inStatus);
	if (*type != TYPE_ERROR) { // if we derived a type for the instantiation
		if (netl->next->next != NULL) { // if there's an initializer, we need to make sure that the types are compatible
			Tree *st = netl->next->next->next; // StaticTerm
			Type *initType = getTypeStaticTerm(st, inStatus);
			if (*initType != TYPE_ERROR) { //  if we derived a type for the initializer
				if (!(*initType >> *type)) { // if the types are incompatible, throw an error
					Token curToken = st->t;
					semmerError(curToken.fileName,curToken.row,curToken.col,"initializer type incompatible with instantiation");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (instantiation type is "<<*type<<")");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (initializer type is "<<*initType<<")");
				}
			} else { // else if we couldn't derive a type for the initializer
				Token curToken = st->t;
				semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve initializer's type");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
			}
		}
	} else { // else if we couldn't derive a type for the instantiation
		Token curToken = tree->child->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve instantiation type");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_TYPE_FOOTER;
}

// blindly derives types from headers: does not verify sub-blocks
TypeStatus getTypeNodeSoft(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
// LOL
	GET_TYPE_FOOTER;
}

// reports errors
TypeStatus getTypeNode(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *nodec = tree->child;
	if (*nodec == TOKEN_SuffixedIdentifier) {
		type = getTypeSuffixedIdentifier(nodec, inStatus);
	} else if (*nodec == TOKEN_NodeInstantiation) {
		type = getTypeNodeInstantiation(nodec, inStatus);
	} else if (*nodec == TOKEN_Filter) {
		type = getTypeFilter(nodec, inStatus);
	} else if (*nodec == TOKEN_Object) {
		type = getTypeObjectBlock(nodec, inStatus);
	} else if (*nodec == TOKEN_PrimOpNode) {
		type = getTypePrimOpNode(nodec, inStatus);
	} else if (*nodec == TOKEN_PrimLiteral) {
		type = getTypePrimLiteral(nodec, inStatus);

	}
	// if we couldn't resolve a type
	if (type == NULL && *nodec != TOKEN_SuffixedIdentifier) {
		Token curToken = tree->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve node's type");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_TYPE_FOOTER;
}

// reports errors
TypeStatus getTypeTypedStaticTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *tstc = tree->child;
	if (*tstc == TOKEN_Node) {
		type = getTypeNode(tstc, inStatus);
	} else if (*tstc == TOKEN_LBRACKET) { // it's an expression
		type = getTypeExp(tstc->next, inStatus); // move past the bracket to the actual Exp node
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeStaticTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *stc = tree->child;
	if (*stc == TOKEN_TypedStaticTerm) {
		type = getTypeTypedStaticTerm(stc, inStatus);
	} else if (*stc == TOKEN_Access) {
// LOL
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeDynamicTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *dtc = tree->child;
	if (*dtc == TOKEN_StaticTerm) {
		type = getTypeStaticTerm(dtc, inStatus);
	} else if (*dtc == TOKEN_Compound) {
// LOL
	} else if (*dtc == TOKEN_Link) {
// LOL
	} else if (*dtc == TOKEN_Send) {
// LOL
	} else if (*dtc == TOKEN_Swap) {
	// LOL
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeSwitchTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	vector<Type *> toTypes; // vector for logging the destination types of each branch
	vector<Tree *> toTrees; // vector for logging the tree nodes of each branch
	Tree *lpCur = tree->child->next->next; // LabeledPipes
	for (;;) { // per-labeled pipe loop
		Tree *lpc = lpCur->child; // StaticTerm or COLON
		// if there is a non-default label on this pipe, check its validity
		if (*lpc == TOKEN_StaticTerm) {
			// derive the label's type
			Type *labelType = getTypeStaticTerm(lpc, inStatus);
			if (*inStatus != *labelType) { // if the type doesn't match, throw an error
				Token curToken = lpc->t;
				semmerError(curToken.fileName,curToken.row,curToken.col,"switch label type doesn't match input type");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (label type is "<<*labelType<<")");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
			}
		}
		// derive the to-type of this label
		Tree *toTree = (*lpc == TOKEN_StaticTerm) ? lpc->next->next : lpc->next; // SimpleTerm
		Type *toType = getTypeSimpleTerm(toTree, inStatus);
		// log the to-type and to-tree of this label
		toTypes.push_back(toType);
		toTrees.push_back(toTree);
		// advance
		if (lpCur->child->next->next != NULL && lpCur->child->next->next->next != NULL) {
			lpCur = lpCur->child->next->next->next; // LabeledPipes
		} else {
			break;
		}
	} // per-labeled pipe loop
	// verify that all of the to-types are the same
	Type *firstToType = toTypes[0];
	Tree *firstToTree = toTrees[0];
	for (unsigned int i=1; i < toTypes.size(); i++) { // for each to-type
		Type *toType = toTypes[i];
		if (*toType != *firstToType) { // if the types don't match, throw an error
			Tree *toTree = toTrees[i];
			Token curToken = toTree->t;
			Token curToken2 = firstToTree->t;
			semmerError(curToken.fileName,curToken.row,curToken.col,"switch destination types are inconsistent");
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (this type is "<<*toType<<")");
			semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (first type is "<<*firstToType<<")");
		}
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeSimpleTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *stc = tree->child;
	if (*stc == TOKEN_DynamicTerm) {
		type = getTypeDynamicTerm(stc, inStatus);
	} else if (*stc == TOKEN_SwitchTerm) {
		type = getTypeSwitchTerm(stc, inStatus);
	}
	GET_TYPE_FOOTER;
}

// reports errors
TypeStatus getTypeSimpleCondTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		inStatus.type = inStatus.recall->type;
		type = getTypeTerm(tree->child->next, inStatus);
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeClosedTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *ctc = tree->child;
	if (*ctc == TOKEN_SimpleTerm) {
		type = getTypeSimpleTerm(ctc, inStatus);
	} else if (*ctc == TOKEN_ClosedCondTerm) {
		type = getTypeClosedCondTerm(ctc, inStatus);
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeOpenTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *otc = tree->child;
	if (*otc == TOKEN_SimpleCondTerm) {
		type = getTypeSimpleCondTerm(otc, inStatus);
	} else if (*otc == TOKEN_OpenCondTerm) {
		type = getTypeOpenCondTerm(otc, inStatus);
	}
	GET_TYPE_FOOTER;
}

// reports errors
TypeStatus getTypeOpenCondTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		Tree *trueBranch = tree->child->next;
		Tree *falseBranch = trueBranch->next->next;
		inStatus.type = inStatus.recall->type;
		Type *trueType = getTypeClosedTerm(trueBranch, inStatus);
		Type *falseType = getTypeOpenTerm(falseBranch, inStatus);
		if (*trueType == *falseType) { // if the two branches match in type
			type = trueType;
		} else { // else if the two branches don't match in type
			Token curToken = tree->child->t; // QUESTION
			Token curToken2 = trueBranch->t; // ClosedTerm
			Token curToken3 = falseBranch->t; // OpenTerm
			semmerError(curToken.fileName,curToken.row,curToken.col,"type mismatch in conditional operator branches");
			semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (true branch type is "<<*trueType<<")");
			semmerError(curToken3.fileName,curToken3.row,curToken3.col,"-- (false branch type is "<<*trueType<<")");
		}
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeClosedCondTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		Tree *trueBranch = tree->child->next;
		Tree *falseBranch = trueBranch->next->next;
		inStatus.type = inStatus.recall->type;
		Type *trueType = getTypeClosedTerm(trueBranch, inStatus);
		Type *falseType = getTypeClosedTerm(falseBranch, inStatus);
		if (*trueType == *falseType) { // if the two branches match in type
			type = trueType;
		} else { // else if the two branches don't match in type
			Token curToken = tree->child->t; // QUESTION
			Token curToken2 = trueBranch->t; // ClosedTerm
			Token curToken3 = falseBranch->t; // ClosedTerm
			semmerError(curToken.fileName,curToken.row,curToken.col,"type mismatch in conditional operator branches");
			semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (true branch type is "<<*trueType<<")");
			semmerError(curToken3.fileName,curToken3.row,curToken3.col,"-- (false branch type is "<<*trueType<<")");
		}
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeTerm(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *tc2 = tree->child->child;
	if (*tc2 == TOKEN_SimpleCondTerm) {
		type = getTypeSimpleCondTerm(tc2, inStatus);
	} else if (*tc2 == TOKEN_OpenCondTerm) {
		type = getTypeOpenCondTerm(tc2, inStatus);
	} else if (*tc2 == TOKEN_SimpleTerm) {
		type = getTypeSimpleTerm(tc2, inStatus);
	} else if (*tc2 == TOKEN_ClosedCondTerm) {
		type = getTypeClosedCondTerm(tc2, inStatus);
	}
	GET_TYPE_FOOTER;
}

// reports errors
TypeStatus getTypeNonEmptyTerms(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	// scan the pipe left to right
	Tree *curTerm = tree->child; // Term
	Type *outType = NULL;
	while (curTerm != NULL) {
		outType = getTypeTerm(curTerm, inStatus);
		if (*outType) { // if we found a proper typing for this term, log it
			curTerm->type = outType;
			inStatus = outType;
		} else { // otherwise, if we were unable to assign a type to the term, flag an error
			Token curToken = curTerm->t;
			semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve term's output type");
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
			// log the fact that typing failed
			outType = NULL;
			break;
		}
		// advance
		curTerm = curTerm->next->child; // Term or NULL
	}
	// if we succeeded in deriving an output type, return the mapping of the imput type to the output type
	if (outType != NULL) {
		type = new FilterType(inStatus, outType);
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypeDeclaration(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *declarationSub = tree->child->next->next; // TypedStaticTerm, NonEmptyTerms, or NULL
	if (declarationSub != NULL && (*declarationSub == TOKEN_TypedStaticTerm || *declarationSub == TOKEN_NonEmptyTerms)) {
		if (tree->handled) { // if this isn't the first time we're trying to derive the type of this node, flag recursion error
			Token curDefToken = tree->child->t;
			semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"ambiguous recursive definition of '"<<curDefToken.s<<"'");
			type = errType;
		} else { // otherwise, proceed with normal type derivation
			tree->handled = true;
			if (*declarationSub == TOKEN_TypedStaticTerm) { // if it's a regular declaration
				Tree *tstc = declarationSub->child; // Node or LBRACKET
				if (*tstc == TOKEN_Node) { // possibly recursive node declaration
					// first, set the identifier's type to the declared type of the Node
					type = getTypeNodeSoft(tstc);
					if (*type) {
						tree->type = type;
						// then, verify types for the declaration sub-block
						type = getTypeNode(tstc);
					}
				} else if (*tstc == TOKEN_BracketedExp) { // non-recursive expression declaration
					// derive the type of the expression without doing any bindings, since expressions must be non-recursive
					type = getTypeBracketedExp(tstc, inStatus);
				}
			} else if (*declarationSub == TOKEN_NonEmptyTerms) { // else if it's a flow-through declaration
				// first, set the identifier's type to the type of the NonEmptyTerms stream (an inputType consumer)
				tree->type = new FilterType(inStatus);
				// then, verify types for the declaration sub-block
				type = getTypeNonEmptyTerms(declarationSub, inStatus);
				// delete the temporary filter type
				delete (tree->type);
			} // otherwise, if it's an import declaration, do nothing
		}
	}
	GET_TYPE_FOOTER;
}

TypeStatus getTypePipe(Tree *tree, TypeStatus inStatus) {
	GET_TYPE_HEADER;
	Tree *pipec = tree->child;
	if (*pipec == TOKEN_NonEmptyTerms) { // if it's a raw NonEmptyTerms pipe
		type = getTypeNonEmptyTerms(pipec, inStatus);
	} else if (*pipec == TOKEN_Declaration) { // else if it's a Declaration pipe
		type = getTypeDeclaration(pipec, inStatus);
	}
	GET_TYPE_FOOTER;
}

void traceTypes(vector<Tree *> *parseme) {
	// get a list of Pipe nodes
	vector<Tree *> &pipeList = parseme[TOKEN_Pipe];
	// iterate through the list of Pipes and trace the type flow for each one
	for (unsigned int i=0; i < pipeList.size(); i++) {
		Tree *pipeCur = pipeList[i];
		if (pipeCur->type == NULL) { // if we haven't derived a type for this pipe yet
			getTypePipe(pipeCur);
		}
	}
}

// main semming function; makes no assumptions about stRoot's value; it's just a return parameter
int sem(Tree *treeRoot, vector<Tree *> *parseme, SymbolTable *&stRoot) {

	// initialize error code
	semmerErrorCode = 0;
	semmerEventuallyGiveUp = eventuallyGiveUp;

	VERBOSE( printNotice("Collecting tree nodes..."); )

	// initialize the symbol table root with the default definitions
	stRoot = genDefaultDefs();

	// populate the symbol table with definitions from the user parseme, and log the used imports/id instances
	vector<SymbolTable *> importList; // import Declaration nodes
	buildSt(treeRoot, stRoot, importList); // get user definitions/imports
	subImportDecls(importList); // resolve and substitute import declarations into the symbol table

	VERBOSE( cout << stRoot; )

	VERBOSE( printNotice("Tracing type flow..."); )

	// assign types to all node streams
	traceTypes(parseme);

	// if there were no errors, free the error type node
	if (!semmerErrorCode) {
		delete errType;
	}

	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
