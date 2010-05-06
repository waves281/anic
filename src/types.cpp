#include "types.h"

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
TypeList::TypeList(Tree *tree, Tree *recall) {
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
		} else { // *(typeSuffix->child) == TOKEN_ArrayTypeSuffix
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
			curType = getStatusSuffixedIdentifier(base);
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
Type *TypeList::operator,(Type &otherType) {
	if (otherType.category == CATEGORY_TYPELIST) {
		return errType;
	} else if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		if (list.size() == 1 && (list[0])->category == CATEGORY_STDTYPE && ( (list[0])->suffix == SUFFIX_CONSTANT || (list[0])->suffix == SUFFIX_LATCH )) {
			StdType *thisTypeCast = (StdType *)(list[0]);
			if (otherTypeCast->kind == STD_NOT) {
				if (thisTypeCast->kind == STD_BOOL) {
					return (new StdType(STD_BOOL, SUFFIX_LATCH));
				}
			} else if (otherTypeCast->kind == STD_COMPLEMENT) {
				if (thisTypeCast->kind == STD_INT) {
					return (new StdType(STD_INT, SUFFIX_LATCH));
				}
			} else if (otherTypeCast->kind == STD_DPLUS || otherTypeCast->kind == STD_DMINUS) {
				if (thisTypeCast->kind == STD_INT || thisTypeCast->kind == STD_FLOAT) {
					return (new StdType(thisTypeCast->kind, SUFFIX_LATCH));
				}
			} else if (otherTypeCast->kind == STD_PLUS || otherTypeCast->kind == STD_MINUS) {
				if (thisTypeCast->kind == STD_INT || thisTypeCast->kind == STD_FLOAT) {
					return (new StdType(thisTypeCast->kind, SUFFIX_LATCH));
				}
			}
		} else if (list.size() == 2 &&
				((list[0])->category == CATEGORY_STDTYPE && ( (list[0])->suffix == SUFFIX_CONSTANT || (list[0])->suffix == SUFFIX_LATCH )) &&
				((list[1])->category == CATEGORY_STDTYPE && ( (list[1])->suffix == SUFFIX_CONSTANT || (list[1])->suffix == SUFFIX_LATCH ))) {
			StdType *thisTypeCast1 = (StdType *)(list[0]);
			StdType *thisTypeCast2 = (StdType *)(list[1]);
			if (otherTypeCast->kind == STD_DOR || otherTypeCast->kind == STD_DAND) {
				if (thisTypeCast1->kind == STD_BOOL && thisTypeCast2->kind == STD_BOOL) {
					return (new StdType(STD_BOOL, SUFFIX_LATCH));
				}
			} else if (otherTypeCast->kind == STD_OR || otherTypeCast->kind == STD_XOR || otherTypeCast->kind == STD_AND) {
				if (thisTypeCast1->kind == STD_INT && thisTypeCast2->kind == STD_INT) {
					return (new StdType(STD_INT, SUFFIX_LATCH));
				}
			} else if (otherTypeCast->kind == STD_DEQUALS || otherTypeCast->kind == STD_NEQUALS ||
					otherTypeCast->kind == STD_LT || otherTypeCast->kind == STD_GT || otherTypeCast->kind == STD_LE || otherTypeCast->kind == STD_GE) {
				if (thisTypeCast1->isComparable() && thisTypeCast2->isComparable()) {
					if (thisTypeCast1->kind >= thisTypeCast2->kind) {
						return (new StdType(thisTypeCast1->kind, SUFFIX_LATCH));
					} else {
						return (new StdType(thisTypeCast2->kind, SUFFIX_LATCH));
					}
				}
			} else if (otherTypeCast->kind == STD_TIMES || otherTypeCast->kind == STD_DIVIDE || otherTypeCast->kind == STD_MOD ||
					otherTypeCast->kind == STD_PLUS || otherTypeCast->kind == STD_MINUS) {
				if (thisTypeCast1->kind == STD_INT && thisTypeCast2->kind == STD_INT) {
					return (new StdType(STD_INT, SUFFIX_LATCH));
				} else if ((thisTypeCast1->kind == STD_INT || thisTypeCast1->kind == STD_FLOAT) && // if the first one is an int or a float
						(thisTypeCast2->kind == STD_INT || thisTypeCast2->kind == STD_FLOAT) && // and the other one is an int or a float
						(thisTypeCast1->kind == STD_FLOAT || thisTypeCast2->kind == STD_FLOAT)) { // and at least one of the two is a float
					return (new StdType(STD_FLOAT, SUFFIX_LATCH));
				}
			}
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (*(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
Type *TypeList::operator>>(Type &otherType) {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (list.size() != otherTypeCast->list.size()) {
			return errType;
		}
		vector<Type *>::iterator iter1 = list.begin();
		vector<Type *>::iterator iter2 = otherTypeCast->list.begin();
		while (iter1 != list.end() && iter2 != otherTypeCast->list.end()) {
			if (!(*(**iter1 >> **iter2))) {
				return errType;
			}
			iter1++;
			iter2++;
		}
		return nullType;
	} else if (otherType.category == CATEGORY_STDTYPE) {
		if (list.size() == 1) {
			return (*(list[0]) >> otherType);
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return nullType;
			}
		}
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
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
Type *ErrorType::operator,(Type &otherType) {return this;}
Type *ErrorType::operator>>(Type &otherType) {return this;}
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
Type *StdType::operator,(Type &otherType) { // KOL
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1) {
			return (*this , *(otherTypeCast->list[0]));
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		if (isComparable() && otherTypeCast->isComparable() && kind <= otherTypeCast->kind && baseSendable(otherType)) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (*(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
Type *StdType::operator>>(Type &otherType) { // KOL
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*(*this >> *(otherTypeCast->list[0])))) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		if (isComparable() && otherTypeCast->isComparable() && kind <= otherTypeCast->kind && baseSendable(otherType)) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (*(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
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
	if (to != NULL) {
		if (to->category == CATEGORY_TYPELIST) {
			to = (TypeList *)to;
		} else {
			to = new TypeList(to);
		}
	} else {
		to = new TypeList();
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
Type *FilterType::operator,(Type &otherType) { // KOL
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*(*this >> *(otherTypeCast->list[0])))) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		// check if the target accepts this filter as input
		if (*(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		}
		// otherwise, check if if this filter is sendable to the target
		if (*this == otherType && baseSendable(otherType)) {
			return &otherType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		// if the target accepts this filter as input
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
Type *FilterType::operator>>(Type &otherType) { // KOL
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*(*this >> *(otherTypeCast->list[0])))) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		// check if the target accepts this filter as input
		if (*(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		}
		// otherwise, check if if this filter is sendable to the target
		if (*this == otherType && baseSendable(otherType)) {
			return &otherType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		// if the target accepts this filter as input
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
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
ObjectType::ObjectType(SymbolTable *base, Tree *recall, int suffix, int depth) : base(base) {
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
			Type *childType = getStatusDeclaration(pipe->child);
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
Type *ObjectType::operator,(Type &otherType) { // KOL
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*(*this >> *(otherTypeCast->list[0])))) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		return (*this >> *(otherTypeCast->from));
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		// check if the target accepts this object as input
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
		// otherwise, check if if this object is sendable to the target
		if (*this == otherType && baseSendable(otherType)) {
			return &otherType;
		} else {
			return errType;
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
Type *ObjectType::operator>>(Type &otherType) { // KOL
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*(*this >> *(otherTypeCast->list[0])))) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		return (*this >> *(otherTypeCast->from));
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		// check if the target accepts this object as input
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
		// otherwise, check if if this object is sendable to the target
		if (*this == otherType && baseSendable(otherType)) {
			return &otherType;
		} else {
			return errType;
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
ObjectType::operator string() {return base->id;}

// typing status block functions
TypeStatus::TypeStatus() : type(nullType), recall(NULL) {}
TypeStatus::TypeStatus(Type *type, Tree *recall) : type(type), recall(recall) {}
TypeStatus::~TypeStatus() {}
TypeStatus::operator Type *() {return type;}
TypeStatus::operator Tree *() {return recall;}
TypeStatus::operator bool() {return (type != NULL);}
TypeStatus &TypeStatus::operator=(TypeStatus otherStatus) {type = otherStatus.type; recall = otherStatus.recall; return *this;}
Type &TypeStatus::operator*() {
	if (type != NULL) {
		return (*type);
	} else {
		return (*errType);
	}
}
bool TypeStatus::operator==(Type &otherType) {return (*type == otherType);}
bool TypeStatus::operator!=(Type &otherType) {return (*type != otherType);}
