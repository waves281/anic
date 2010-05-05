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
class TypeStatus;

class Type {
	public:
		// data members
		int category; // the category that this type belongs to
		int suffix; // the type suffix (constant, latch, stream, or array)
		int depth; // stream depth of arrays and streams
		// mutators
		void delatch();
		// allocators/deallocators
		virtual ~Type();
		// core methods
		// virtual
		virtual bool isComparable() = 0;
		// non-virtual
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
		TypeList(Tree *tree, Tree *&recall);
		TypeList(Type *type);
		TypeList();
		~TypeList();
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type &operator>>(Type &otherType);
		operator string();
};

class ErrorType : public Type {
	public:
		// allocators/deallocators
		ErrorType();
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type &operator>>(Type &otherType);
		operator string();
};

// Type kind specifiers

#define STD_NULL 0

#define STD_MIN_COMPARABLE 1 /* anything in the range is considered comparable */

#define STD_INT 1
#define STD_FLOAT 2
#define STD_BOOL 3
#define STD_CHAR 4
#define STD_STRING 5

#define STD_MAX_COMPARABLE 5 /* anything in the range is considered comparable */

#define STD_NOT 6
#define STD_COMPLEMENT 7
#define STD_DPLUS 8
#define STD_DMINUS 9

#define STD_DOR 10
#define STD_DAND 11
#define STD_OR 12
#define STD_XOR 13
#define STD_AND 14
#define STD_DEQUALS 15
#define STD_NEQUALS 16
#define STD_LT 17
#define STD_GT 18
#define STD_LE 19
#define STD_GE 20
#define STD_LS 21
#define STD_RS 22
#define STD_TIMES 23
#define STD_DIVIDE 24
#define STD_MOD 25

#define STD_PLUS 26
#define STD_MINUS 27

class StdType : public Type {
	public:
		// data members
		int kind; // the class of type that this is
		// allocators/deallocators
		StdType(int kind, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~StdType();
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type &operator>>(Type &otherType);
		operator string();
};

class FilterType : public Type {
	public:
		// data members
		TypeList *from; // the source of this object type
		TypeList *to; // the destination of this object type
		// allocators/deallocators
		FilterType(Type *from, Type *to, int suffix = SUFFIX_CONSTANT, int depth = 0);
		FilterType(Type *from, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~FilterType();
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
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
		ObjectType(SymbolTable *base, Tree *&recall, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~ObjectType();
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type &operator>>(Type &otherType);
		operator string();
};

// typing status block

class TypeStatus {
	public:
		// data members
		Type *type; // the encapsulated type
		Tree *recall; // the encapsulated recall binding
		// allocators/deallocators
		TypeStatus();
		TypeStatus(Type *type, Tree *recall = NULL);
		~TypeStatus();
		// converters
		operator Type *();
		operator Tree *();
		operator bool();
		// operators
		TypeStatus &operator=(TypeStatus &otherStatus);
		TypeStatus &operator=(Type *otherType);
		Type &operator*();
		bool operator==(Type &otherType);
		bool operator!=(Type &otherType);
};

// forward declarations of mutually recursive typing functions

TypeStatus getTypeSuffixedIdentifier(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypePrefixOrMultiOp(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypePrimary(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeBracketedExp(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeExp(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypePrimOpNode(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypePrimLiteral(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeBlock(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeFilterHeader(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeFilter(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeObjectBlock(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeTypeList(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeParamList(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeRetList(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeNodeInstantiation(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeNodeSoft(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeNode(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeTypedStaticTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeStaticTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeDynamicTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeSwitchTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeSimpleTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeSimpleCondTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeClosedTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeOpenTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeOpenCondTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeClosedCondTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeNonEmptyTerms(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeDeclaration(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypePipe(Tree *tree, TypeStatus inStatus = TypeStatus());

// semantic analysis helper blocks

#define GET_TYPE_HEADER \
	/* if the type is memoized, short-circuit evaluate */\
	if (tree->type != NULL) {\
		return tree->type;\
	}\
	/* otherwise, compute the type normally */\
	TypeStatus outStatus;\
	Type *&type = outStatus.type

#define GET_TYPE_FOOTER \
	/* if we could't resolve a valid type, use the error type */\
	if (type == NULL) {\
		type = errType;\
	}\
	/* latch the status to the tree node */\
	tree->status = outStatus;\
	/* return the derived status block */\
	return outStatus

// main semantic analysis function

int sem(Tree *treeRoot, vector<Tree *> *parseme, SymbolTable *&stRoot);

#endif
