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
#define CATEGORY_SEQ 8

// forward declarations
class CodeTree;

// core CodeTree class

class CodeTree {
	public:
		// data members
		int category; // the category that this CodeTree belongs to
		// allocators/deallocators
		virtual ~CodeTree();
		// operators
		virtual operator string() const = 0;
};

// CodeTree subclasses

class LabelCodeTree : public CodeTree {
	public:
		// data members
		string id; // string representation of this label
		// allocators/deallocators
		LabelCodeTree(const string &id);
		~LabelCodeTree();
		// operators
		operator string() const;
};

class DataCodeTree : public CodeTree {
	public:
		// data members
		unsigned int offset; // the offset into the data array of this node
		// allocators/deallocators
		virtual ~DataCodeTree();
		// operators
		// non-virtual
		DataCodeTree *operator+(unsigned int offset); // adjust the offset upwards
		DataCodeTree *operator-(unsigned int offset); // adjust the offset downwards
};

class ConstCodeTree : public DataCodeTree {
	public:
		// data members
		vector<unsigned int> data; // vector of the raw data contained in this constant
		// allocators/deallocators
		ConstCodeTree(const vector<unsigned int> &data);
		~ConstCodeTree();
		// operators
		operator string() const;
};

class TempCodeTree : public DataCodeTree {
	public:
		// data members
		unsigned int size; // how much space (in bytes) this temporary node holds
		// allocators/deallocators
		TempCodeTree(unsigned int size);
		~TempCodeTree();
		// operators
		operator string() const;
};

class UnOpCodeTree : public CodeTree {
	public:
		// data members
		int kind; // the kind of operator to apply to the subnode
		DataCodeTree *subNode; // pointer to the data subnode that we're applying the operator to
		// allocators/deallocators
		UnOpCodeTree(int kind, DataCodeTree *subNode);
		~UnOpCodeTree();
		// operators
		operator string() const;
};

class BinOpCodeTree : public CodeTree {
	public:
		// data members
		int kind; // the kind of operator to apply to the subnodes
		DataCodeTree *subNodeLeft; // pointer to the data subnode representing the left operand
		DataCodeTree *subNodeRight; // pointer to the data subnode representing the right operand
		// allocators/deallocators
		BinOpCodeTree(int kind, DataCodeTree *subNodeLeft, DataCodeTree *subNodeRight);
		~BinOpCodeTree();
		// operators
		operator string() const;
};

class LockCodeTree : public CodeTree {
	public:
		// data members
		DataCodeTree *subNode; // pointer to the data subnode that we want to lock
		// allocators/deallocators
		LockCodeTree(DataCodeTree *subNode);
		~LockCodeTree();
		// operators
		operator string() const;
};

class UnlockCodeTree : public CodeTree {
	public:
		// data members
		DataCodeTree *subNode; // pointer to the data subnode that we want to unlock
		// allocators/deallocators
		UnlockCodeTree(DataCodeTree *subNode);
		~UnlockCodeTree();
		// operators
		operator string() const;
};

class CopyCodeTree : public CodeTree {
	public:
		// data members
		DataCodeTree *source; // pointer to the source data subnode
		// allocators/deallocators
		CopyCodeTree(DataCodeTree *source);
		~CopyCodeTree();
		// operators
		operator string() const;
};

class SeqCodeTree : public CodeTree {
	public:
		// data members
		vector<CodeTree *> seqList; // vector representing the sequencing of CodeTrees that must be executed in order
		// allocators/deallocators
		SeqCodeTree();
		~SeqCodeTree();
		// operators
		operator string() const;
};

// main code generation function

int gen(Tree *treeRoot, SymbolTable *stRoot, CodeTree *&codeRoot);

#endif
