#ifndef _TYPES_H_
#define _TYPES_H_

#include "globalDefs.h"
#include "constantDefs.h"
#include "driver.h"

#include "../tmp/parserStruct.h"

// forward declarations
class Tree;
class SymbolTable;
class RepTree;
class IRTree;

// Type category specifiers
#define CATEGORY_TYPELIST 0
#define CATEGORY_STDTYPE 1
#define CATEGORY_FILTERTYPE 2
#define CATEGORY_OBJECTTYPE 3
#define CATEGORY_ERRORTYPE 4

// Type suffix specifiers
#define SUFFIX_CONSTANT 0
#define SUFFIX_LATCH 1
#define SUFFIX_LIST 2
#define SUFFIX_STREAM 3
#define SUFFIX_ARRAY 4
#define SUFFIX_POOL 5

// forward declarations
class Type;
class TypeList;
class MemberedType;
class ErrorType;
class StdType;
class FilterType;
class StructorListResult;
class StructorList;
class MemberListResult;
class MemberList;
class ObjectType;
class ErrorType;
class TypeStatusBase;
class TypeStatus;
class SchedTree;
class DataTree;

// core Type class

class Type {
	public:
		// data members
		int category; // the category that this Type belongs to
		int suffix; // the type suffix (constant, latch, stream, or array)
		int depth; // stream depth of arrays and streams
		bool operable; // whether a node of this type can be operated upon
		bool toStringHandled; // used for recursion detection in operator string()
		// mutators
		void constantize(); // for when an identifier is present without an accessor or a sub-identifier's type is constrained by upstream identifiers
		void latchize(); // for when we're instantiating a node with a single initializer
		void poolize(); // for when we're instantiating a node with a multi initializer
		void decreaseDepth();
		bool delatch() const;
		bool delist();
		bool destream();
		void copyDelatch();
		bool pack();
		bool unpack();
		Type *link(const Type &otherType) const;
		// allocators/deallocators
		Type(int category, int suffix = SUFFIX_CONSTANT, int depth = 0);
		virtual ~Type() = 0;
		// core methods
		// virtual
		virtual bool isComparable(const Type &otherType) const = 0;
		virtual Type *copy() = 0;
		virtual void erase() = 0;
		virtual void clear() = 0;
		virtual string toString(unsigned int tabDepth = 1) = 0;
		// non-virtual
		bool baseEquals(const Type &otherType) const;
		bool baseSendable(const Type &otherType) const;
		string suffixString() const;
		// operators
		// virtual
		virtual bool operator==(Type &otherType) = 0;
		virtual bool operator==(int kind) const = 0;
		virtual Type *operator,(Type &otherType) = 0;
		virtual bool operator>>(Type &otherType) = 0;
		virtual operator string() = 0;
		// non-virtual
		operator bool() const;
		bool operator!() const;
		bool operator!=(Type &otherType);
		bool operator!=(int kind) const;
};

// Type subclasses

class TypeList : public Type {
	public:
		// data members
		vector<Type *> list; // pointers to the underlying list of types
		// allocators/deallocators
		TypeList(const vector<Type *> &list);
		TypeList(Type *type);
		TypeList();
		~TypeList();
		// core methods
		bool isComparable(const Type &otherType) const;
		Type *copy();
		void erase();
		void clear();
		string toString(unsigned int tabDepth);
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind) const;
		Type *operator,(Type &otherType);
		bool operator>>(Type &otherType);
		operator string();
};

class ErrorType : public Type {
	public:
		// allocators/deallocators
		ErrorType();
		~ErrorType();
		// core methods
		bool isComparable(const Type &otherType) const;
		Type *copy();
		void erase();
		void clear();
		string toString(unsigned int tabDepth);
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind) const;
		Type *operator,(Type &otherType);
		bool operator>>(Type &otherType);
		operator string();
};

// Type kind specifiers

#define STD_NULL 0
#define STD_STD 1

#define STD_MIN_COMPARABLE 2

#define STD_BOOL 2
#define STD_INT 3
#define STD_FLOAT 4
#define STD_CHAR 5
#define STD_STRING 6

#define STD_MAX_COMPARABLE 6

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
		int kind; // the kind of standard type that this is
		// allocators/deallocators
		StdType(int kind, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~StdType();
		// core methods
		bool isComparable() const;
		bool isComparable(const Type &otherType) const;
		int kindCast(const StdType &otherType) const; // returns kind resulting from sending *this to otherType, STD_NULL if the comparison is invalid
		pair<Type *, bool> stdFlowDerivation(const TypeStatus &prevStatus, Tree *nextTerm) const; // bool is whether we consumed nextTerm in the derivation
		bool objectTypePromotion(Type &otherType) const; // returns whether we can specially promote this StdType to the given ObjectType
		Type *copy();
		void erase();
		void clear();
		string kindToString() const;
		string toString(unsigned int tabDepth);
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind) const;
		Type *operator,(Type &otherType);
		bool operator>>(Type &otherType);
		operator string();
};

class FilterType : public Type {
	public:
		// data members
		TypeList *fromInternal; // the source of this object type
		TypeList *toInternal; // the destination of this object type
		Tree *defSite; // the FilterHeader tree node that defines this FilterType
		// allocators/deallocators
		FilterType(Type *fromInternal = nullType, Type *toInternal = nullType, int suffix = SUFFIX_CONSTANT, int depth = 0);
		FilterType(Tree *defSite, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~FilterType();
		// core methods
		TypeList *from();
		TypeList *to();
		bool isComparable(const Type &otherType) const;
		Type *copy();
		void erase();
		void clear();
		string toString(unsigned int tabDepth);
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind) const;
		Type *operator,(Type &otherType);
		bool operator>>(Type &otherType);
		operator string();
};

class StructorListResult {
	public:
		// data members
		const pair<Type *, Tree *> &internalPair;
		// allocators/deallocators
		StructorListResult(const pair<Type *, Tree *> &internalPair);
		~StructorListResult();
		// converters
		operator Type *() const;
		// core methods
		Tree *defSite() const;
		// operators
		Type *operator->() const;
		bool operator==(const StructorListResult &otherResult) const;
		bool operator!=(const StructorListResult &otherResult) const;
};

class StructorList {
	public:
		// data members
		vector<pair<Type *, Tree *> > structors;
		// allocators/deallocators
		StructorList();
		StructorList(const StructorList &otherStructorList);
		~StructorList();
		// core methods
		void add(TypeList *typeList);
		void add(Tree *tree);
		unsigned int size() const;
		void clear();
		// iterator methods
		class iterator {
			public:
				// data members
				vector<pair<Type *, Tree *> >::iterator internalIter;
				// allocators/deallocators
				iterator();
				iterator(const iterator &otherIter);
				iterator(const vector<pair<Type *, Tree *> >::iterator &internalIter);
				~iterator();
				// operators
				iterator &operator=(const iterator &otherIter);
				void operator++(int);
				bool operator==(const iterator &otherIter);
				bool operator!=(const iterator &otherIter);
				StructorListResult operator*();
		};
		iterator begin();
		iterator end();
};

class MemberListResult {
	public:
		// data members
		const pair<string, pair<Type *, Tree *> > &internalPair;
		// allocators/deallocators
		MemberListResult(const pair<string, pair<Type *, Tree *> > &internalPair);
		~MemberListResult();
		// converters
		operator string() const;
		operator Type *() const;
		// core methods
		Tree *defSite() const;
		// operators
		Type *operator->() const;
		bool operator==(const MemberListResult &otherResult) const;
		bool operator!=(const MemberListResult &otherResult) const;
};

class MemberList {
	public:
		// data members
		map<string, pair<Type *, Tree *> > memberMap;
		// allocators/deallocators
		MemberList();
		MemberList(const MemberList &otherMemberList);
		~MemberList();
		// core methods
		void add(string name, Type *type);
		void add(string name, Tree *tree);
		unsigned int size() const;
		void clear();
		// iterator methods
		class iterator {
			public:
				// data members
				map<string, pair<Type *, Tree *> >::iterator internalIter;
				// allocators/deallocators
				iterator();
				iterator(const iterator &otherIter);
				iterator(const map<const string, pair<Type *, Tree *> >::iterator &internalIter);
				~iterator();
				// operators
				iterator &operator=(const iterator &otherIter);
				void operator++(int);
				bool operator==(const iterator &otherIter);
				bool operator!=(const iterator &otherIter);
				MemberListResult operator*();
		};
		iterator begin();
		iterator end();
		iterator find(const string &name);
};

class ObjectType : public Type {
	public:
		// data members
		StructorList instructorList; // list of the types of this object's instructors (each one is a TypeList)
		StructorList outstructorList; // list of the types of this object's outstructors (each one is a TypeList)
		MemberList memberList; // smart map of the names of members to their types
		// allocators/deallocators
		ObjectType(int suffix = SUFFIX_CONSTANT, int depth = 0);
		ObjectType(const StructorList &instructorList, const StructorList &outstructorList, int suffix = SUFFIX_CONSTANT, int depth = 0);
		ObjectType(const StructorList &instructorList, const StructorList &outstructorList, const MemberList &memberList, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~ObjectType();
		// core methods
		bool isComparable(const Type &otherType) const;
		Type *copy();
		void erase();
		void clear();
		string toString(unsigned int tabDepth);
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind) const;
		Type *operator,(Type &otherType);
		bool operator>>(Type &otherType);
		operator string();
};

// typing status class

class TypeStatus {
	public:
		// data members
		Type *type; // the type derived for this parse tree node
		Type *retType; // the carry-over return type derived for this parse tree node
		RepTree *rep; // the memory representation tree for this parse tree node
		IRTree *code; // the intermediate representation code tree for this parse tree node
		// allocators/deallocators
		TypeStatus(Type *type = NULL, Type *retType = NULL);
		TypeStatus(Type *type, const TypeStatus &otherStatus);
		~TypeStatus();
		// converters
		operator Type *() const;
		operator unsigned int() const;
		// core methods
		DataTree *castCode(const Type &destType) const;
		DataTree *castCommonCode(const Type &otherType) const;
		// operators
		TypeStatus &operator=(const TypeStatus &otherStatus);
		TypeStatus &operator=(Type *otherType);
		Type &operator*() const;
		Type *operator->() const;
		bool operator==(Type &otherType);
		bool operator!=(Type &otherType);
};

// external linkage specifiers
extern Type *nullType;
extern Type *errType;

// post-includes
#include "parser.h"
#include "genner.h"

// Type to string helper blocks

#define TYPE_TO_STRING_HEADER \
	/* if we have reached a recursive loop, return this fact */\
	if (toStringHandled) {\
		return "<RECURSION>";\
	} else { /* otherwise, set the recursive printing flag */\
		toStringHandled = true;\
	}\
	/* prepare to compute the string normally */\
	string acc

#define TYPE_TO_STRING_FOOTER \
	/* unset the recursive printing flag */\
	toStringHandled = false;\
	/* return the completed accumulator */\
	return acc

#define TYPE_TO_STRING_INDENT \
	acc += "\n\t| ";\
	for (unsigned int i=0; i < tabDepth; i++)\
		acc += "  "

#define TYPE_TO_STRING_INDENT_CLOSE \
	acc += "\n\t| ";\
	for (unsigned int i=0; i < tabDepth-1; i++)\
		acc += "  "

#endif
