#include "types.h"

// Type functions
Type::Type(int category, int suffix, int depth) : category(category), suffix(suffix), depth(depth), operable(true), toStringHandled(false) {}
bool Type::baseEquals(const Type &otherType) const {return (suffix == otherType.suffix && depth == otherType.depth);}
bool Type::baseSendable(const Type &otherType) const {
	return (
		(suffix == SUFFIX_CONSTANT && (otherType.suffix == SUFFIX_CONSTANT || otherType.suffix == SUFFIX_LIST)) ||
		(suffix == SUFFIX_LATCH && (otherType.suffix == SUFFIX_CONSTANT || otherType.suffix == SUFFIX_LATCH || otherType.suffix == SUFFIX_LIST || otherType.suffix == SUFFIX_STREAM)) ||
		(suffix == SUFFIX_ARRAY && otherType.suffix == SUFFIX_ARRAY && depth == otherType.depth) ||
		(suffix == SUFFIX_POOL && (otherType.suffix == SUFFIX_POOL || otherType.suffix == SUFFIX_ARRAY) && depth == otherType.depth)
	);
}
string Type::suffixString() const {
	string acc;
	if (suffix == SUFFIX_LATCH) {
		acc = '\\';
	} else if (suffix == SUFFIX_LIST) {
		acc = "[]";
	} else if (suffix == SUFFIX_STREAM) {
		acc = "\\\\";
	} else if (suffix == SUFFIX_ARRAY) {
		acc = "[.]";
		for (int i = 1; i < depth; i++) {
			acc += "[.]";
		}
	} else if (suffix == SUFFIX_POOL) {
		acc = "\\[.]";
		for (int i = 1; i < depth; i++) {
			acc += "[.]";
		}
	}
	return acc;
}
Type::~Type() {}
void Type::constantizeType() {
	if (suffix == SUFFIX_LATCH) {
		suffix = SUFFIX_CONSTANT;
	} else if (suffix == SUFFIX_STREAM) {
		suffix = SUFFIX_ARRAY;
	} else if (suffix == SUFFIX_POOL) {
		suffix = SUFFIX_ARRAY;
	}
}
bool Type::constantizeReference() {
	if (suffix == SUFFIX_CONSTANT) {
		return true;
	} else if (suffix == SUFFIX_LATCH) {
		suffix = SUFFIX_CONSTANT;
		return true;
	} else if (suffix == SUFFIX_LIST) {
		return false;
	} else if (suffix == SUFFIX_STREAM) {
		return false;
	} else if (suffix == SUFFIX_ARRAY) {
		return true;
	} else /* if (suffix == SUFFIX_POOL) */ {
		suffix = SUFFIX_ARRAY;
		return true;
	}
}
void Type::decreaseDepth() {
	if (suffix == SUFFIX_ARRAY) {
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_CONSTANT;
		}
	} else /* if (suffix == SUFFIX_POOL) */ {
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_LATCH;
		}
	}
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
	} else /* if (suffix == SUFFIX_POOL) */ {
		return true;
	}
}
bool Type::delist() {
	if (suffix == SUFFIX_CONSTANT) {
		return false;
	} else if (suffix == SUFFIX_LATCH) {
		return false;
	} else if (suffix == SUFFIX_LIST) {
		suffix = SUFFIX_CONSTANT;
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		suffix = SUFFIX_CONSTANT;
		return true;
	} else if (suffix == SUFFIX_ARRAY) {
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_CONSTANT;
		}
		return true;
	} else /* if (suffix == SUFFIX_POOL) */ {
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_CONSTANT;
		} else {
			suffix = SUFFIX_ARRAY;
		}
		return true;
	}
}
bool Type::destream() {
	if (suffix == SUFFIX_CONSTANT) {
		return false;
	} else if (suffix == SUFFIX_LATCH) {
		return false;
	} else if (suffix == SUFFIX_LIST) {
		return false;
	} else if (suffix == SUFFIX_STREAM) {
		suffix = SUFFIX_LATCH;
		return true;
	} else if (suffix == SUFFIX_ARRAY) {
		return false;
	} else /* if (suffix == SUFFIX_POOL) */ {
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_LATCH;
		}
		return true;
	}
}
bool Type::copyDelatch() {
	if (suffix == SUFFIX_CONSTANT) {
		suffix = SUFFIX_LATCH;
		return true;
	} else if (suffix == SUFFIX_LATCH) {
		return true;
	} else if (suffix == SUFFIX_LIST) {
		return false;
	} else if (suffix == SUFFIX_STREAM) {
		return false;
	} else if (suffix == SUFFIX_ARRAY) {
		return false;
	} else /* if (suffix == SUFFIX_POOL) */ {
		return false;
	}
}
bool Type::copyDelist() {
	if (suffix == SUFFIX_CONSTANT) {
		return false;
	} else if (suffix == SUFFIX_LATCH) {
		return false;
	} else if (suffix == SUFFIX_LIST) {
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		suffix = SUFFIX_LIST;
		return true;
	} else if (suffix == SUFFIX_ARRAY) {
		return true;
	} else /* if (suffix == SUFFIX_POOL) */ {
		suffix = SUFFIX_ARRAY;
		return true;
	}
}
bool Type::copyDestream() {
	if (suffix == SUFFIX_CONSTANT) {
		return false;
	} else if (suffix == SUFFIX_LATCH) {
		return false;
	} else if (suffix == SUFFIX_LIST) {
		suffix = SUFFIX_STREAM;
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		return true;
	} else if (suffix == SUFFIX_ARRAY) {
		suffix = SUFFIX_POOL;
		return false;
	} else /* if (suffix == SUFFIX_POOL) */ {
		return true;
	}
}
Type::operator bool() const {return (category != CATEGORY_ERRORTYPE);}
bool Type::operator!() const {return (category == CATEGORY_ERRORTYPE);}
bool Type::operator!=(const Type &otherType) const {return (!(operator==(otherType)));};
bool Type::operator!=(int kind) const {return (!(operator==(kind)));}

// TypeList functions
// constructor works on ParamList and TypeList
TypeList::TypeList(const vector<Type *> &list) : Type(CATEGORY_TYPELIST), list(list) {}
TypeList::TypeList(Type *type) : Type(CATEGORY_TYPELIST) {
	list.push_back(type);
}
TypeList::TypeList() : Type(CATEGORY_TYPELIST) {
	list.push_back(nullType);
}
TypeList::~TypeList() {
	for (vector<Type *>::iterator iter = list.begin(); iter != list.end(); iter++) {
		if (**iter != *nullType && **iter != *errType && !((*iter)->operable)) {
			delete (*iter);
		}
	}
}
bool TypeList::isComparable(const Type &otherType) const {return (list.size() == 1 && list[0]->isComparable(otherType));}
Type *TypeList::copy() {Type *retVal = new TypeList(*this); retVal->operable = true; return retVal;}
void TypeList::erase() {list.clear(); delete this;}
bool TypeList::operator==(const Type &otherType) const {
	if (this == &otherType) { // if the lists are actually the same object instance, return true
		return true;
	} else if (otherType.category == CATEGORY_TYPELIST) {
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
	} else if (list.size() == 1) {
		return (*(list[0]) == otherType);
	} else {
		return false;
	}
}
bool TypeList::operator==(int kind) const {return (list.size() == 1 && list[0]->category == CATEGORY_STDTYPE && ((StdType *)(list[0]))->kind == kind);}
Type *TypeList::operator,(const Type &otherType) const {
	if (otherType.category == CATEGORY_TYPELIST) {
		return errType;
	} else if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		if (list.size() == 1 && (list[0])->category == CATEGORY_STDTYPE && ((list[0])->suffix == SUFFIX_CONSTANT || (list[0])->suffix == SUFFIX_LATCH)) {
			StdType *thisTypeCast = (StdType *)(list[0]);
			if (otherTypeCast->kind == STD_NOT && *(*thisTypeCast >> *stdBoolType)) {
				return (new StdType(STD_BOOL, SUFFIX_LATCH));
			} else if (otherTypeCast->kind == STD_COMPLEMENT && *(*thisTypeCast >> *stdIntType)) {
				return (new StdType(STD_INT, SUFFIX_LATCH));
			} else if ((otherTypeCast->kind == STD_DPLUS || otherTypeCast->kind == STD_DMINUS) &&
					*(*thisTypeCast >> *stdIntType)) {
				return (new StdType(STD_INT, SUFFIX_LATCH));
			} else if (otherTypeCast->kind == STD_PLUS || otherTypeCast->kind == STD_MINUS) {
				if (*thisTypeCast >> *stdIntType) {
					return (new StdType(STD_INT, SUFFIX_LATCH));
				} else if (*thisTypeCast >> *stdFloatType){
					return (new StdType(STD_FLOAT, SUFFIX_LATCH));
				} else {
					return errType;
				}
			} else {
				return errType;
			}
		} else if (list.size() == 2 &&
				((list[0])->category == CATEGORY_STDTYPE && ((list[0])->suffix == SUFFIX_CONSTANT || (list[0])->suffix == SUFFIX_LATCH)) &&
				((list[1])->category == CATEGORY_STDTYPE && ((list[1])->suffix == SUFFIX_CONSTANT || (list[1])->suffix == SUFFIX_LATCH))) {
			StdType *thisTypeCast1 = (StdType *)(list[0]);
			StdType *thisTypeCast2 = (StdType *)(list[1]);
			if ((otherTypeCast->kind == STD_DOR || otherTypeCast->kind == STD_DAND) &&
					(*(*thisTypeCast1 >> *stdBoolType) && *(*thisTypeCast2 >> *stdBoolType))) {
				return (new StdType(STD_BOOL, SUFFIX_LATCH));
			} else if ((otherTypeCast->kind == STD_OR || otherTypeCast->kind == STD_XOR || otherTypeCast->kind == STD_AND) &&
					(*(*thisTypeCast1 >> *stdIntType) && *(*thisTypeCast2 >> *stdIntType))) {
				return (new StdType(STD_INT, SUFFIX_LATCH));
			} else if (otherTypeCast->kind == STD_DEQUALS || otherTypeCast->kind == STD_NEQUALS ||
					otherTypeCast->kind == STD_LT || otherTypeCast->kind == STD_GT ||
					otherTypeCast->kind == STD_LE || otherTypeCast->kind == STD_GE) {
				if (thisTypeCast1->kindCompare(*thisTypeCast2)) {
					return (new StdType(STD_BOOL, SUFFIX_LATCH));
				} else {
					return errType;
				}
			} else if (otherTypeCast->kind == STD_TIMES || otherTypeCast->kind == STD_DIVIDE || otherTypeCast->kind == STD_MOD ||
					otherTypeCast->kind == STD_PLUS || otherTypeCast->kind == STD_MINUS) {
				if (*(*thisTypeCast1 >> *stdIntType) && *(*thisTypeCast2 >> *stdIntType)) {
					return (new StdType(STD_INT, SUFFIX_LATCH));
				} else if (*(*thisTypeCast1 >> *stdFloatType) && *(*thisTypeCast2 >> *stdFloatType)) {
					return (new StdType(STD_FLOAT, SUFFIX_LATCH));
				} else {
					return errType;
				}
			}
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (otherTypeCast->suffix == SUFFIX_LATCH && *(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
TypeStatusBase TypeList::operator>>(const Type &otherType) const {
	if (this == &otherType) { // if the lists are actually the same object instance
		return nullType;
	} else if (otherType.category == CATEGORY_TYPELIST) {
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
		if (list.size() == 1) {
			return (*(list[0]) >> otherType);
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		if (list.size() == 1) {
			return (*(list[0]) >> otherType);
		} else if (otherType.suffix == SUFFIX_LATCH || otherType.suffix == SUFFIX_STREAM) {
			ObjectType *otherTypeCast = (ObjectType *)(&otherType);
			for (vector<TypeList *>::const_iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
				if (*(*this >> **iter)) {
					return nullType;
				}
			}
		} else {
			return errType;
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
string TypeList::toString(unsigned int tabDepth) {
	TYPE_TO_STRING_HEADER;
	for (vector<Type *>::const_iterator iter = list.begin(); iter != list.end(); iter++) {
		acc += (*iter)->toString(tabDepth);
		if ((iter+1) != list.end()) {
			acc += ", ";
		}
	}
	TYPE_TO_STRING_FOOTER;
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
ErrorType::ErrorType() : Type(CATEGORY_ERRORTYPE) {}
ErrorType::~ErrorType() {}
bool ErrorType::isComparable(const Type &otherType) const {return false;}
Type *ErrorType::copy() {Type *retVal = new ErrorType(*this); retVal->operable = true; return retVal;}
void ErrorType::erase() {delete this;}
bool ErrorType::operator==(const Type &otherType) const {
	if (otherType.category == CATEGORY_ERRORTYPE) {
		return (this == &otherType);
	} else {
		return false;
	}
}
bool ErrorType::operator==(int kind) const {return false;}
Type *ErrorType::operator,(const Type &otherType) const {return errType;}
TypeStatusBase ErrorType::operator>>(const Type &otherType) const {return errType;}
string ErrorType::toString(unsigned int tabDepth) {
	return "<ERROR>";
}
ErrorType::operator string() {return toString(1);}

// StdType functions
StdType::StdType(int kind, int suffix, int depth) : Type(CATEGORY_STDTYPE, suffix, depth), kind(kind) {}
StdType::~StdType() {}
bool StdType::isComparable(const Type &otherType) const {
	return (otherType.category == CATEGORY_STDTYPE && (kindCompare(*((StdType *)(&otherType))) || ((StdType *)(&otherType))->kindCompare(*this)));
}
int StdType::kindCompare(const StdType &otherType) const {
	if (kind < STD_MIN_COMPARABLE || kind > STD_MAX_COMPARABLE || otherType.kind < STD_MIN_COMPARABLE || otherType.kind > STD_MAX_COMPARABLE) {
		return STD_NULL;
	} else if (kind == otherType.kind) {
		return kind;
	} else if (kind == STD_INT && otherType.kind == STD_FLOAT) {
		return STD_FLOAT;
	} else if (kind == STD_INT && otherType.kind == STD_CHAR) {
		return STD_CHAR;
	} else if (otherType.kind == STD_STRING) {
		return STD_STRING;
	} else {
		return STD_NULL;
	}
}
pair<Type *, bool> StdType::stdFlowDerivation(const TypeStatus &prevTermStatus, Tree *nextTerm) const {
	// derive the nextTermStatus if we'll subsequently need it
	TypeStatus nextTermStatus = errType;
	switch(kind) {
		case STD_DEQUALS:
		case STD_NEQUALS:
		case STD_LT:
		case STD_GT:
		case STD_LE:
		case STD_GE:
		case STD_LS:
		case STD_RS:
		case STD_TIMES:
		case STD_DIVIDE:
		case STD_MOD:
		case STD_PLUS:
		case STD_MINUS:
			if (nextTerm != NULL &&
					*(nextTerm->child->child) == TOKEN_SimpleTerm &&
					*(nextTerm->child->child->child) == TOKEN_StaticTerm) {
				nextTermStatus = getStatusTerm(nextTerm, prevTermStatus);	
			}
			break;
		default:
			break;
	}
	// do the actual exceptional derivation tests
	switch(kind) {
		case STD_DEQUALS:
		case STD_NEQUALS:
		case STD_LT:
		case STD_GT:
		case STD_LE:
		case STD_GE:
			if (*nextTermStatus) {
				if (prevTermStatus.type->category == CATEGORY_STDTYPE && (prevTermStatus.type->suffix == SUFFIX_CONSTANT || prevTermStatus.type->suffix == SUFFIX_LATCH) &&
						nextTermStatus.type->category == CATEGORY_STDTYPE && (nextTermStatus.type->suffix == SUFFIX_CONSTANT || nextTermStatus.type->suffix == SUFFIX_LATCH) &&
						((StdType *)(prevTermStatus.type))->kindCompare(*((StdType *)(nextTermStatus.type)))) { // if the terms are comparable, return bool
					return make_pair(new StdType(STD_BOOL, SUFFIX_LATCH), true); // return true, since we're consuming the nextTerm
				}
			}
			break;
		case STD_LS:
		case STD_RS:
			if (*(*prevTermStatus >> *stdIntType) && *(*nextTermStatus >> *stdIntType)) { // if both terms can be converted to int, return int
				return make_pair(new StdType(STD_INT, SUFFIX_LATCH), true); // return true, since we're consuming the nextTerm
			}
			break;
		case STD_TIMES:
		case STD_DIVIDE:
		case STD_MOD:
		case STD_PLUS:
		case STD_MINUS:
			if (*nextTermStatus) {
				if (*(*prevTermStatus >> *stdIntType) && *(*nextTermStatus >> *stdIntType)) { // if both terms can be converted to int, return int
					return make_pair(new StdType(STD_INT, SUFFIX_LATCH), true); // return true, since we're consuming the nextTerm
				}
				if (*(*prevTermStatus >> *stdFloatType) && *(*nextTermStatus >> *stdFloatType)) { // if both terms can be converted to float, return float
					return make_pair(new StdType(STD_FLOAT, SUFFIX_LATCH), true); // return true, since we're consuming the nextTerm
				}
				// if this is the + operator and one of the terms is a string and the other is a StdType constant or latch, return string
				if (kind == STD_PLUS &&
						((*(*prevTermStatus >> *stdStringType) && nextTermStatus->category == CATEGORY_STDTYPE &&
							(nextTermStatus->suffix == SUFFIX_CONSTANT || nextTermStatus->suffix == SUFFIX_LATCH)) ||
						(*(*nextTermStatus >> *stdStringType) && prevTermStatus->category == CATEGORY_STDTYPE &&
							(prevTermStatus->suffix == SUFFIX_CONSTANT || prevTermStatus->suffix == SUFFIX_LATCH)))) {
					return make_pair(new StdType(STD_STRING, SUFFIX_LATCH), true); // return true, since were consuming the nextTerm
				}
			}
			// if we got here, we failed to derive a three-term type, so now we try using STD_PLUS and STD_MINUS in their unary form
			if (kind == STD_PLUS || kind == STD_MINUS) { // if it's an operator with a unary form that accepts both ints an floats
				if (*(*prevTermStatus >> *stdIntType)) { // if both terms can be converted to int, return int
					return make_pair(new StdType(STD_INT, SUFFIX_LATCH), false); // return false, since we're not consuming the nextTerm
				}
				if (*(*prevTermStatus >> *stdFloatType)) { // if both terms can be converted to float, return float
					return make_pair(new StdType(STD_FLOAT, SUFFIX_LATCH), false); // return false, since we're not consuming the nextTerm
				}
			}
			break;
		case STD_DPLUS:
		case STD_DMINUS:
			if (*(*prevTermStatus >> *stdIntType)) { // if both terms can be converted to int, return int
				return make_pair(new StdType(STD_INT, SUFFIX_LATCH), false); // return false, since we're not consuming the nextTerm
			}
			break;
		default:
			break;
	}
	return make_pair(errType, false); // return false, since we're not consuming the nextTerm (though this doesn't really matter -- it's an error anyway)
}
bool StdType::filterTypePromotion(const FilterType &otherType) const {
	if (otherType == *boolUnOpType &&
			kind == STD_NOT) {
		return true;
	} else if (otherType == *intUnOpType &&
			(kind == STD_COMPLEMENT || kind == STD_DPLUS || kind == STD_DMINUS || kind == STD_PLUS || kind == STD_MINUS)) {
		return true;
	} else if (otherType == *boolBinOpType &&
			(kind == STD_DOR || kind == STD_DAND)) {
		return true;
	} else if (otherType == *intBinOpType &&
			(kind == STD_OR || kind == STD_AND || kind == STD_XOR || kind == STD_PLUS || kind == STD_MINUS || kind == STD_TIMES || kind == STD_DIVIDE || kind == STD_MOD || kind == STD_LS || kind == STD_RS)) {
		return true;
	} else if (otherType == *floatBinOpType &&
			(kind == STD_PLUS || kind == STD_MINUS || kind == STD_TIMES || kind == STD_DIVIDE || kind == STD_MOD)) {
		return true;
	} else if ((otherType == *boolCompOpType || otherType == *intCompOpType || otherType == *floatCompOpType || otherType == *charCompOpType || otherType == *stringCompOpType) &&
			(kind == STD_DEQUALS || kind == STD_NEQUALS || kind == STD_LT || kind == STD_GT || kind == STD_LE || kind == STD_GE)) {
		return true;
	} else {
		return false;
	}
}
bool StdType::objectTypePromotion(const ObjectType &otherType) const {
	if (otherType == *stringerType &&
			(kind >= STD_MIN_COMPARABLE && kind <= STD_MAX_COMPARABLE)) {
		return true;
	} else {
		return false;
	}
}
Type *StdType::copy() {Type *retVal = new StdType(*this); retVal->operable = true; return retVal;}
void StdType::erase() {delete this;}
bool StdType::operator==(const Type &otherType) const {
	if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		return (kind == otherTypeCast->kind && baseEquals(otherType));
	} else {
		return false;
	}
}
bool StdType::operator==(int kind) const {return (this->kind == kind);}
Type *StdType::operator,(const Type &otherType) const {
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
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
TypeStatusBase StdType::operator>>(const Type &otherType) const {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && *(*this >> *(otherTypeCast->list[0]))) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		if (baseSendable(otherType) && kindCompare(*otherTypeCast)) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		// check for a StdType -> FilterType promotion case
		if (baseSendable(otherType) && filterTypePromotion(*otherTypeCast)) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		if (otherType.suffix == SUFFIX_LATCH || otherType.suffix == SUFFIX_STREAM) {
			// try to do a basic constructor send
			ObjectType *otherTypeCast = (ObjectType *)(&otherType);
			for (vector<TypeList *>::const_iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
				if (*(*this >> **iter)) {
					return nullType;
				}
			}
			// basic constructor sending failed, so check for a StdType -> ObjectType promotion case
			if (objectTypePromotion(*otherTypeCast)) {
				return nullType;
			} else {
				return errType;
			}
		} else {
			return errType;
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
string StdType::toString(unsigned int tabDepth) {
	TYPE_TO_STRING_HEADER;
	acc += kindToString(kind);
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}
StdType::operator string() {return toString(1);}

// FilterType functions
FilterType::FilterType(Type *from, Type *to, int suffix, int depth) : Type(CATEGORY_FILTERTYPE, suffix, depth) {
	if (from->category == CATEGORY_TYPELIST) {
		this->from = (TypeList *)from;
	} else {
		this->from = new TypeList(from);
	}
	if (to->category == CATEGORY_TYPELIST) {
		this->to = (TypeList *)to;
	} else {
		this->to = new TypeList(to);
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
Type *FilterType::copy() {Type *retVal = new FilterType(*this); retVal->operable = true; return retVal;}
void FilterType::erase() {from = NULL; to = NULL; delete this;}
bool FilterType::operator==(const Type &otherType) const {
	if (this == &otherType) { // if the filters are actually the same object instance, return true
		return true;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		return (baseEquals(otherType) && *from == *(otherTypeCast->from) && *to == *(otherTypeCast->to));
	} else {
		return false;
	}
}
bool FilterType::operator==(int kind) const {return false;}
Type *FilterType::operator,(const Type &otherType) const {
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
		if (otherTypeCast->suffix == SUFFIX_LATCH && *(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
TypeStatusBase FilterType::operator>>(const Type &otherType) const {
	if (this == &otherType) { // if the filters are actually the same object instance
		return nullType;
	} else if (otherType.category == CATEGORY_TYPELIST) {
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
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		if (otherType.suffix == SUFFIX_LATCH) {
			ObjectType *otherTypeCast = (ObjectType *)(&otherType);
			for (vector<TypeList *>::const_iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
				if (*(*this >> **iter)) {
					return nullType;
				}
			}
		} else {
			return errType;
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
string FilterType::toString(unsigned int tabDepth) {
	TYPE_TO_STRING_HEADER;
	acc = "[";
	acc += from->toString(tabDepth+1);
	acc += " --> ";
	acc += to->toString(tabDepth+1);
	acc += "]";
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}
FilterType::operator string() {
	TYPE_TO_STRING_HEADER;
	acc = "[";
	acc += (string)(*from);
	acc += " --> ";
	acc += (string)(*to);
	acc += "]";
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}

// ObjectType functions
ObjectType::ObjectType(int suffix, int depth) : Type(CATEGORY_OBJECTTYPE, suffix, depth), propagationHandled(false) {}
ObjectType::ObjectType(const vector<TypeList *> &constructorTypes, int suffix, int depth) : Type(CATEGORY_OBJECTTYPE, suffix, depth), constructorTypes(constructorTypes), propagationHandled(false) {}
ObjectType::ObjectType(const vector<TypeList *> &constructorTypes, const vector<string> &memberNames, const vector<Type *> &memberTypes, const vector<Tree *> &memberDefSites, int suffix, int depth) : 
	Type(CATEGORY_OBJECTTYPE, suffix, depth), constructorTypes(constructorTypes), memberNames(memberNames), memberTypes(memberTypes), memberDefSites(memberDefSites), propagationHandled(false) {}
ObjectType::~ObjectType() {
	for (vector<TypeList *>::iterator iter = constructorTypes.begin(); iter != constructorTypes.end(); iter++) {
		if (**iter != *nullType && **iter != *errType && !((*iter)->operable)) {
			delete (*iter);
		}
	}
	for (vector<Type *>::iterator iter = memberTypes.begin(); iter != memberTypes.end(); iter++) {
		if (**iter != *nullType && **iter != *errType && !((*iter)->operable)) {
			delete (*iter);
		}
	}
}
bool ObjectType::isComparable(const Type &otherType) const {return false;}
Type *ObjectType::copy() {ObjectType *retVal = new ObjectType(*this); copyList.push_back(retVal); retVal->operable = true; return retVal;}
void ObjectType::erase() {constructorTypes.clear(); memberNames.clear(); memberTypes.clear(); memberDefSites.clear(); delete this;}
void ObjectType::propagateToCopies() {
	if (propagationHandled) { // if we've already propagated to this node and got here through a recursive type loop, we're done
		return;
	}
	propagationHandled = true; // flag this filter as already iterated
	for (vector<ObjectType *>::const_iterator iter = copyList.begin(); iter != copyList.end(); iter++) {
		(*iter)->constructorTypes = constructorTypes;
		(*iter)->memberNames = memberNames;
		(*iter)->memberTypes = memberTypes;
		(*iter)->memberDefSites = memberDefSites;
		// recurse
		(*iter)->propagateToCopies();
	}
	propagationHandled = false; // unflag this filter as already iterated
}
bool ObjectType::operator==(const Type &otherType) const {
	if (this == &otherType) { // if the objects are actually the same object instance, return true
		return true;
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
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
bool ObjectType::operator==(int kind) const {return false;}
Type *ObjectType::operator,(const Type &otherType) const {
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
		if (otherTypeCast->suffix == SUFFIX_LATCH && *(*this >> *(otherTypeCast->from))) {
			return (otherTypeCast->to);
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
TypeStatusBase ObjectType::operator>>(const Type &otherType) const {
	if (this == &otherType) { // if the objects are actually the same object instance
		return nullType;
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*(*this >> *(otherTypeCast->list[0])))) {
			return nullType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		// try to find a downcastability from the left object to the right one
		if (baseSendable(otherType)) {
			// verify constructor downcastability
			vector<TypeList *>::const_iterator consIter;
			for (consIter = otherTypeCast->constructorTypes.begin(); consIter != otherTypeCast->constructorTypes.end(); consIter++) {
				vector<TypeList *>::const_iterator consIter2;
				for (consIter2 = constructorTypes.begin(); consIter2 != constructorTypes.end(); consIter2++) {
					if ((*consIter2)->baseEquals(**consIter) && (**consIter2 == **consIter)) {
						break;
					}
				}
				if (consIter2 == constructorTypes.end()) { // if we failed to find a match for this constructor, break early
					break;
				}
			}
			if (consIter == otherTypeCast->constructorTypes.end()) { // if we matched all constructor types (we didn't break early), continue
				// verify member downcastability
				vector<string>::const_iterator memberNameIter;
				vector<Type *>::const_iterator memberTypeIter;
				for (memberNameIter = otherTypeCast->memberNames.begin(), memberTypeIter = otherTypeCast->memberTypes.begin();
						memberNameIter != otherTypeCast->memberNames.end();
						memberNameIter++, memberTypeIter++) {
					vector<string>::const_iterator memberNameIter2;
					vector<Type *>::const_iterator memberTypeIter2;
					for (memberNameIter2 = memberNames.begin(), memberTypeIter2 = memberTypes.begin();
							memberNameIter2 != memberNames.end();
							memberNameIter2++, memberTypeIter2++) {
						if ((*memberNameIter2 == *memberNameIter) && (*memberTypeIter2)->baseEquals(**memberTypeIter) && (**memberTypeIter2 == **memberTypeIter)) {
							break;
						}
					}
					if (memberNameIter2 == memberNames.end()) { // if we failed to find a match for this member, break early
						break;
					}
				}
				if (memberNameIter == otherTypeCast->memberNames.end()) { // if we matched all mambers (we didn't break early), return success
					return nullType;
				}
			}
		}
		// otherwise, try to connect the left object into a constructor on the right object
		if (otherType.suffix == SUFFIX_LATCH || otherType.suffix == SUFFIX_STREAM) {
			for (vector<TypeList *>::const_iterator iter = otherTypeCast->constructorTypes.begin(); iter != otherTypeCast->constructorTypes.end(); iter++) {
				if (*(*this >> **iter)) {
					return nullType;
				}
			}
		} else {
			return errType;
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
string ObjectType::toString(unsigned int tabDepth) {
	TYPE_TO_STRING_HEADER;
	acc = "{";
	for (vector<TypeList *>::const_iterator iter = constructorTypes.begin(); iter != constructorTypes.end(); iter++) {
		TYPE_TO_STRING_INDENT;
		acc += "=[";
		acc += (*iter)->toString(tabDepth+1);
		acc += ']';
		if (iter+1 != constructorTypes.end()) {
			acc += ", ";
		}
	}
	if (constructorTypes.size() > 0 && memberNames.size() > 0) {
		acc += ", ";
	}
	vector<string>::const_iterator memberNameIter = memberNames.begin();
	vector<Type *>::const_iterator memberTypeIter = memberTypes.begin();
	while (memberNameIter != memberNames.end()) {
		TYPE_TO_STRING_INDENT;
		acc += *memberNameIter;
		acc += '=';
		acc += (*memberTypeIter)->toString(tabDepth+1);
		if (memberNameIter+1 != memberNames.end()) {
			acc += ", ";
		}
		// advance
		memberNameIter++;
		memberTypeIter++;
	}
	TYPE_TO_STRING_INDENT_CLOSE;
	acc += '}';
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}
ObjectType::operator string() {
	TYPE_TO_STRING_HEADER;
	acc = "{";
	for (vector<TypeList *>::const_iterator iter = constructorTypes.begin(); iter != constructorTypes.end(); iter++) {
		acc += "=[";
		acc += (string)(**iter);
		acc += ']';
		if (iter+1 != constructorTypes.end()) {
			acc += ", ";
		}
	}
	if (constructorTypes.size() > 0 && memberNames.size() > 0) {
		acc += ", ";
	}
	vector<string>::const_iterator memberNameIter = memberNames.begin();
	vector<Type *>::const_iterator memberTypeIter = memberTypes.begin();
	while (memberNameIter != memberNames.end()) {
		acc += *memberNameIter;
		acc += '=';
		acc += (string)(**memberTypeIter);
		if (memberNameIter+1 != memberNames.end()) {
			acc += ", ";
		}
		// advance
		memberNameIter++;
		memberTypeIter++;
	}
	acc += '}';
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}

// TypeStatusBase functions
TypeStatusBase::TypeStatusBase(Type *type) : type(type), code(NULL) {}
TypeStatusBase::operator Type *() const {return type;}
TypeStatusBase::operator unsigned int() const {return (unsigned int)type;}
const Type &TypeStatusBase::operator*() const {
	if (type != NULL) {
		return (*type);
	} else {
		return (*errType);
	}
}
Type *TypeStatusBase::operator->() const {return type;}
bool TypeStatusBase::operator==(const Type &otherType) const {return (*type == otherType);}
bool TypeStatusBase::operator!=(const Type &otherType) const {return (*type != otherType);}

// TypeStatus functions
TypeStatus::TypeStatus(Type *type, Type *retType) : TypeStatusBase(type), retType(retType) {}
TypeStatus::TypeStatus(Type *type, const TypeStatus &otherStatus) : TypeStatusBase(type), retType(otherStatus.retType) {}
TypeStatus::~TypeStatus() {}
TypeStatus &TypeStatus::operator=(const TypeStatus &otherStatus) {type = otherStatus.type; retType = otherStatus.retType; return *this;}
TypeStatus &TypeStatus::operator=(Type *otherType) {type = otherType; return *this;}

// auxiliary functions
string kindToString (int kind) {
	switch(kind) {
		// null type
		case STD_NULL:
			return "null";
		// standard types
		case STD_STD:
			return "std";
		case STD_INT:
			return "int";
		case STD_FLOAT:
			return "float";
			break;
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
