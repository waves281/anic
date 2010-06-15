#include "types.h"

#include "outputOperators.h"

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
void TypeList::erase() {clear(); delete this;}
void TypeList::clear() {list.clear();}
bool TypeList::operator==(const Type &otherType) const {
	if (this == &otherType) { // if the lists are actually the same object instance, return true
		return true;
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (list.size() != otherTypeCast->list.size()) {
			return false;
		}
		vector<Type *>::const_iterator iter1;
		vector<Type *>::const_iterator iter2;
		for (iter1 = list.begin(), iter2 = otherTypeCast->list.begin(); iter1 != list.end(); iter1++, iter2++) {
			if (**iter1 != **iter2) {
				return false;
			}
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
			if (otherTypeCast->kind == STD_NOT && (*thisTypeCast >> *stdBoolType)) {
				return (new StdType(STD_BOOL, SUFFIX_LATCH));
			} else if (otherTypeCast->kind == STD_COMPLEMENT && (*thisTypeCast >> *stdIntType)) {
				return (new StdType(STD_INT, SUFFIX_LATCH));
			} else if ((otherTypeCast->kind == STD_DPLUS || otherTypeCast->kind == STD_DMINUS) && (*thisTypeCast >> *stdIntType)) {
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
					((*thisTypeCast1 >> *stdBoolType) && (*thisTypeCast2 >> *stdBoolType))) {
				return (new StdType(STD_BOOL, SUFFIX_LATCH));
			} else if ((otherTypeCast->kind == STD_OR || otherTypeCast->kind == STD_XOR || otherTypeCast->kind == STD_AND) &&
					((*thisTypeCast1 >> *stdIntType) && (*thisTypeCast2 >> *stdIntType))) {
				return (new StdType(STD_INT, SUFFIX_LATCH));
			} else if (otherTypeCast->kind == STD_DEQUALS || otherTypeCast->kind == STD_NEQUALS ||
					otherTypeCast->kind == STD_LT || otherTypeCast->kind == STD_GT ||
					otherTypeCast->kind == STD_LE || otherTypeCast->kind == STD_GE) {
				if (thisTypeCast1->kindCast(*thisTypeCast2)) {
					return (new StdType(STD_BOOL, SUFFIX_LATCH));
				} else {
					return errType;
				}
			} else if (otherTypeCast->kind == STD_TIMES || otherTypeCast->kind == STD_DIVIDE || otherTypeCast->kind == STD_MOD ||
					otherTypeCast->kind == STD_PLUS || otherTypeCast->kind == STD_MINUS) {
				if ((*thisTypeCast1 >> *stdIntType) && (*thisTypeCast2 >> *stdIntType)) {
					return (new StdType(STD_INT, SUFFIX_LATCH));
				} else if ((*thisTypeCast1 >> *stdFloatType) && (*thisTypeCast2 >> *stdFloatType)) {
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
		if (otherTypeCast->suffix == SUFFIX_LATCH && (*this >> *(otherTypeCast->from))) {
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
bool TypeList::operator>>(const Type &otherType) const {
	if (this == &otherType) { // if the lists are actually the same object instance
		return true;
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (list.size() != otherTypeCast->list.size()) {
			return false;
		}
		vector<Type *>::const_iterator iter1 = list.begin();
		vector<Type *>::const_iterator iter2 = otherTypeCast->list.begin();
		while (iter1 != list.end() && iter2 != otherTypeCast->list.end()) {
			if (!(**iter1 >> **iter2)) {
				return false;
			}
			iter1++;
			iter2++;
		}
		return true;
	} else if (otherType.category == CATEGORY_STDTYPE) {
		if (list.size() == 1 && (*(list[0]) >> otherType)) {
			return true;
		} else {
			return false;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		if (list.size() == 1 && (*(list[0]) >> otherType)) {
			return true;
		} else {
			return false;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		if (list.size() == 1) {
			return (*(list[0]) >> otherType);
		} else if (otherType.suffix == SUFFIX_LATCH || otherType.suffix == SUFFIX_STREAM) {
			ObjectType *otherTypeCast = (ObjectType *)(&otherType);
			for (vector<TypeList *>::const_iterator iter = otherTypeCast->instructorTypes.begin(); iter != otherTypeCast->instructorTypes.end(); iter++) {
				if (*this >> **iter) {
					return true;
				}
			}
		} else {
			return false;
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return false;
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
void ErrorType::clear() {}
bool ErrorType::operator==(const Type &otherType) const {
	if (otherType.category == CATEGORY_ERRORTYPE) {
		return (this == &otherType);
	} else {
		return false;
	}
}
bool ErrorType::operator==(int kind) const {return false;}
Type *ErrorType::operator,(const Type &otherType) const {return errType;}
bool ErrorType::operator>>(const Type &otherType) const {return false;}
string ErrorType::toString(unsigned int tabDepth) {
	return "<ERROR>";
}
ErrorType::operator string() {return toString(1);}

// StdType functions
StdType::StdType(int kind, int suffix, int depth) : Type(CATEGORY_STDTYPE, suffix, depth), kind(kind) {}
StdType::~StdType() {}
bool StdType::isComparable() const {
	return (kind >= STD_MIN_COMPARABLE && kind <= STD_MAX_COMPARABLE);
}
bool StdType::isComparable(const Type &otherType) const {
	return (otherType.category == CATEGORY_STDTYPE && (kindCast(*((StdType *)(&otherType))) || ((StdType *)(&otherType))->kindCast(*this)));
}
int StdType::kindCast(const StdType &otherType) const {
	if (!(isComparable()) || !(otherType.isComparable())) {
		return STD_NULL;
	} else if (kind == otherType.kind) {
		return kind;
	} else if (kind == STD_INT && otherType.kind == STD_FLOAT) {
		return STD_FLOAT;
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
						((StdType *)(prevTermStatus.type))->kindCast(*((StdType *)(nextTermStatus.type)))) { // if the terms are comparable, return bool
					return make_pair(new StdType(STD_BOOL, SUFFIX_LATCH), true); // return true, since we're consuming the nextTerm
				}
			}
			break;
		case STD_LS:
		case STD_RS:
			if ((*prevTermStatus >> *stdIntType) && (*nextTermStatus >> *stdIntType)) { // if both terms can be converted to int, return int
				return make_pair(new StdType(STD_INT, SUFFIX_LATCH), true); // return true, since we're consuming the nextTerm
			}
			break;
		case STD_TIMES:
		case STD_DIVIDE:
		case STD_MOD:
		case STD_PLUS:
		case STD_MINUS:
			if (*nextTermStatus) {
				if ((*prevTermStatus >> *stdIntType) && (*nextTermStatus >> *stdIntType)) { // if both terms can be converted to int, return int
					return make_pair(new StdType(STD_INT, SUFFIX_LATCH), true); // return true, since we're consuming the nextTerm
				}
				if ((*prevTermStatus >> *stdFloatType) && (*nextTermStatus >> *stdFloatType)) { // if both terms can be converted to float, return float
					return make_pair(new StdType(STD_FLOAT, SUFFIX_LATCH), true); // return true, since we're consuming the nextTerm
				}
				// if this is the + operator and both terms are convertible to string, return string
				if (kind == STD_PLUS &&
						((*prevTermStatus >> *stdStringType) && (*nextTermStatus >> *stdStringType))) {
					return make_pair(new StdType(STD_STRING, SUFFIX_LATCH), true); // return true, since were consuming the nextTerm
				}
			}
			// if we got here, we failed to derive a three-term type, so now we try using STD_PLUS and STD_MINUS in their unary form
			if (kind == STD_PLUS || kind == STD_MINUS) { // if it's an operator with a unary form that accepts both ints an floats
				if (*prevTermStatus >> *stdIntType) { // if both terms can be converted to int, return int
					return make_pair(new StdType(STD_INT, SUFFIX_LATCH), false); // return false, since we're not consuming the nextTerm
				}
				if (*prevTermStatus >> *stdFloatType) { // if both terms can be converted to float, return float
					return make_pair(new StdType(STD_FLOAT, SUFFIX_LATCH), false); // return false, since we're not consuming the nextTerm
				}
			}
			break;
		case STD_DPLUS:
		case STD_DMINUS:
			if (*prevTermStatus >> *stdIntType) { // if both terms can be converted to int, return int
				return make_pair(new StdType(STD_INT, SUFFIX_LATCH), false); // return false, since we're not consuming the nextTerm
			}
			break;
		default:
			break;
	}
	return make_pair(errType, false); // return false, since we're not consuming the nextTerm (though this doesn't really matter -- it's an error anyway)
}
bool StdType::objectTypePromotion(const Type &otherType) const {
	if (kind >= STD_MIN_COMPARABLE && kind <= STD_MAX_COMPARABLE &&
			otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType tempObjectType(*stringerType); tempObjectType.suffix = suffix; tempObjectType.depth = depth;
		bool result = (tempObjectType >> otherType);
		tempObjectType.clear();
		if (result) {
			return true;
		}
	}
	if (kind == STD_NOT) {
		ObjectType tempObjectType(*boolUnOpType); tempObjectType.suffix = suffix; tempObjectType.depth = depth;
		bool result = (tempObjectType >> otherType);
		tempObjectType.clear();
		if (result) {
			return true;
		}
	}
	if (kind == STD_COMPLEMENT || kind == STD_DPLUS || kind == STD_DMINUS || kind == STD_PLUS || kind == STD_MINUS) {
		ObjectType tempObjectType(*intUnOpType); tempObjectType.suffix = suffix; tempObjectType.depth = depth;
		bool result = (tempObjectType >> otherType);
		tempObjectType.clear();
		if (result) {
			return true;
		}
	}
	if (kind == STD_DOR || kind == STD_DAND) {
		ObjectType tempObjectType(*boolBinOpType); tempObjectType.suffix = suffix; tempObjectType.depth = depth;
		bool result = (tempObjectType >> otherType);
		tempObjectType.clear();
		if (result) {
			return true;
		}
	}
	if (kind == STD_OR || kind == STD_AND || kind == STD_XOR || kind == STD_PLUS || kind == STD_MINUS || kind == STD_TIMES || kind == STD_DIVIDE || kind == STD_MOD || kind == STD_LS || kind == STD_RS) {
		ObjectType tempObjectType(*intBinOpType); tempObjectType.suffix = suffix; tempObjectType.depth = depth;
		bool result = (tempObjectType >> otherType);
		tempObjectType.clear();
		if (result) {
			return true;
		}
	}
	if (kind == STD_PLUS || kind == STD_MINUS || kind == STD_TIMES || kind == STD_DIVIDE || kind == STD_MOD) {
		ObjectType tempObjectType(*floatBinOpType); tempObjectType.suffix = suffix; tempObjectType.depth = depth;
		bool result = (tempObjectType >> otherType);
		tempObjectType.clear();
		if (result) {
			return true;
		}
	}
	if (kind == STD_DEQUALS || kind == STD_NEQUALS || kind == STD_LT || kind == STD_GT || kind == STD_LE || kind == STD_GE) {
		ObjectType tempObjectType1(*boolCompOpType); tempObjectType1.suffix = suffix; tempObjectType1.depth = depth;
		bool result = (tempObjectType1 >> otherType);
		tempObjectType1.clear();
		if (result) {
			return true;
		}
		ObjectType tempObjectType2(*intCompOpType); tempObjectType2.suffix = suffix; tempObjectType2.depth = depth;
		result = (tempObjectType2 >> otherType);
		tempObjectType2.clear();
		if (result) {
			return true;
		}
		ObjectType tempObjectType3(*floatCompOpType); tempObjectType3.suffix = suffix; tempObjectType3.depth = depth;
		result = (tempObjectType3 >> otherType);
		tempObjectType3.clear();
		if (result) {
			return true;
		}
		ObjectType tempObjectType4(*charCompOpType); tempObjectType4.suffix = suffix; tempObjectType4.depth = depth;
		result = (tempObjectType4 >> otherType);
		tempObjectType4.clear();
		if (result) {
			return true;
		}
		ObjectType tempObjectType5(*stringCompOpType); tempObjectType5.suffix = suffix; tempObjectType5.depth = depth;
		result = (tempObjectType5 >> otherType);
		tempObjectType5.clear();
		return result;
	}
	// none of the above cases succeeded, so return false
	return false;
}
Type *StdType::copy() {Type *retVal = new StdType(*this); retVal->operable = true; return retVal;}
void StdType::erase() {delete this;}
void StdType::clear() {}
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
		if (*this >> *(otherTypeCast->from)) {
			return (otherTypeCast->to);
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
bool StdType::operator>>(const Type &otherType) const {
	if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*this >> *(otherTypeCast->list[0]))) {
			return true;
		} else {
			return false;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		if (baseSendable(otherType) && kindCast(*otherTypeCast)) {
			return true;
		} else {
			return false;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		return (objectTypePromotion(otherType));
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		if (otherType.suffix == SUFFIX_LATCH || otherType.suffix == SUFFIX_STREAM) {
			// try to do a basic instructor send
			for (vector<TypeList *>::const_iterator iter = otherTypeCast->instructorTypes.begin(); iter != otherTypeCast->instructorTypes.end(); iter++) {
				if (*this >> **iter) {
					return true;
				}
			}
		}
		// basic instructor sending failed, so check for a StdType -> ObjectType promotion case
		return (objectTypePromotion(*otherTypeCast));
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return false;
}
string StdType::kindToString() const {
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
		// can't happen; the above should cover all cases
		default:
			return "";
	}
}
string StdType::toString(unsigned int tabDepth) {
	TYPE_TO_STRING_HEADER;
	acc += kindToString();
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
void FilterType::erase() {clear(); delete this;}
void FilterType::clear() {from = NULL; to = NULL;}
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
		if (otherTypeCast->suffix == SUFFIX_LATCH && (*this >> *(otherTypeCast->from))) {
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
bool FilterType::operator>>(const Type &otherType) const {
	if (this == &otherType) { // if the filters are actually the same object instance
		return true;
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*this >> *(otherTypeCast->list[0]))) {
			return true;
		} else {
			return false;
		}
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return false;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		if (baseSendable(otherType) && operator==(otherType)) {
			return true;
		} else {
			return false;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		if (otherType.suffix == SUFFIX_LATCH) {
			ObjectType *otherTypeCast = (ObjectType *)(&otherType);
			for (vector<TypeList *>::const_iterator iter = otherTypeCast->instructorTypes.begin(); iter != otherTypeCast->instructorTypes.end(); iter++) {
				if (*this >> **iter) {
					return true;
				}
			}
		} else {
			return false;
		}
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return false;
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
ObjectType::ObjectType(const vector<TypeList *> &instructorTypes, const vector<TypeList *> &outstructorTypes, int suffix, int depth) :
	Type(CATEGORY_OBJECTTYPE, suffix, depth), instructorTypes(instructorTypes), outstructorTypes(outstructorTypes), propagationHandled(false) {}
ObjectType::ObjectType(const vector<TypeList *> &instructorTypes, const vector<TypeList *> &outstructorTypes, const vector<string> &memberNames, const vector<Type *> &memberTypes, const vector<Tree *> &memberDefSites, int suffix, int depth) : 
	Type(CATEGORY_OBJECTTYPE, suffix, depth), instructorTypes(instructorTypes), outstructorTypes(outstructorTypes), memberNames(memberNames), memberTypes(memberTypes), memberDefSites(memberDefSites), propagationHandled(false) {}
ObjectType::~ObjectType() {
	for (vector<TypeList *>::iterator iter = instructorTypes.begin(); iter != instructorTypes.end(); iter++) {
		if (**iter != *nullType && **iter != *errType && !((*iter)->operable)) {
			delete (*iter);
		}
	}
	for (vector<TypeList *>::iterator iter = outstructorTypes.begin(); iter != outstructorTypes.end(); iter++) {
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
void ObjectType::erase() {clear(); delete this;}
void ObjectType::clear() {instructorTypes.clear(); outstructorTypes.clear(); memberNames.clear(); memberTypes.clear(); memberDefSites.clear();}
void ObjectType::propagateToCopies() {
	if (propagationHandled) { // if we've already propagated to this node and got here through a recursive type loop, we're done
		return;
	}
	propagationHandled = true; // flag this filter as already iterated
	for (vector<ObjectType *>::const_iterator iter = copyList.begin(); iter != copyList.end(); iter++) {
		(*iter)->instructorTypes = instructorTypes;
		(*iter)->outstructorTypes = outstructorTypes;
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
		if ((instructorTypes.size() == otherTypeCast->instructorTypes.size()) && (outstructorTypes.size() == otherTypeCast->outstructorTypes.size()) && (memberNames.size() == otherTypeCast->memberNames.size())) {
			// verify that the instructors match
			vector<TypeList *>::const_iterator insIter1 = instructorTypes.begin();
			vector<TypeList *>::const_iterator insIter2 = otherTypeCast->instructorTypes.begin();
			while (insIter1 != instructorTypes.end() && insIter2 != otherTypeCast->instructorTypes.end()) {
				if (**insIter1 != **insIter2) {
					return false;
				}
				// advance
				insIter1++;
				insIter2++;
			}
			// verify that the outstructors match
			vector<TypeList *>::const_iterator outsIter1 = outstructorTypes.begin();
			vector<TypeList *>::const_iterator outsIter2 = otherTypeCast->outstructorTypes.begin();
			while (outsIter1 != outstructorTypes.end() && outsIter2 != otherTypeCast->outstructorTypes.end()) {
				if (**outsIter1 != **outsIter2) {
					return false;
				}
				// advance
				outsIter1++;
				outsIter2++;
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
		if (otherTypeCast->suffix == SUFFIX_LATCH && (*this >> *(otherTypeCast->from))) {
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
bool ObjectType::operator>>(const Type &otherType) const {
	if (this == &otherType) { // if the objects are actually the same object instance, allow the downcastability
		return true;
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		// try a direct downcastability
		if (otherTypeCast->list.size() == 1 && (*this >> *(otherTypeCast->list[0]))) {
			return true;
		}
		// otherwise, try an outstructed downcastability
		for (vector<TypeList *>::const_iterator outsIter = outstructorTypes.begin(); outsIter != outstructorTypes.end(); outsIter++) {
			if (**outsIter >> *otherTypeCast) { // if there is a type match, return true
				return true;
			}
		}
		// else if we didn't find a type match, return false
		return false;
	} else if (otherType.category == CATEGORY_STDTYPE) {
		// otherwise, try an outstructed downcastability
		for (vector<TypeList *>::const_iterator outsIter = outstructorTypes.begin(); outsIter != outstructorTypes.end(); outsIter++) {
			if (**outsIter >> otherType) { // if there is a type match, return true
				return true;
			}
		}
		// else if we didn't find a type match, return false
		return false;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		// otherwise, try an outstructed downcastability
		for (vector<TypeList *>::const_iterator outsIter = outstructorTypes.begin(); outsIter != outstructorTypes.end(); outsIter++) {
			if (**outsIter >> otherType) { // if there is a type match, return true
				return true;
			}
		}
		// else if we didn't find a type match, return false
		return false;
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		if (baseSendable(otherType)) {
			// try to find a direct downcastability from the left object to the right one
			// verify instructor downcastability
			vector<TypeList *>::const_iterator insIter;
			for (insIter = otherTypeCast->instructorTypes.begin(); insIter != otherTypeCast->instructorTypes.end(); insIter++) {
				vector<TypeList *>::const_iterator insIter2;
				for (insIter2 = instructorTypes.begin(); insIter2 != instructorTypes.end(); insIter2++) {
					if ((*insIter2)->baseEquals(**insIter) && (**insIter2 == **insIter)) {
						break;
					}
				}
				if (insIter2 == instructorTypes.end()) { // if we failed to find a match for this constructor, break early
					break;
				}
			}
			if (insIter == otherTypeCast->instructorTypes.end()) { // if we matched all constructor types (we didn't break early), continue
				// verify outstructor downcastability
				vector<TypeList *>::const_iterator outsIter;
				for (outsIter = otherTypeCast->outstructorTypes.begin(); outsIter != otherTypeCast->outstructorTypes.end(); outsIter++) {
					vector<TypeList *>::const_iterator outsIter2;
					for (outsIter2 = outstructorTypes.begin(); outsIter2 != outstructorTypes.end(); outsIter2++) {
						if ((*outsIter2)->baseEquals(**outsIter) && (**outsIter2 == **outsIter)) {
							break;
						}
					}
					if (outsIter2 == outstructorTypes.end()) { // if we failed to find a match for this constructor, break early
						break;
					}
				}
				if (outsIter == otherTypeCast->outstructorTypes.end()) { // if we matched all constructor types (we didn't break early), continue
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
						return true;
					}
				}
			}
			// try to connect the left object into a constructor on the right object
			for (vector<TypeList *>::const_iterator insIter = otherTypeCast->instructorTypes.begin(); insIter != otherTypeCast->instructorTypes.end(); insIter++) {
				if (*this >> **insIter) {
					return true;
				}
			}
			// otherwise, try an outstructed downcastability from the left object to the right object
			for (vector<TypeList *>::const_iterator outsIter = outstructorTypes.begin(); outsIter != outstructorTypes.end(); outsIter++) {
			if (**outsIter >> *otherTypeCast) { // if there is a type match, return true
				return true;
			}
		}
		}
		// if we had no matches to any of the above cases, conclude that the objects are not downcastable and return false
		return false;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return false;
}
string ObjectType::toString(unsigned int tabDepth) {
	TYPE_TO_STRING_HEADER;
	acc = "{";
	for (vector<TypeList *>::const_iterator iter = instructorTypes.begin(); iter != instructorTypes.end(); iter++) {
		TYPE_TO_STRING_INDENT;
		acc += "=[";
		acc += (*iter)->toString(tabDepth+1);
		acc += ']';
		if (iter+1 != instructorTypes.end()) {
			acc += ", ";
		}
	}
	if (instructorTypes.size() > 0 && outstructorTypes.size() > 0) {
		acc += ", ";
	}
	for (vector<TypeList *>::const_iterator iter = outstructorTypes.begin(); iter != outstructorTypes.end(); iter++) {
		TYPE_TO_STRING_INDENT;
		acc += "=[--> ";
		acc += (*iter)->toString(tabDepth+1);
		acc += ']';
		if (iter+1 != outstructorTypes.end()) {
			acc += ", ";
		}
	}
	if (outstructorTypes.size() > 0 && memberNames.size() > 0) {
		acc += ", ";
	}
	vector<string>::const_iterator memberNameIter;
	vector<Type *>::const_iterator memberTypeIter;
	for (memberNameIter = memberNames.begin(), memberTypeIter = memberTypes.begin(); memberNameIter != memberNames.end(); memberNameIter++, memberTypeIter++) {
		TYPE_TO_STRING_INDENT;
		acc += *memberNameIter;
		acc += '=';
		acc += (*memberTypeIter)->toString(tabDepth+1);
		if (memberNameIter+1 != memberNames.end()) {
			acc += ", ";
		}
	}
	TYPE_TO_STRING_INDENT_CLOSE;
	acc += '}';
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}
ObjectType::operator string() {
	TYPE_TO_STRING_HEADER;
	acc = "{";
	for (vector<TypeList *>::const_iterator iter = instructorTypes.begin(); iter != instructorTypes.end(); iter++) {
		acc += "=[";
		acc += (string)(**iter);
		acc += ']';
		if (iter+1 != instructorTypes.end()) {
			acc += ", ";
		}
	}
	if (instructorTypes.size() > 0 && outstructorTypes.size() > 0) {
		acc += ", ";
	}
	for (vector<TypeList *>::const_iterator iter = outstructorTypes.begin(); iter != outstructorTypes.end(); iter++) {
		acc += "=[-->";
		acc += (string)(**iter);
		acc += ']';
		if (iter+1 != outstructorTypes.end()) {
			acc += ", ";
		}
	}
	if (outstructorTypes.size() > 0 && memberNames.size() > 0) {
		acc += ", ";
	}
	vector<string>::const_iterator memberNameIter;
	vector<Type *>::const_iterator memberTypeIter;
	for (memberNameIter = memberNames.begin(), memberTypeIter = memberTypes.begin(); memberNameIter != memberNames.end(); memberNameIter++, memberTypeIter++) {
		acc += *memberNameIter;
		acc += '=';
		acc += (string)(**memberTypeIter);
		if (memberNameIter+1 != memberNames.end()) {
			acc += ", ";
		}
	}
	acc += '}';
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}

// TypeStatus functions
TypeStatus::TypeStatus(Type *type, Type *retType) : type(type), retType(retType), code(NULL) {}
TypeStatus::TypeStatus(Type *type, const TypeStatus &otherStatus) : type(type), retType(otherStatus.retType), code(NULL) {}
TypeStatus::~TypeStatus() {}
TypeStatus::operator Type *() const {return type;}
TypeStatus::operator unsigned int() const {return (unsigned int)type;}
DataTree *TypeStatus::castCode(const Type &destType) const {
	StdType *thisType = (StdType *)type;
	StdType *otherType = (StdType *)(&destType);
	if (thisType->kind == otherType->kind) {
		return (DataTree *)code;
	} else if (thisType->kind == STD_INT && otherType->kind == STD_FLOAT) {
		return (new TempTree(new ConvOpTree(CONVOP_INT2FLOAT, (DataTree *)code)));
	} else if (thisType->kind == STD_FLOAT && otherType->kind == STD_INT) {
		return (new TempTree(new ConvOpTree(CONVOP_FLOAT2INT, (DataTree *)code)));
	} else if (otherType->kind == STD_STRING) {
		switch(thisType->kind) {
			case STD_BOOL:
				return (new TempTree(new ConvOpTree(CONVOP_BOOL2STRING, (DataTree *)code)));
			case STD_INT:
				return (new TempTree(new ConvOpTree(CONVOP_INT2STRING, (DataTree *)code)));
			case STD_FLOAT:
				return (new TempTree(new ConvOpTree(CONVOP_FLOAT2STRING, (DataTree *)code)));
			case STD_CHAR:
				return (new TempTree(new ConvOpTree(CONVOP_CHAR2STRING, (DataTree *)code)));
			default: // can't happen; the above should cover all cases
				return NULL;
				break;
		}
	}
	return (DataTree *)code;
}
DataTree *TypeStatus::castCommonCode(const Type &otherType) const {
	if (*type >> otherType) {
		return castCode(otherType);
	} else /* if (otherType >> *type) */ {
		return (DataTree *)code;
	}
}
TypeStatus &TypeStatus::operator=(const TypeStatus &otherStatus) {type = otherStatus.type; retType = otherStatus.retType; return *this;}
TypeStatus &TypeStatus::operator=(Type *otherType) {type = otherType; return *this;}
const Type &TypeStatus::operator*() const {
	if (type != NULL) {
		return (*type);
	} else {
		return (*errType);
	}
}
Type *TypeStatus::operator->() const {return type;}
bool TypeStatus::operator==(const Type &otherType) const {return (*type == otherType);}
bool TypeStatus::operator!=(const Type &otherType) const {return (*type != otherType);}
