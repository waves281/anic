#ifndef _SEMMER_H_
#define _SEMMER_H_

#include "mainDefs.h"
#include "system.h"
#include "constantDefs.h"

#include "lexer.h"
#include "parser.h"

// SymbolTable node kinds

#define KIND_BLOCK 1
#define KIND_STD 2
#define KIND_IMPORT 3
#define KIND_STATIC_DECL 4
#define KIND_THROUGH_DECL 5
#define KIND_PARAM 6

class SymbolTable {
	public:
		// data members
		int kind;
		string id;
		Tree *defSite; // where the symbol is defined in the Tree (Declaration or Param); NULL for root block and standard nodes
		SymbolTable *parent;
		vector<SymbolTable *> children;
		// allocators/deallocators
		SymbolTable(int kind, string id, Tree *defSite = NULL);
		SymbolTable(SymbolTable &st);
		~SymbolTable();
		// deep-copy assignment operator
		SymbolTable &operator=(SymbolTable &st);
		// concatenators
		SymbolTable &operator*=(SymbolTable *st);
};

// Type category specifiers
#define CATEGORY_TYPELIST 0
#define CATEGORY_STDTYPE 1
#define CATEGORY_FILTERTYPE 2
#define CATEGORY_OBJECTTYPE 3
#define CATEGORY_ERRORTYPE 4

// Type suffix specifiers
#define SUFFIX_CONSTANT 0
#define SUFFIX_LATCH 1
#define SUFFIX_STREAM 2
#define SUFFIX_ARRAY 3

// forward declarations
class Type;
class TypeList;
class MemberedType;
class ErrorType;
class StdType;
class FilterType;
class ObjectType;
class ErrorType;

class Type {
	public:
		// data members
		int category; // the category that this type belongs to
		int suffix; // the type suffix
		int depth; // stream depth of arrays and streams
		// mutators
		void delatch();
		// allocators/deallocators
		virtual ~Type();
		// core methods
		bool baseEquals(Type &otherType);
		bool baseSendable(Type &otherType);
		// operators
		// virtual
		virtual bool operator==(Type &otherType) = 0;
		virtual Type &operator>>(Type &otherType) = 0;
		virtual operator string() = 0;
		// non-virtual
		operator bool();
		bool operator!();
		bool operator!=(Type &otherType);
};

class TypeList : public Type {
	public:
		// data members
		vector<Type *> list; // pointers to the underlying list of types
		// allocators/deallocators
		TypeList(Tree *tree);
		TypeList();
		~TypeList();
		// operators
		bool operator==(Type &otherType);
		Type &operator>>(Type &otherType);
		operator string();
};

class ErrorType : public Type {
	public:
		// allocators/deallocators
		ErrorType();
		// operators
		bool operator==(Type &otherType);
		Type &operator>>(Type &otherType);
		operator string();
};

// Type kind specifiers

#define STD_NULL 0
#define STD_NODE 1

#define STD_MIN_COMPARABLE 2 /* anything in the range is considered comparable */

#define STD_INT 2
#define STD_FLOAT 3
#define STD_BOOL 4
#define STD_CHAR 5
#define STD_STRING 6

#define STD_MAX_COMPARABLE 6 /* anything in the range is considered comparable */

#define STD_NOT 7
#define STD_COMPLEMENT 8
#define STD_DPLUS 9
#define STD_DMINUS 10

#define STD_DOR 11
#define STD_DAND 12
#define STD_OR 13
#define STD_XOR 14
#define STD_AND 15
#define STD_DEQUALS 16
#define STD_NEQUALS 17
#define STD_LT 18
#define STD_GT 19
#define STD_LE 20
#define STD_GE 21
#define STD_LS 22
#define STD_RS 23
#define STD_TIMES 24
#define STD_DIVIDE 25
#define STD_MOD 26

#define STD_PLUS 27
#define STD_MINUS 28

class StdType : public Type {
	public:
		// data members
		int kind; // the class of type that this is
		// allocators/deallocators
		StdType(int kind, int suffix = SUFFIX_CONSTANT, int depth = 0);
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		Type &operator>>(Type &otherType);
		operator string();
};

class FilterType : public Type {
	public:
		// data members
		TypeList *from; // the source of this object type
		TypeList *to; // the destination of this object type
		// allocators/deallocators
		FilterType(TypeList *from, TypeList *to, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~FilterType();
		// operators
		bool operator==(Type &otherType);
		Type &operator>>(Type &otherType);
		operator string();
};

class ObjectType : public Type {
	public:
		// data members
		SymbolTable *base; // the Node that defines this type (base->defSite is Declaration->TypedStaticTerm->Node->Object)
		vector<TypeList *> constructorTypes; // list of the types of this object's constructors (each one is a TypeList)
		vector<string> memberNames; // list of names of raw non-constructor members of this object
		vector<Type *> memberTypes; // list of types of raw non-constructor members of this object
		// allocators/deallocators
		ObjectType(SymbolTable *base, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~ObjectType();
		// operators
		bool operator==(Type &otherType);
		Type &operator>>(Type &otherType);
		operator string();
};

// forward declarations of mutually recursive typing functions

Type *getTypeSuffixedIdentifier(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypePrefixOrMultiOp(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypePrimary(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeExp(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypePrimOpNode(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypePrimLiteral(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeBlock(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeFilterHeader(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeFilter(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeObjectBlock(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeTypeList(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeParamList(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeRetList(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeNodeInstantiation(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeNode(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeTypedStaticTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeStaticTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeDynamicTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeSwitchTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeSimpleTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeSimpleCondTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeClosedTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeOpenTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeOpenCondTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeClosedCondTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeTerm(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeNonEmptyTerms(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypeDeclaration(Type *inType, Tree *recallBinding, Tree *tree);
Type *getTypePipe(Type *inType, Tree *recallBinding, Tree *tree);

// semantic analysis helper blocks

#define GET_TYPE_HEADER \
	/* if the type is memoized, short-circuit evaluate */\
	if (tree->type != NULL) {\
		return tree->type;\
	}\
	/* otherwise, compute the type normally */\
	Type *type = NULL

#define GET_TYPE_FOOTER \
	/* if we could't resolve a valid type, use the error type */\
	if (type == NULL) {\
		type = errType;\
	}\
	/* latch the type to the tree node */\
	tree->type = type;\
	/* return the derived type */\
	return type

// main semantic analysis function

int sem(Tree *treeRoot, vector<Tree *> *parseme, SymbolTable *&stRoot);

#endif
