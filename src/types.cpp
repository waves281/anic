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
TypeList(vector<Type *> &list) : list(list) {category = CATEGORY_TYPELIST; suffix = SUFFIX_CONSTANT; depth = 0;}
TypeList::TypeList(Type *type) {
	category = CATEGORY_TYPELIST; suffix = SUFFIX_CONSTANT; depth = 0;
	list.push_back(type);
}
TypeList::TypeList() {category = CATEGORY_TYPELIST; suffix = SUFFIX_CONSTANT; depth = 0;}
TypeList::~TypeList() {
	for (vector<Type *>::iterator iter = list.begin(); iter != list.end(); iter++) {
		if (**iter != *nullType && **iter != *errType) {
			delete (*iter);
		}
	}
}
bool TypeList::isComparable(Type &otherType) {return (list.size() == 1 && list[0]->isComparable(otherType));}
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
ErrorType::~ErrorType() {}
bool ErrorType::isComparable(Type &otherType) {return false;}
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
bool StdType::isComparable(Type &otherType) {return (otherType.category == CATEGORY_STDTYPE && ( kindCompare(*((StdType *)(&otherType))) || ((StdType *)(&otherType))->kindCompare(*this) ));}
int StdType::kindCompare(StdType &otherType) {
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
bool StdType::operator==(Type &otherType) {
	if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		return (kind == otherTypeCast->kind && baseEquals(otherType));
	} else {
		return false;
	}
}
Type *StdType::operator,(Type &otherType) {
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
Type *StdType::operator>>(Type &otherType) {
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
bool FilterType::isComparable(Type &otherType) {return false;}
bool FilterType::operator==(Type &otherType) {
	if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		return (*from == *(otherTypeCast->from) && *to == *(otherTypeCast->to) && baseEquals(otherType));
	} else {
		return false;
	}
}
Type *FilterType::operator,(Type &otherType) {
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
Type *FilterType::operator>>(Type &otherType) {
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
		if (baseSendable(otherType) && *this == otherType) {
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
	string acc("[");
	acc += (string)(*from);
	acc += " --> ";
	acc += (string)(*to);
	acc += "]";
	return acc;
}

// ObjectType functions
// constructor works only if base->defSite is Declaration->TypedStaticTerm->Node->Object
ObjectType::ObjectType(vector<TypeList *> &constructorTypes, int suffix, int depth) : constructorTypes(constructorTypes) {category = CATEGORY_OBJECTTYPE; this->suffix = suffix; this->depth = depth;}
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
bool ObjectType::isComparable(Type &otherType) {return false;}
bool ObjectType::operator==(Type &otherType) {
	if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		if (constructorTypes.size() == otherTypeCast->constructorTypes.size() && memberNames.size() == otherTypeCast->memberNames.size()) {
			// verify that the constructors match
			vector<TypeList *>::iterator consIter1 = constructorTypes.begin();
			vector<TypeList *>::iterator consIter2 = otherTypeCast->constructorTypes.begin();
			while (consIter1 != constructorTypes.end() && consIter2 != otherTypeCast->constructorTypes.end()) {
				if (**consIter1 != **consIter2) {
					return false;
				}
				// advance
				consIter1++;
				consIter2++;
			}
			// verify that the member types match
			vector<string>::iterator memberNameIter1 = memberNames.begin();
			vector<string>::iterator memberNameIter2 = otherTypeCast->memberNames.begin();
			while (memberNameIter1 != memberNames.end() && memberNameIter2 != otherTypeCast->memberNames.end()) {
				if (*memberNameIter1 != *memberNameIter2) {
					return false;
				}
				// advance
				memberNameIter1++;
				memberNameIter2++;
			}
			// verify that the member types match
			vector<Type *>::iterator memberTypeIter1 = memberTypes.begin();
			vector<Type *>::iterator memberTypeIter2 = otherTypeCast->memberTypes.begin();
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
Type *ObjectType::operator,(Type &otherType) {
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
		for (vector<TypeList *>::iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
			if (*(*this >> **iter)) {
				return &otherType;
			}
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
Type *ObjectType::operator>>(Type &otherType) {
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
		if (baseSendable(otherType) && *this == otherType) {
			return &otherType;
		} else {
			return errType;
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
ObjectType::operator string() {
	string acc("{");
	for (vector<TypeList *>::iterator iter = constructorTypes.begin(); iter != constructorTypes.end(); iter++) {
		acc += "=[";
		acc += (string)(**iter);
		acc += "]";
		if (iter+1 != constructorTypes.end()) {
			acc += ", ";
		}
	}
	acc += ";";
	vector<string>::iterator memberNameIter = memberNames.begin();
	vector<Type *>::iterator memberTypeIter = memberTypes.begin();
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
	return acc;
}

// typing status block functions
TypeStatus::TypeStatus() : type(nullType), recall(NULL) {}
TypeStatus::TypeStatus(Type *type, Tree *recall) : type(type), recall(recall) {}
TypeStatus::~TypeStatus() {}
TypeStatus::operator Type *() {return type;}
TypeStatus::operator Tree *() {return recall;}
TypeStatus::operator bool() {return (type != NULL);}
TypeStatus &TypeStatus::operator=(TypeStatus otherStatus) {type = otherStatus.type; recall = otherStatus.recall; return *this;}
TypeStatus &TypeStatus::operator=(Type *otherType) {type = otherType; return *this;}
TypeStatus &TypeStatus::operator=(Tree *otherTree) {recall = otherTree; return *this;}
Type &TypeStatus::operator*() {
	if (type != NULL) {
		return (*type);
	} else {
		return (*errType);
	}
}
Type *TypeStatus::operator->() {return type;}
bool TypeStatus::operator==(Type &otherType) {return (*type == otherType);}
bool TypeStatus::operator!=(Type &otherType) {return (*type != otherType);}
