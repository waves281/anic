#include "types.h"

// Type functions
bool Type::baseEquals(const Type &otherType) const {return (suffix == otherType.suffix && depth == otherType.depth);}
bool Type::baseSendable(const Type &otherType) const {
	return (
		(suffix == SUFFIX_CONSTANT && otherType.suffix == SUFFIX_CONSTANT) ||
		(suffix == SUFFIX_LATCH && (otherType.suffix == SUFFIX_LATCH || otherType.suffix == SUFFIX_STREAM)) ||
		(suffix == SUFFIX_ARRAY && otherType.suffix == SUFFIX_ARRAY && depth == otherType.depth) ||
		(suffix == SUFFIX_POOL && (otherType.suffix == SUFFIX_POOL || otherType.suffix == SUFFIX_ARRAY) && depth == otherType.depth)
	);
}
Type::~Type() {}
bool Type::constant() {
	if (suffix == SUFFIX_CONSTANT) {
		return true;
	} else if (suffix == SUFFIX_LATCH) {
		suffix = SUFFIX_CONSTANT;
		return true;
	} else if (suffix == SUFFIX_LIST) {
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		suffix = SUFFIX_LIST;
		return true;
	} else if (suffix == SUFFIX_ARRAY) {
		return true;
	} else if (suffix == SUFFIX_POOL) {
		suffix = SUFFIX_ARRAY;
		return true;
	}
	// can't happen
	return false;
}
bool Type::delatch() {
	if (suffix == SUFFIX_CONSTANT) {
		return false;
	} else if (suffix == SUFFIX_LATCH) {
		return true;
	} else if (suffix == SUFFIX_LIST) {
		suffix = SUFFIX_CONSTANT;
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		suffix = SUFFIX_LATCH;
		return true;
	} else if (suffix == SUFFIX_ARRAY) {
		return false;
	} else if (suffix == SUFFIX_POOL) {
		return true;
	}
	// can't happen
	return false;
}
bool Type::copyDelatch() {
	if (suffix == SUFFIX_CONSTANT) {
		suffix = SUFFIX_LATCH;
		return true;
	} else if (suffix == SUFFIX_LATCH) {
		return true;
	} else if (suffix == SUFFIX_LIST) {
		suffix = SUFFIX_LATCH;
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		suffix = SUFFIX_LATCH;
		return true;
	} else if (suffix == SUFFIX_ARRAY) {
		suffix = SUFFIX_POOL;
		return true;
	} else if (suffix == SUFFIX_POOL) {
		return true;
	}
	// can't happen
	return false;
}
bool Type::destream() {
	if (suffix == SUFFIX_CONSTANT) {
		return false;
	} else if (suffix == SUFFIX_LATCH) {
		return false;
	} else if (suffix == SUFFIX_LIST) {
		suffix = SUFFIX_CONSTANT;
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		suffix = SUFFIX_LATCH;
		return true;
	} else if (suffix == SUFFIX_ARRAY) {
		if (depth == 1) {
			depth = 0;
			suffix = SUFFIX_CONSTANT;
			return true;
		} else {
			return false;
		}
	} else if (suffix == SUFFIX_POOL) {
		if (depth == 1) {
			depth = 0;
			suffix = SUFFIX_LATCH;
			return true;
		} else {
			return false;
		}
	}
	// can't happen
	return false;
}
bool Type::copyDestream() {
	if (suffix == SUFFIX_CONSTANT) {
		return false;
	} else if (suffix == SUFFIX_LATCH) {
		return false;
	} else if (suffix == SUFFIX_LIST) {
		suffix = SUFFIX_LATCH;
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		suffix = SUFFIX_LATCH;
		return true;
	} else if (suffix == SUFFIX_ARRAY) {
		if (depth == 1) {
			depth = 0;
			suffix = SUFFIX_LATCH;
			return true;
		} else {
			return false;
		}
	} else if (suffix == SUFFIX_POOL) {
		if (depth == 1) {
			depth = 0;
			suffix = SUFFIX_LATCH;
			return true;
		} else {
			return false;
		}
	}
	// can't happen
	return false;
}
Type::operator bool() const {return (category != CATEGORY_ERRORTYPE);}
bool Type::operator!() const {return (category == CATEGORY_ERRORTYPE);}
bool Type::operator!=(const Type &otherType) const {return (!operator==(otherType));};

// TypeList functions
// constructor works on ParamList and TypeList
TypeList::TypeList(const vector<Type *> &list) : list(list) {category = CATEGORY_TYPELIST; suffix = SUFFIX_CONSTANT; depth = 0; toStringHandled = false;}
TypeList::TypeList(Type *type) {
	category = CATEGORY_TYPELIST; suffix = SUFFIX_CONSTANT; depth = 0; toStringHandled = false;
	list.push_back(type);
}
TypeList::TypeList() {category = CATEGORY_TYPELIST; suffix = SUFFIX_CONSTANT; depth = 0; toStringHandled = false;}
TypeList::~TypeList() {
	for (vector<Type *>::iterator iter = list.begin(); iter != list.end(); iter++) {
		if (**iter != *nullType && **iter != *errType) {
			delete (*iter);
		}
	}
}
bool TypeList::isComparable(const Type &otherType) const {return (list.size() == 1 && list[0]->isComparable(otherType));}
Type *TypeList::copy() const {return new TypeList(*this);}
bool TypeList::operator==(const Type &otherType) const {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (list.size() != otherTypeCast->list.size()) {
			return false;
		}
		vector<Type *>::const_iterator iter1 = list.begin();
		vector<Type *>::const_iterator iter2 = otherTypeCast->list.begin();
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
Type *TypeList::operator,(Type &otherType) const {
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
				if (thisTypeCast1->kindCompare(*thisTypeCast2)) {
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
		for (vector<TypeList *>::const_iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
Type *TypeList::operator>>(Type &otherType) const {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (list.size() != otherTypeCast->list.size()) {
			return errType;
		}
		vector<Type *>::const_iterator iter1 = list.begin();
		vector<Type *>::const_iterator iter2 = otherTypeCast->list.begin();
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
		for (vector<TypeList *>::const_iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
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
	TYPE_TO_STRING_HEADER;
	for (vector<Type *>::const_iterator iter = list.begin(); iter != list.end(); iter++) {
		acc += (string)(**iter);
		if ((iter+1) != list.end()) {
			acc += ", ";
		}
	}
	TYPE_TO_STRING_FOOTER;
}

// ErrorType functions
ErrorType::ErrorType() {category = CATEGORY_ERRORTYPE; toStringHandled = false;}
ErrorType::~ErrorType() {}
bool ErrorType::isComparable(const Type &otherType) const {return false;}
Type *ErrorType::copy() const {return new ErrorType(*this);}
bool ErrorType::operator==(const Type &otherType) const {
	if (otherType.category == CATEGORY_ERRORTYPE) {
		return (this == &otherType);
	} else {
		return false;
	}
}
Type *ErrorType::operator,(Type &otherType) const {return errType;}
Type *ErrorType::operator>>(Type &otherType) const {return errType;}
ErrorType::operator string() {return "error";}

// StdType functions
StdType::StdType(int kind, int suffix, int depth) : kind(kind) {category = CATEGORY_STDTYPE; this->suffix = suffix; this->depth = depth; toStringHandled = false;}
StdType::~StdType() {}
bool StdType::isComparable(const Type &otherType) const {return (otherType.category == CATEGORY_STDTYPE && ( kindCompare(*((StdType *)(&otherType))) || ((StdType *)(&otherType))->kindCompare(*this) ));}
int StdType::kindCompare(const StdType &otherType) const {
	if (!(kind >= STD_MIN_COMPARABLE && kind <= STD_MAX_COMPARABLE && otherType.kind >= STD_MIN_COMPARABLE && otherType.kind <= STD_MAX_COMPARABLE)) {
		return STD_NULL;
	} else if (kind == otherType.kind) {
		return kind;
	} else if (kind < otherType.kind) {
		if (kind == STD_INT && otherType.kind == STD_FLOAT) {
			return STD_FLOAT;
		} else if (kind == STD_INT && otherType.kind == STD_CHAR) {
			return STD_CHAR;
		} else if (kind == STD_CHAR && otherType.kind == STD_STRING) {
			return STD_STRING;
		} else {
			return STD_NULL;
		}
	} else { // kind > otherType.kind
		return STD_NULL;
	}
}
Type *StdType::copy() const {return new StdType(*this);}
bool StdType::operator==(const Type &otherType) const {
	if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		return (kind == otherTypeCast->kind && baseEquals(otherType));
	} else {
		return false;
	}
}
Type *StdType::operator,(Type &otherType) const {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1) {
			return (*this , *(otherTypeCast->list[0]));
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (*(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		for (vector<TypeList *>::const_iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
Type *StdType::operator>>(Type &otherType) const {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && *(*this >> *(otherTypeCast->list[0]))) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		if (baseSendable(otherType) && kindCompare(*otherTypeCast) && kind <= otherTypeCast->kind) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
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
	category = CATEGORY_FILTERTYPE; this->suffix = suffix; this->depth = depth; toStringHandled = false;
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
bool FilterType::isComparable(const Type &otherType) const {return false;}
Type *FilterType::copy() const {return new FilterType(*this);}
bool FilterType::operator==(const Type &otherType) const {
	if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		return (*from == *(otherTypeCast->from) && *to == *(otherTypeCast->to) && baseEquals(otherType));
	} else {
		return false;
	}
}
Type *FilterType::operator,(Type &otherType) const {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1) {
			return (*this , *(otherTypeCast->list[0]));
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (*(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		for (vector<TypeList *>::const_iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
Type *FilterType::operator>>(Type &otherType) const {
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
		if (baseSendable(otherType) && operator==(otherType)) {
			return &otherType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
FilterType::operator string() {
	TYPE_TO_STRING_HEADER;
	acc = "[";
	acc += (string)(*from);
	acc += " --> ";
	acc += (string)(*to);
	acc += "]";
	TYPE_TO_STRING_FOOTER;
}

// ObjectType functions
ObjectType::ObjectType(int suffix, int depth) {category = CATEGORY_OBJECTTYPE; this->suffix = suffix; this->depth = depth; toStringHandled = false;}
ObjectType::ObjectType(const vector<TypeList *> &constructorTypes, int suffix, int depth) : constructorTypes(constructorTypes)
	{category = CATEGORY_OBJECTTYPE; this->suffix = suffix; this->depth = depth; toStringHandled = false;}
ObjectType::ObjectType(const vector<TypeList *> &constructorTypes, const vector<string> &memberNames, const vector<Type *> &memberTypes, int suffix, int depth) : 
	constructorTypes(constructorTypes), memberNames(memberNames), memberTypes(memberTypes) {
	category = CATEGORY_OBJECTTYPE; this->suffix = suffix; this->depth = depth; toStringHandled = false;
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
bool ObjectType::isComparable(const Type &otherType) const {return false;}
Type *ObjectType::copy() const {return new ObjectType(*this);}
bool ObjectType::operator==(const Type &otherType) const {
	if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		if (constructorTypes.size() == otherTypeCast->constructorTypes.size() && memberNames.size() == otherTypeCast->memberNames.size()) {
			// verify that the constructors match
			vector<TypeList *>::const_iterator consIter1 = constructorTypes.begin();
			vector<TypeList *>::const_iterator consIter2 = otherTypeCast->constructorTypes.begin();
			while (consIter1 != constructorTypes.end() && consIter2 != otherTypeCast->constructorTypes.end()) {
				if (**consIter1 != **consIter2) {
					return false;
				}
				// advance
				consIter1++;
				consIter2++;
			}
			// verify that the member types match
			vector<string>::const_iterator memberNameIter1 = memberNames.begin();
			vector<string>::const_iterator memberNameIter2 = otherTypeCast->memberNames.begin();
			while (memberNameIter1 != memberNames.end() && memberNameIter2 != otherTypeCast->memberNames.end()) {
				if (*memberNameIter1 != *memberNameIter2) {
					return false;
				}
				// advance
				memberNameIter1++;
				memberNameIter2++;
			}
			// verify that the member types match
			vector<Type *>::const_iterator memberTypeIter1 = memberTypes.begin();
			vector<Type *>::const_iterator memberTypeIter2 = otherTypeCast->memberTypes.begin();
			while (memberTypeIter1 != memberTypes.end() && memberTypeIter2 != otherTypeCast->memberTypes.end()) {
				if (**memberTypeIter1 != **memberTypeIter2) {
					return false;
				}
				// advance
				memberTypeIter1++;
				memberTypeIter2++;
			}
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}
Type *ObjectType::operator,(Type &otherType) const {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1) {
			return (*this , *(otherTypeCast->list[0]));
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (*(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		for (vector<TypeList *>::const_iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
Type *ObjectType::operator>>(Type &otherType) const {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && *(*this >> *(otherTypeCast->list[0]))) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		if (baseSendable(otherType) && operator==(otherType)) {
			return &otherType;
		} else {
			return errType;
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
ObjectType::operator string() {
	TYPE_TO_STRING_HEADER;
	acc = "{";
	for (vector<TypeList *>::const_iterator iter = constructorTypes.begin(); iter != constructorTypes.end(); iter++) {
		acc += "=[";
		acc += (string)(**iter);
		acc += "]";
		if (iter+1 != constructorTypes.end()) {
			acc += ", ";
		}
	}
	acc += ";";
	vector<string>::const_iterator memberNameIter = memberNames.begin();
	vector<Type *>::const_iterator memberTypeIter = memberTypes.begin();
	while (memberNameIter != memberNames.end()) {
		acc += *memberNameIter;
		acc += "=";
		acc += (string)(**memberTypeIter);
		if (memberNameIter+1 != memberNames.end()) {
			acc += ", ";
		}
		// advance
		memberNameIter++;
		memberTypeIter++;
	}
	acc += "}";
	TYPE_TO_STRING_FOOTER;
}

// typing status block functions
TypeStatus::TypeStatus(Type *type, Tree *recall, Type *retType) : type(type), recall(recall), retType(retType) {}
TypeStatus::TypeStatus(Type *type, const TypeStatus &otherStatus) : type(type), recall(otherStatus.recall), retType(otherStatus.retType) {}
TypeStatus::~TypeStatus() {}
TypeStatus::operator Type *() const {return type;}
TypeStatus &TypeStatus::operator=(const TypeStatus &otherStatus) {type = otherStatus.type; recall = otherStatus.recall; retType = otherStatus.retType; return *this;}
TypeStatus &TypeStatus::operator=(Type *otherType) {type = otherType; return *this;}
TypeStatus &TypeStatus::operator=(Tree *otherTree) {recall = otherTree; return *this;}
Type &TypeStatus::operator*() const {
	if (type != NULL) {
		return (*type);
	} else {
		return (*errType);
	}
}
Type *TypeStatus::operator->() const {return type;}
bool TypeStatus::operator==(const Type &otherType) const {return (*type == otherType);}
bool TypeStatus::operator!=(const Type &otherType) const {return (*type != otherType);}
