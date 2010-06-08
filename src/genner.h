#ifndef _GENNER_H_
#define _GENNER_H_

#include "globalDefs.h"
#include "constantDefs.h"
#include "driver.h"

#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "semmer.h"

// CodeTree category specifiers
#define CATEGORY_LABEL 0
#define CATEGORY_CONST 1
#define CATEGORY_TEMP 2
#define CATEGORY_UNOP 3
#define CATEGORY_BINOP 4
#define CATEGORY_LOCK 5
#define CATEGORY_UNLOCK 6
#define CATEGORY_COPY 7

// forward declarations
class CodeTree;
class LabelCodeTree;
class DataCodeTree;
class ConstTree;
class TempTree;
class UnOpCodeTree;
class BinOpCodeTree;
class LockCodeTree;
class UnlockCodeTree;
class CopyCodeTree;


// core CodeTree class

// usage: abstract class; never used directly
class CodeTree {
	public:
		// data members
		int category; // the category that this CodeTree belongs to
		// allocators/deallocators
		virtual ~CodeTree();
		// core methods
		virtual string toString(unsigned int tabDepth = 1) const = 0;
};

// CodeTree subclasses

// usage: print out assembler label id followed by the sequential code in seqList
class LabelCodeTree : public CodeTree {
	public:
		// data members
		string id; // string representation of this label
		vector<CodeTree *> seqList; // vector representing the sequencing of CodeTrees that must be executed in order
		// allocators/deallocators
		LabelCodeTree(const string &id, const vector<CodeTree *> &seqList);
		~LabelCodeTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: abstract class; never used directly
class DataCodeTree : public CodeTree {
	public:
		// data members
		unsigned int offset; // the offset into the data array of this node
		// allocators/deallocators
		virtual ~DataCodeTree();
		// operators
		virtual DataCodeTree *operator+(unsigned int offset) const = 0; // adjust the offset upwards
		virtual DataCodeTree *operator-(unsigned int offset) const = 0; // adjust the offset downwards
};

// usage: place word-vector data into memory
class ConstCodeTree : public DataCodeTree {
	public:
		// data members
		vector<uint32_t> data; // vector of the raw data contained in this constant
		// allocators/deallocators
		ConstCodeTree(const vector<uint32_t> &data);
		~ConstCodeTree();
		// core methods
		string toString(unsigned int tabDepth) const;
		// operators
		DataCodeTree *operator+(unsigned int offset) const;
		DataCodeTree *operator-(unsigned int offset) const;
};

// usage: allocate space of the given size in memory
class TempCodeTree : public DataCodeTree {
	public:
		// data members
		unsigned int size; // how much space (in bytes) this temporary node holds
		// allocators/deallocators
		TempCodeTree(unsigned int size);
		~TempCodeTree();
		// core methods
		string toString(unsigned int tabDepth) const;
		// operators
		DataCodeTree *operator+(unsigned int offset) const;
		DataCodeTree *operator-(unsigned int offset) const;
};

// usage: perform the given unary operation on the subnode
class UnOpCodeTree : public CodeTree {
	public:
		// data members
		int kind; // the kind of operator to apply to the subnode
		DataCodeTree *subNode; // pointer to the data subnode that we're applying the operator to
		// allocators/deallocators
		UnOpCodeTree(int kind, DataCodeTree *subNode);
		~UnOpCodeTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: perform the given binary operation on subNodeLeft and subNodeRight
class BinOpCodeTree : public CodeTree {
	public:
		// data members
		int kind; // the kind of operator to apply to the subnodes
		DataCodeTree *subNodeLeft; // pointer to the data subnode representing the left operand
		DataCodeTree *subNodeRight; // pointer to the data subnode representing the right operand
		// allocators/deallocators
		BinOpCodeTree(int kind, DataCodeTree *subNodeLeft, DataCodeTree *subNodeRight);
		~BinOpCodeTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: grab a lock on data node subNode
class LockCodeTree : public CodeTree {
	public:
		// data members
		DataCodeTree *subNode; // pointer to the data subnode that we want to lock
		// allocators/deallocators
		LockCodeTree(DataCodeTree *subNode);
		~LockCodeTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: release the lock on data node subNode
class UnlockCodeTree : public CodeTree {
	public:
		// data members
		DataCodeTree *subNode; // pointer to the data subnode that we want to unlock
		// allocators/deallocators
		UnlockCodeTree(DataCodeTree *subNode);
		~UnlockCodeTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: allocate room for, and copy the memory of the data node subNode
class CopyCodeTree : public CodeTree {
	public:
		// data members
		DataCodeTree *source; // pointer to the source data subnode
		// allocators/deallocators
		CopyCodeTree(DataCodeTree *source);
		~CopyCodeTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// main code generation function

int gen(Tree *treeRoot, SymbolTable *stRoot, CodeTree *&codeRoot);

#endif
