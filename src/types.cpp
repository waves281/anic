#include "types.h"

#include "outputOperators.h"

// Type functions
Type::Type(int category, int suffix, int depth, Tree *offsetExp) : category(category), suffix(suffix), depth(depth), offsetExp(offsetExp),
	referensible(true), instantiable(true), toStringHandled(false) {}
bool Type::baseEquals(const Type &otherType) const {return (suffix == otherType.suffix && depth == otherType.depth);}
bool Type::baseSendable(const Type &otherType) const {
	return (
		(suffix == SUFFIX_CONSTANT && (otherType.suffix == SUFFIX_CONSTANT || (otherType.suffix == SUFFIX_LIST && otherType.depth == 1))) ||
		(suffix == SUFFIX_LATCH && (otherType.suffix == SUFFIX_CONSTANT || otherType.suffix == SUFFIX_LATCH ||
			((otherType.suffix == SUFFIX_LIST || otherType.suffix == SUFFIX_STREAM) && otherType.depth == 1))) ||
		(suffix == SUFFIX_LIST && (otherType.suffix == SUFFIX_LIST) && depth == otherType.depth) ||
		(suffix == SUFFIX_STREAM && (otherType.suffix == SUFFIX_LIST || otherType.suffix == SUFFIX_STREAM) && depth == otherType.depth) ||
		(suffix == SUFFIX_ARRAY && (otherType.suffix == SUFFIX_ARRAY) && depth == otherType.depth) ||
		(suffix == SUFFIX_POOL && (otherType.suffix == SUFFIX_POOL || otherType.suffix == SUFFIX_ARRAY) && depth == otherType.depth)
	);
}
int Type::offsetKind() const {
	if (suffix == SUFFIX_CONSTANT || suffix == SUFFIX_ARRAY) {
		return OFFSET_RAW;
	} else if (suffix == SUFFIX_LATCH) {
		switch(category) {
			case CATEGORY_STDTYPE:
				if ((((StdType *)(this))->kind != STD_STRING)) {
					return OFFSET_RAW;
				} else {
					return OFFSET_PARTITION;
				}
			case CATEGORY_FILTERTYPE:
			case CATEGORY_OBJECTTYPE:
				return OFFSET_PARTITION;
			default:
				// can't happen; we don't call this on TYPELIST or ERRORTYPE categories
				return OFFSET_NULL;
		}
	} else if (suffix == SUFFIX_LIST || suffix == SUFFIX_STREAM) {
		return OFFSET_PARTITION;
	} else /* if (suffix == SUFFIX_POOL) */ {
		switch(category) {
			case CATEGORY_STDTYPE:
				if ((((StdType *)(this))->kind != STD_STRING)) {
					return OFFSET_BLOCK;
				} else {
					return OFFSET_SHARE;
				}
			case CATEGORY_FILTERTYPE:
			case CATEGORY_OBJECTTYPE:
				return OFFSET_SHARE;
			default:
				// can't happen; we don't call this on TYPELIST or ERRORTYPE categories
				return OFFSET_NULL;
		}
	}
}
string Type::suffixString() const {
	string acc;
	if (suffix == SUFFIX_LATCH) {
		acc += '\\';
	} else if (suffix == SUFFIX_LIST) {
		for (int i = 0; i < depth; i++) {
			acc += "[]";
		}
	} else if (suffix == SUFFIX_STREAM) {
		for (int i = 0; i < depth; i++) {
			acc += "\\\\";
		}
	} else if (suffix == SUFFIX_ARRAY) {
		for (int i = 0; i < depth; i++) {
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
void Type::constantize() {
	if (suffix == SUFFIX_LATCH) {
		suffix = SUFFIX_CONSTANT;
	} else if (suffix == SUFFIX_STREAM) {
		suffix = SUFFIX_LIST;
	} else if (suffix == SUFFIX_POOL) {
		suffix = SUFFIX_ARRAY;
	}
}
void Type::latchize() {
	suffix = SUFFIX_LATCH;
	depth = 0;
}
void Type::poolize(Tree *offsetExp) {
	suffix = SUFFIX_POOL;
	this->offsetExp = offsetExp;
	if (depth == 0) {
		depth = 1;
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
bool Type::delatch() const {
	if (suffix == SUFFIX_CONSTANT) {
		return false;
	} else if (suffix == SUFFIX_LATCH) {
		return true;
	} else if (suffix == SUFFIX_LIST) {
		return false;
	} else if (suffix == SUFFIX_STREAM) {
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
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_CONSTANT;
		}
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_CONSTANT;
		} else {
			suffix = SUFFIX_LIST;
		}
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
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_LATCH;
		}
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
void Type::copyDelatch(Tree *offsetExp) {
	if (suffix == SUFFIX_CONSTANT) {
		suffix = SUFFIX_LATCH;
	} else if (suffix == SUFFIX_LIST) {
		suffix = SUFFIX_STREAM;
	} else if (suffix == SUFFIX_ARRAY) {
		suffix = SUFFIX_POOL;
		this->offsetExp = offsetExp;
	}
}
bool Type::pack() {
	if (suffix == SUFFIX_CONSTANT) {
		suffix = SUFFIX_LIST;
		depth = 1;
		return true;
	} else if (suffix == SUFFIX_LATCH) {
		suffix = SUFFIX_STREAM;
		depth = 1;
		return true;
	} else if (suffix == SUFFIX_LIST) {
		depth++;
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		depth++;
		return true;
	} else if (suffix == SUFFIX_ARRAY) {
		return false;
	} else /* if (suffix == SUFFIX_POOL) */ {
		return false;
	}
}
bool Type::unpack() {
	if (suffix == SUFFIX_CONSTANT) {
		return false;
	} else if (suffix == SUFFIX_LATCH) {
		return false;
	} else if (suffix == SUFFIX_LIST) {
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_CONSTANT;
		}
		return true;
	} else if (suffix == SUFFIX_STREAM) {
		depth--;
		if (depth == 0) {
			suffix = SUFFIX_LATCH;
		}
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
			suffix = SUFFIX_LATCH;
		}
		return true;
	}
}
Type *Type::link(Type &otherType) {
	if (suffix == SUFFIX_LIST && otherType.suffix == SUFFIX_LIST && depth == otherType.depth) {
		if (*this >> otherType) {
			return (Type *)(&otherType);
		} else if (otherType >> *this) {
			return (Type *)this;
		} else {
			return errType;
		}
	} else if (suffix == SUFFIX_STREAM && otherType.suffix == SUFFIX_STREAM && depth == otherType.depth) {
		if (*this >> otherType) {
			return (Type *)(&otherType);
		} else if (otherType >> *this) {
			return (Type *)this;
		} else {
			return errType;
		}
	} else {
		return errType;
	}
}
Type::operator bool() const {return (category != CATEGORY_ERRORTYPE);}
bool Type::operator!() const {return (category == CATEGORY_ERRORTYPE);}
bool Type::operator!=(Type &otherType) {return (!(operator==(otherType)));};
bool Type::operator!=(int kind) const {return (!(operator==(kind)));}

// StdType functions
StdType::StdType(int kind, int suffix, int depth, Tree *offsetExp) : Type(CATEGORY_STDTYPE, suffix, depth, offsetExp), kind(kind) {}
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
bool StdType::objectTypePromotion(Type &otherType) const {
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
Type *StdType::copy() {Type *retVal = new StdType(*this); retVal->referensible = true; return retVal;}
void StdType::erase() {delete this;}
void StdType::clear() {}
bool StdType::operator==(Type &otherType) {
	if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		return (kind == otherTypeCast->kind && baseEquals(otherType));
	} else {
		return false;
	}
}
bool StdType::operator==(int kind) const {return (this->kind == kind);}
Type *StdType::operator,(Type &otherType) {
	if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1) {
			return (*this , *(otherTypeCast->list[0]));
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (*this >> *(otherTypeCast->from())) {
			return otherTypeCast->to();
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
bool StdType::operator>>(Type &otherType) {
	if (otherType.category == CATEGORY_STDTYPE) {
		StdType *otherTypeCast = (StdType *)(&otherType);
		if (baseSendable(otherType) && kindCast(*otherTypeCast)) {
			return true;
		} else {
			return false;
		}
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*this >> *(otherTypeCast->list[0]))) {
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
			for (StructorList::iterator iter = otherTypeCast->instructorList.begin(); iter != otherTypeCast->instructorList.end(); iter++) {
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
	COLOR( acc += SET_TERM(BRIGHT_CODE AND CYAN_CODE); )
	acc += kindToString();
	COLOR( acc += SET_TERM(RESET_CODE); )
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}
StdType::operator string() {return toString(1);}

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
		if (**iter != *nullType && **iter != *errType && !((*iter)->referensible)) {
			delete (*iter);
		}
	}
}
bool TypeList::isComparable(const Type &otherType) const {return (list.size() == 1 && list[0]->isComparable(otherType));}
Type *TypeList::copy() {Type *retVal = new TypeList(*this); retVal->referensible = true; return retVal;}
void TypeList::erase() {clear(); delete this;}
void TypeList::clear() {list.clear();}
bool TypeList::operator==(Type &otherType) {
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
Type *TypeList::operator,(Type &otherType) {
	if (otherType.category == CATEGORY_STDTYPE) {
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
	} else if (otherType.category == CATEGORY_TYPELIST) {
		return errType;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (otherTypeCast->suffix == SUFFIX_LATCH && (*this >> *(otherTypeCast->from()))) {
			return otherTypeCast->to();
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
bool TypeList::operator>>(Type &otherType) {
	if (this == &otherType) { // if the lists are actually the same object instance
		return true;
	} else if (otherType.category == CATEGORY_STDTYPE) {
		if (list.size() == 1 && (*(list[0]) >> otherType)) {
			return true;
		} else {
			return false;
		}
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
			for (StructorList::iterator iter = otherTypeCast->instructorList.begin(); iter != otherTypeCast->instructorList.end(); iter++) {
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

// FilterType functions
FilterType::FilterType(Type *from, Type *to, int suffix, int depth, Tree *offsetExp) : Type(CATEGORY_FILTERTYPE, suffix, depth, offsetExp), defSite(NULL) {
	if (from->category == CATEGORY_TYPELIST) {
		fromInternal = (TypeList *)from;
	} else {
		fromInternal = new TypeList(from);
	}
	if (to->category == CATEGORY_TYPELIST) {
		toInternal = (TypeList *)to;
	} else {
		toInternal = new TypeList(to);
	}
}
FilterType::FilterType(Tree *defSite, int suffix, int depth, Tree *offsetExp) : Type(CATEGORY_FILTERTYPE, suffix, depth, offsetExp), fromInternal(NULL), toInternal(NULL), defSite(defSite) {}
FilterType::~FilterType() {
	if (fromInternal != NULL && *fromInternal != *nullType && *fromInternal != *errType) {
		delete fromInternal;
	}
	if (toInternal != NULL && *toInternal != *nullType && *toInternal != *errType) {
		delete toInternal;
	}
}
TypeList *FilterType::from() { // either TypeList or errType
	if (fromInternal == NULL) {
		TypeStatus derivedStatus = getStatusFilterHeader(defSite);
		if (*derivedStatus) { // if we managed to derive a type for the filter header
			fromInternal = ((FilterType *)(derivedStatus.type))->fromInternal;
			toInternal = ((FilterType *)(derivedStatus.type))->toInternal;
		} else { // else if we failed to derive a type for the filter header, log both the from- and to- types as erroneous
			fromInternal = (TypeList *)errType;
			toInternal = (TypeList *)errType;
		}
	}
	return fromInternal;
}
TypeList *FilterType::to() { // either TypeList or errType
	if (toInternal == NULL) {
		TypeStatus derivedStatus = getStatusFilterHeader(defSite);
		if (*derivedStatus) { // if we managed to derive a type for the filter header
			fromInternal = ((FilterType *)(derivedStatus.type))->fromInternal;
			toInternal = ((FilterType *)(derivedStatus.type))->toInternal;
		} else { // else if we failed to derive a type for the filter header, log both the from- and to- types as erroneous
			fromInternal = (TypeList *)errType;
			toInternal = (TypeList *)errType;
		}
	}
	return toInternal;
}
bool FilterType::isComparable(const Type &otherType) const {return false;}
Type *FilterType::copy() {Type *retVal = new FilterType(*this); retVal->referensible = true; return retVal;}
void FilterType::erase() {clear(); delete this;}
void FilterType::clear() {fromInternal = NULL; toInternal = NULL; defSite = NULL;}
bool FilterType::operator==(Type &otherType) {
	if (this == &otherType) { // if the filters are actually the same object instance, return true
		return true;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		return (baseEquals(otherType) && *(from()) == *(otherTypeCast->from()) && *(to()) == *(otherTypeCast->to()));
	} else {
		return false;
	}
}
bool FilterType::operator==(int kind) const {return false;}
Type *FilterType::operator,(Type &otherType) {
	if (otherType.category == CATEGORY_STDTYPE) {
		return errType;
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1) {
			return (*this , *(otherTypeCast->list[0]));
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (otherTypeCast->suffix == SUFFIX_LATCH && (*this >> *(otherTypeCast->from()))) {
			return otherTypeCast->to();
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}
bool FilterType::operator>>(Type &otherType) {
	if (this == &otherType) { // if the filters are actually the same object instance
		return true;
	} else if (otherType.category == CATEGORY_STDTYPE) {
		return false;
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1 && (*this >> *(otherTypeCast->list[0]))) {
			return true;
		} else {
			return false;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		if (baseSendable(otherType) && operator==(otherType)) {
			return true;
		} else {
			return false;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		if (otherType.suffix == SUFFIX_LATCH) {
			ObjectType *otherTypeCast = (ObjectType *)(&otherType);
			for (StructorList::iterator iter = otherTypeCast->instructorList.begin(); iter != otherTypeCast->instructorList.end(); iter++) {
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
	COLOR( acc += SET_TERM(BRIGHT_CODE AND GREEN_CODE); )
	acc += '[';
	COLOR( acc += SET_TERM(RESET_CODE); )
	acc += from()->toString(tabDepth+1);
	COLOR( acc += SET_TERM(BRIGHT_CODE AND GREEN_CODE); )
	acc += " --> ";
	COLOR( acc += SET_TERM(RESET_CODE); )
	acc += to()->toString(tabDepth+1);
	COLOR( acc += SET_TERM(BRIGHT_CODE AND GREEN_CODE); )
	acc += ']';
	COLOR( acc += SET_TERM(RESET_CODE); )
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}
FilterType::operator string() {
	TYPE_TO_STRING_HEADER;
	COLOR( acc += SET_TERM(BRIGHT_CODE AND GREEN_CODE); )
	acc += '[';
	COLOR( acc += SET_TERM(RESET_CODE); )
	acc += (string)(*(from()));
	COLOR( acc += SET_TERM(BRIGHT_CODE AND GREEN_CODE); )
	acc += " --> ";
	COLOR( acc += SET_TERM(RESET_CODE); )
	acc += (string)(*(to()));
	COLOR( acc += SET_TERM(BRIGHT_CODE AND GREEN_CODE); )
	acc += ']';
	COLOR( acc += SET_TERM(RESET_CODE); )
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}

// StructorListResult functions
StructorListResult::StructorListResult(const pair<Type *, Tree *> &internalPair) : internalPair(internalPair) {}
StructorListResult::~StructorListResult() {}
StructorListResult::operator Type *() const {return internalPair.first;}
Tree *StructorListResult::defSite() const {return internalPair.second;}
Type *StructorListResult::operator->() const {return internalPair.first;}
bool StructorListResult::operator==(const StructorListResult &otherResult) const {return *(internalPair.first) == *(otherResult.internalPair.first);}
bool StructorListResult::operator!=(const StructorListResult &otherResult) const {return *(internalPair.first) == *(otherResult.internalPair.first);}

// StructorList functions
StructorList::StructorList() {}
StructorList::StructorList(const StructorList &otherStructorList) : structors(otherStructorList.structors) {}
StructorList::~StructorList() {}
void StructorList::add(TypeList *typeList) {structors.push_back(make_pair(typeList, (Tree *)NULL));}
void StructorList::add(Tree *tree) {structors.push_back(make_pair((Type *)NULL, tree));}
unsigned int StructorList::size() const {return structors.size();}
void StructorList::clear() {structors.clear();}
StructorList::iterator::iterator() : internalIter() {}
StructorList::iterator::iterator(const StructorList::iterator &otherIter) : internalIter(otherIter.internalIter) {}
StructorList::iterator::iterator(const vector<pair<Type *, Tree *> >::iterator &internalIter) : internalIter(internalIter) {}
StructorList::iterator::~iterator() {}
StructorList::iterator &StructorList::iterator::operator=(const StructorList::iterator &otherIter) {internalIter = otherIter.internalIter; return *this;}
void StructorList::iterator::operator++(int) {internalIter++;}
bool StructorList::iterator::operator==(const iterator &otherIter) {return (internalIter == otherIter.internalIter);}
bool StructorList::iterator::operator!=(const iterator &otherIter) {return (internalIter != otherIter.internalIter);}
StructorListResult StructorList::iterator::operator*() {
	pair<Type *, Tree *> &target = (*internalIter);
	if (target.first == NULL) {
		if (*(target.second) == TOKEN_Instructor) {
			TypeStatus derivedStatus = getStatusInstructor(target.second);
			if (*derivedStatus) {
				target.first = ((FilterType *)(derivedStatus.type))->from();
			} else {
				target.first = errType;
			}
		} else /* if (*(target.second) == TOKEN_Outstructor) */ {
			TypeStatus derivedStatus = getStatusOutstructor(target.second);
			if (*derivedStatus) {
				target.first = ((FilterType *)(derivedStatus.type))->to();
			} else {
				target.first = errType;
			}
		}
	}
	return StructorListResult(target);
}
StructorList::iterator StructorList::begin() {return iterator(structors.begin());}
StructorList::iterator StructorList::end() {return iterator(structors.end());}

// MemberListResult functions
MemberListResult::MemberListResult(const pair<string, pair<Type *, Tree *> > &internalPair) : internalPair(internalPair) {}
MemberListResult::~MemberListResult() {}
MemberListResult::operator string() const {return internalPair.first;}
MemberListResult::operator Type *() const {return internalPair.second.first;}
Tree *MemberListResult::defSite() const {return internalPair.second.second;}
Type *MemberListResult::operator->() const {return internalPair.second.first;}
bool MemberListResult::operator==(const MemberListResult &otherResult) const {return internalPair.first == otherResult.internalPair.first && *(internalPair.second.first) == *(otherResult.internalPair.second.first);}
bool MemberListResult::operator!=(const MemberListResult &otherResult) const {return internalPair.first != otherResult.internalPair.first || *(internalPair.second.first) != *(otherResult.internalPair.second.first);}

// MemberList functions
MemberList::MemberList() {}
MemberList::MemberList(const MemberList &otherMemberList) : memberMap(otherMemberList.memberMap) {}
MemberList::~MemberList() {}
void MemberList::add(string name, Type *type) {memberMap.insert(make_pair(name, make_pair(type, (Tree *)NULL)));}
void MemberList::add(string name, Tree *defSite) {memberMap.insert(make_pair(name, make_pair((Type *)NULL, defSite)));}
unsigned int MemberList::size() const {return memberMap.size();}
void MemberList::clear() {memberMap.clear();}
MemberList::iterator::iterator() : internalIter() {}
MemberList::iterator::iterator(const MemberList::iterator &otherIter) : internalIter(otherIter.internalIter) {}
MemberList::iterator::iterator(const map<string, pair<Type *, Tree *> >::iterator &internalIter) : internalIter(internalIter) {}
MemberList::iterator::~iterator() {}
MemberList::iterator &MemberList::iterator::operator=(const MemberList::iterator &otherIter) {internalIter = otherIter.internalIter; return *this;}
void MemberList::iterator::operator++(int) {internalIter++;}
bool MemberList::iterator::operator==(const iterator &otherIter) {return (internalIter == otherIter.internalIter);}
bool MemberList::iterator::operator!=(const iterator &otherIter) {return (internalIter != otherIter.internalIter);}
MemberListResult MemberList::iterator::operator*() {
	pair<const string, pair<Type *, Tree *> > &lookup = (*internalIter);
	pair<Type *, Tree *> &target = lookup.second;
	if (target.first == NULL) {
		TypeStatus derivedStatus = getStatusDeclaration(target.second);
		target.first = derivedStatus.type;
	}
	return MemberListResult(lookup);
}
MemberList::iterator MemberList::begin() {return iterator(memberMap.begin());}
MemberList::iterator MemberList::end() {return iterator(memberMap.end());}
MemberList::iterator MemberList::find(const string &name) {return iterator(memberMap.find(name));}

// ObjectType functions
ObjectType::ObjectType(int suffix, int depth, Tree *offsetExp) : Type(CATEGORY_OBJECTTYPE, suffix, depth, offsetExp) {}
ObjectType::ObjectType(const StructorList &instructorList, const StructorList &outstructorList, int suffix, int depth, Tree *offsetExp) :
	Type(CATEGORY_OBJECTTYPE, suffix, depth, offsetExp), instructorList(instructorList), outstructorList(outstructorList) {}
ObjectType::ObjectType(const StructorList &instructorList, const StructorList &outstructorList, const MemberList &memberList, int suffix, int depth, Tree *offsetExp) : 
	Type(CATEGORY_OBJECTTYPE, suffix, depth, offsetExp), instructorList(instructorList), outstructorList(outstructorList), memberList(memberList) {}
ObjectType::~ObjectType() {
	for (StructorList::iterator iter = instructorList.begin(); iter != instructorList.end(); iter++) {
		if (**iter != *nullType && **iter != *errType && !((*iter)->referensible)) {
			delete (Type *)(*iter);
		}
	}
	for (StructorList::iterator iter = outstructorList.begin(); iter != outstructorList.end(); iter++) {
		if (**iter != *nullType && **iter != *errType && !((*iter)->referensible)) {
			delete (Type *)(*iter);
		}
	}
	for (MemberList::iterator iter = memberList.begin(); iter != memberList.end(); iter++) {
		if (**iter != *nullType && **iter != *errType && !((*iter)->referensible)) {
			delete (Type *)(*iter);
		}
	}
}
bool ObjectType::isComparable(const Type &otherType) const {return false;}
Type *ObjectType::copy() {ObjectType *retVal = new ObjectType(*this); retVal->referensible = true; return retVal;}
void ObjectType::erase() {clear(); delete this;}
void ObjectType::clear() {instructorList.clear(); outstructorList.clear(); memberList.clear();}
bool ObjectType::operator==(Type &otherType) {
	if (this == &otherType) { // if the objects are actually the same object instance, return true
		return true;
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		ObjectType *otherTypeCast = (ObjectType *)(&otherType);
		if ((instructorList.size() == otherTypeCast->instructorList.size()) && (outstructorList.size() == otherTypeCast->outstructorList.size()) && (memberList.size() == otherTypeCast->memberList.size())) {
			// verify that the instructors match
			StructorList::iterator insIter1 = instructorList.begin();
			StructorList::iterator insIter2 = otherTypeCast->instructorList.begin();
			while (insIter1 != instructorList.end() && insIter2 != otherTypeCast->instructorList.end()) {
				if (**insIter1 != **insIter2) {
					return false;
				}
				// advance
				insIter1++;
				insIter2++;
			}
			// verify that the outstructors match
			StructorList::iterator outsIter1 = outstructorList.begin();
			StructorList::iterator outsIter2 = otherTypeCast->outstructorList.begin();
			while (outsIter1 != outstructorList.end() && outsIter2 != otherTypeCast->outstructorList.end()) {
				if (**outsIter1 != **outsIter2) {
					return false;
				}
				// advance
				outsIter1++;
				outsIter2++;
			}
			// verify that the member names and types match
			MemberList::iterator memberListIter1 = memberList.begin();
			MemberList::iterator memberListIter2 = otherTypeCast->memberList.begin();
			while (memberListIter1 != memberList.end() && memberListIter2 != otherTypeCast->memberList.end()) {
				if (*memberListIter1 != *memberListIter2) {
					return false;
				}
				// advance
				memberListIter1++;
				memberListIter2++;
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
Type *ObjectType::operator,(Type &otherType) {
	if (otherType.category == CATEGORY_STDTYPE) {
		if (suffix == SUFFIX_LATCH) {
			for (StructorList::iterator outsIter = outstructorList.begin(); outsIter != outstructorList.end(); outsIter++) {
				Type *outstructorFlowType = (**outsIter , otherType);
				if (*outstructorFlowType) {
					return outstructorFlowType;
				}
			}
			return errType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		if (otherTypeCast->list.size() == 1) {
			return (*this , *(otherTypeCast->list[0]));
		} else if (suffix == SUFFIX_LATCH) {
			for (StructorList::iterator outsIter = outstructorList.begin(); outsIter != outstructorList.end(); outsIter++) {
				Type *outstructorFlowType = (**outsIter , *otherTypeCast);
				if (*outstructorFlowType) {
					return outstructorFlowType;
				}
			}
			return errType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		FilterType *otherTypeCast = (FilterType *)(&otherType);
		if (*this >> *(otherTypeCast->from())) {
			return otherTypeCast->to();
		} else if (suffix == SUFFIX_LATCH) {
			for (StructorList::iterator outsIter = outstructorList.begin(); outsIter != outstructorList.end(); outsIter++) {
				if (**outsIter >> *(otherTypeCast->from())) {
					return otherTypeCast->to();
				}
			}
			return errType;
		} else {
			return errType;
		}
	} else if (otherType.category == CATEGORY_OBJECTTYPE) {
		return errType;
	}
	// otherType.category == CATEGORY_ERRORTYPE
	return errType;
}

bool ObjectType::operator>>(Type &otherType) {
	if (this == &otherType) { // if the objects are actually the same object instance, allow the downcastability
		return true;
	} else if (otherType.category == CATEGORY_STDTYPE) {
		// try an outstructed downcastability
		for (StructorList::iterator outsIter = outstructorList.begin(); outsIter != outstructorList.end(); outsIter++) {
			if (**outsIter >> otherType) { // if there is a type match, return true
				return true;
			}
		}
		// else if we didn't find a type match, return false
		return false;
	} else if (otherType.category == CATEGORY_TYPELIST) {
		TypeList *otherTypeCast = (TypeList *)(&otherType);
		// try a direct downcastability
		if (otherTypeCast->list.size() == 1 && (*this >> *(otherTypeCast->list[0]))) {
			return true;
		}
		// otherwise, try an outstructed downcastability
		for (StructorList::iterator outsIter = outstructorList.begin(); outsIter != outstructorList.end(); outsIter++) {
			if (**outsIter >> *otherTypeCast) { // if there is a type match, return true
				return true;
			}
		}
		// else if we didn't find a type match, return false
		return false;
	} else if (otherType.category == CATEGORY_FILTERTYPE) {
		// try an outstructed downcastability
		for (StructorList::iterator outsIter = outstructorList.begin(); outsIter != outstructorList.end(); outsIter++) {
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
			StructorList::iterator insIter;
			for (insIter = otherTypeCast->instructorList.begin(); insIter != otherTypeCast->instructorList.end(); insIter++) {
				StructorList::iterator insIter2;
				for (insIter2 = instructorList.begin(); insIter2 != instructorList.end(); insIter2++) {
					if (**insIter2 == **insIter) {
						break;
					}
				}
				if (insIter2 == instructorList.end()) { // if we failed to find a match for this instructor, break early
					break;
				}
			}
			if (insIter == otherTypeCast->instructorList.end()) { // if we matched all instructor types (we didn't break early), continue
				// verify outstructor downcastability
				StructorList::iterator outsIter;
				for (outsIter = otherTypeCast->outstructorList.begin(); outsIter != otherTypeCast->outstructorList.end(); outsIter++) {
					StructorList::iterator outsIter2;
					for (outsIter2 = outstructorList.begin(); outsIter2 != outstructorList.end(); outsIter2++) {
						if (**outsIter2 == **outsIter) {
							break;
						}
					}
					if (outsIter2 == outstructorList.end()) { // if we failed to find a match for this outstructor, break early
						break;
					}
				}
				if (outsIter == otherTypeCast->outstructorList.end()) { // if we matched all outstructor types (we didn't break early), continue
					// verify member downcastability
					MemberList::iterator memberListIter;
					for (memberListIter = otherTypeCast->memberList.begin(); memberListIter != otherTypeCast->memberList.end(); memberListIter++) {
						MemberList::iterator memberListIter2;
						for (memberListIter2 = memberList.begin(); memberListIter2 != memberList.end(); memberListIter2++) {
							if ((*memberListIter2)->baseEquals(**memberListIter) && (*memberListIter2 == *memberListIter)) {
								break;
							}
						}
						if (memberListIter2 == memberList.end()) { // if we failed to find a match for this member, break early
							break;
						}
					}
					if (memberListIter == otherTypeCast->memberList.end()) { // if we matched all mambers (we didn't break early), return success
						return true;
					}
				}
			}
			// try to connect the left object into an instructor on the right object
			for (StructorList::iterator insIter = otherTypeCast->instructorList.begin(); insIter != otherTypeCast->instructorList.end(); insIter++) {
				if (*this >> **insIter) {
					return true;
				}
			}
			// otherwise, try an outstructed downcastability from the left object to the right object
			for (StructorList::iterator outsIter = outstructorList.begin(); outsIter != outstructorList.end(); outsIter++) {
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
	COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
	acc += '{';
	COLOR( acc += SET_TERM(RESET_CODE); )
	for (StructorList::iterator iter = instructorList.begin(); iter != instructorList.end(); iter++) {
		TYPE_TO_STRING_INDENT;
		COLOR( acc += SET_TERM(BRIGHT_CODE AND MAGENTA_CODE); )
		acc += '=';
		COLOR( acc += SET_TERM(RESET_CODE AND BRIGHT_CODE AND GREEN_CODE); )
		acc += "[";
		COLOR( acc += SET_TERM(RESET_CODE); )
		acc += (*iter)->toString(tabDepth+1);
		COLOR( acc += SET_TERM(BRIGHT_CODE AND GREEN_CODE); )
		acc += ']';
		COLOR( acc += SET_TERM(RESET_CODE); )
		StructorList::iterator tempIter = iter;
		tempIter++;
		if (tempIter != instructorList.end()) {
			COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
			acc += ", ";
			COLOR( acc += SET_TERM(RESET_CODE); )
		}
	}
	if (instructorList.size() > 0 && (outstructorList.size() > 0 || memberList.size() > 0)) {
		COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
		acc += ", ";
		COLOR( acc += SET_TERM(RESET_CODE); )
	}
	for (StructorList::iterator iter = outstructorList.begin(); iter != outstructorList.end(); iter++) {
		TYPE_TO_STRING_INDENT;
		COLOR( acc += SET_TERM(BRIGHT_CODE AND MAGENTA_CODE); )
		acc += '=';
		COLOR( acc += SET_TERM(RESET_CODE AND BRIGHT_CODE AND GREEN_CODE); )
		acc += "[--> ";
		COLOR( acc += SET_TERM(RESET_CODE); )
		acc += (*iter)->toString(tabDepth+1);
		COLOR( acc += SET_TERM(BRIGHT_CODE AND GREEN_CODE); )
		acc += ']';
		COLOR( acc += SET_TERM(RESET_CODE); )
		StructorList::iterator tempIter = iter;
		tempIter++;
		if (tempIter != outstructorList.end()) {
			COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
			acc += ", ";
			COLOR( acc += SET_TERM(RESET_CODE); )
		}
	}
	if (outstructorList.size() > 0 && memberList.size() > 0) {
		COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
		acc += ", ";
		COLOR( acc += SET_TERM(RESET_CODE); )
	}
	for (MemberList::iterator iter = memberList.begin(); iter != memberList.end(); iter++) {
		TYPE_TO_STRING_INDENT;
		COLOR( acc += SET_TERM(BRIGHT_CODE AND MAGENTA_CODE); )
		acc += (string)(*iter);
		COLOR( acc += SET_TERM(RESET_CODE); )
		acc += '=';
		acc += (*iter)->toString(tabDepth+1);
		MemberList::iterator tempIter = iter;
		tempIter++;
		if (tempIter != memberList.end()) {
			COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
			acc += ", ";
			COLOR( acc += SET_TERM(RESET_CODE); )
		}
	}
	TYPE_TO_STRING_INDENT_CLOSE;
	COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
	acc += '}';
	COLOR( acc += SET_TERM(RESET_CODE); )
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}
ObjectType::operator string() {
	TYPE_TO_STRING_HEADER;
	COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
	acc += '{';
	COLOR( acc += SET_TERM(RESET_CODE); )
	for (StructorList::iterator iter = instructorList.begin(); iter != instructorList.end(); iter++) {
		COLOR( acc += SET_TERM(BRIGHT_CODE AND MAGENTA_CODE); )
		acc += '=';
		COLOR( acc += SET_TERM(RESET_CODE AND BRIGHT_CODE AND GREEN_CODE); )
		acc += "[";
		COLOR( acc += SET_TERM(RESET_CODE); )
		acc += (string)(**iter);
		COLOR( acc += SET_TERM(BRIGHT_CODE AND GREEN_CODE); )
		acc += ']';
		COLOR( acc += SET_TERM(RESET_CODE); )
		StructorList::iterator tempIter = iter;
		tempIter++;
		if (tempIter != instructorList.end()) {
			COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
			acc += ", ";
			COLOR( acc += SET_TERM(RESET_CODE); )
		}
	}
	if (instructorList.size() > 0 && (outstructorList.size() > 0 || memberList.size() > 0)) {
		COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
		acc += ", ";
		COLOR( acc += SET_TERM(RESET_CODE); )
	}
	for (StructorList::iterator iter = outstructorList.begin(); iter != outstructorList.end(); iter++) {
		COLOR( acc += SET_TERM(BRIGHT_CODE AND MAGENTA_CODE); )
		acc += '=';
		COLOR( acc += SET_TERM(RESET_CODE AND BRIGHT_CODE AND GREEN_CODE); )
		acc += "[--> ";
		COLOR( acc += SET_TERM(RESET_CODE); )
		acc += (string)(**iter);
		COLOR( acc += SET_TERM(BRIGHT_CODE AND GREEN_CODE); )
		acc += ']';
		COLOR( acc += SET_TERM(RESET_CODE); )
		StructorList::iterator tempIter = iter;
		tempIter++;
		if (tempIter != outstructorList.end()) {
			COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
			acc += ", ";
			COLOR( acc += SET_TERM(RESET_CODE); )
		}
	}
	if (outstructorList.size() > 0 && memberList.size() > 0) {
		COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
		acc += ", ";
		COLOR( acc += SET_TERM(RESET_CODE); )
	}
	for (MemberList::iterator iter = memberList.begin(); iter != memberList.end(); iter++) {
		COLOR( acc += SET_TERM(BRIGHT_CODE AND MAGENTA_CODE); )
		acc += (string)(*iter);
		COLOR( acc += SET_TERM(RESET_CODE); )
		acc += '=';
		acc += (string)(**iter);
		MemberList::iterator tempIter = iter;
		tempIter++;
		if (tempIter != memberList.end()) {
			COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
			acc += ", ";
			COLOR( acc += SET_TERM(RESET_CODE); )
		}
	}
	COLOR( acc += SET_TERM(BRIGHT_CODE AND RED_CODE); )
	acc += '}';
	COLOR( acc += SET_TERM(RESET_CODE); )
	acc += suffixString();
	TYPE_TO_STRING_FOOTER;
}

// ErrorType functions
ErrorType::ErrorType() : Type(CATEGORY_ERRORTYPE) {}
ErrorType::~ErrorType() {}
bool ErrorType::isComparable(const Type &otherType) const {return false;}
Type *ErrorType::copy() {Type *retVal = new ErrorType(*this); retVal->referensible = true; return retVal;}
void ErrorType::erase() {delete this;}
void ErrorType::clear() {}
bool ErrorType::operator==(Type &otherType) {
	if (otherType.category == CATEGORY_ERRORTYPE) {
		return (this == &otherType);
	} else {
		return false;
	}
}
bool ErrorType::operator==(int kind) const {return false;}
Type *ErrorType::operator,(Type &otherType) {return errType;}
bool ErrorType::operator>>(Type &otherType) {return false;}
string ErrorType::toString(unsigned int tabDepth) {
	return "<ERROR>";
}
ErrorType::operator string() {return toString(1);}

// TypeStatus functions
TypeStatus::TypeStatus(Type *type, Type *retType) : type(type), retType(retType), code(NULL) {}
TypeStatus::TypeStatus(Type *type, const TypeStatus &otherStatus) : type(type), retType(otherStatus.retType), code(NULL) {}
TypeStatus::~TypeStatus() {}
TypeStatus::operator Type *() const {return type;}
TypeStatus::operator uintptr_t() const {return (unsigned int)type;}
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
Type &TypeStatus::operator*() const {
	if (type != NULL) {
		return (*type);
	} else {
		return (*errType);
	}
}
Type *TypeStatus::operator->() const {return type;}
bool TypeStatus::operator==(Type &otherType) {return (*type == otherType);}
bool TypeStatus::operator!=(Type &otherType) {return (*type != otherType);}
