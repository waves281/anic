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
#define CATEGORY_READ 3
#define CATEGORY_MEM 4
#define CATEGORY_UNOP 5
#define CATEGORY_BINOP 6
#define CATEGORY_LOCK 7
#define CATEGORY_UNLOCK 8
#define CATEGORY_WRITE 9
#define CATEGORY_SCHEDULE 10

// forward declarations
class IRTree;
	class LabelTree;
	class MemTree;
	class DataTree;
		class ConstTree;
		class TempTree;
		class ReadTree;
	class OpTree;
		class UnOpTree;
		class BinOpTree;
	class CodeTree;
		class LockTree;
		class UnlockTree;
		class WriteTree;
		class ScheduleTree;

// IRTree classes

class IRTree {
	public:
		// data members
		int category; // the category that this IRTree belongs to
		// allocators/deallocators
		virtual ~IRTree();
};

// LabelTree classes

// usage: print out assembler label id followed by the sequential code in seqList
class LabelTree : public IRTree {
	public:
		// data members
		string id; // string representation of this label
		vector<CodeTree *> seqList; // vector representing the sequencing of CodeTrees that must be executed in order
		// allocators/deallocators
		LabelTree(const string &id, const vector<CodeTree *> &seqList);
		~LabelTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// MemTree classes

// usage: represents memory from bytes (base) to (base + length - 1) that has been statically filled with the given raw data vector
class MemTree : public IRTree {
	public:
		// data members
		uint32_t base; // the base address of the data
		uint32_t length; // the length of the data block
		vector<unsigned char> data; // the raw data initialization vector, if any, for this node
		// allocators/deallocators
		MemTree(unsigned int base, unsigned int length);
		~MemTree();
		// core methods
		string toString(unsigned int tabDepth) const;
		// operators
		MemTree *operator+(unsigned int offset) const;
};

// DataTree classes

// usage: abstract class; never used directly
class DataTree : public IRTree {
	public:
		// allocators/deallocators
		virtual ~DataTree();
		// core methods
		virtual string toString(unsigned int tabDepth = 1) const = 0;
};

// usage: an in-place inclusion of a data vector
class ConstTree : public DataTree {
	public:
		// data members
		vector<unsigned char> data; // vector of the raw data contained in this node
		// allocators/deallocators
		ConstTree(const vector<unsigned char> &data);
		ConstTree(uint32_t data);
		~ConstTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: allocate temporary storage for the result of an operation
class TempTree : public DataTree {
	public:
		// data members
		OpTree *opNode; // pointer to the operation that this temporary is holding the result of
		// allocators/deallocators
		TempTree(OpTree *opNode);
		~TempTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: read memory from the specified node
class ReadTree : public DataTree {
	public:
		// data members
		MemTree *memNode; // pointer to the node of memory that we want to read
		// allocators/deallocators
		ReadTree(MemTree *memNode);
		~ReadTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// OpTree classes

// usage: abstract class; never used directly
class OpTree : public IRTree {
	public:
		// allocators/deallocators
		virtual ~OpTree();
		// core methods
		virtual string toString(unsigned int tabDepth = 1) const = 0;
};

// usage: perform the given unary operation on the subnode
class UnOpTree : public OpTree {
	public:
		// data members
		int kind; // the kind of operator to apply to the subnode
		DataTree *subNode; // pointer to the data subnode that we're applying the operator to
		// allocators/deallocators
		UnOpTree(int kind, DataTree *subNode);
		~UnOpTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: perform the given binary operation on subNodeLeft and subNodeRight
class BinOpTree : public OpTree {
	public:
		// data members
		int kind; // the kind of operator to apply to the subnodes
		DataTree *subNodeLeft; // pointer to the data subnode representing the left operand
		DataTree *subNodeRight; // pointer to the data subnode representing the right operand
		// allocators/deallocators
		BinOpTree(int kind, DataTree *subNodeLeft, DataTree *subNodeRight);
		~BinOpTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// CodeTree classes

// usage: abstract class; never used directly
class CodeTree : public IRTree {
	public:
		// allocators/deallocators
		virtual ~CodeTree();
		// core methods
		virtual string toString(unsigned int tabDepth = 1) const = 0;
};

// usage: grab a lock on memory node subNode
class LockTree : public CodeTree {
	public:
		// data members
		MemTree *memNode; // pointer to the data subnode that we want to lock
		// allocators/deallocators
		LockTree(MemTree *memNode);
		~LockTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: release the lock on memory node subNode
class UnlockTree : public CodeTree {
	public:
		// data members
		MemTree *memNode; // pointer to the memory subnode that we want to unlock
		// allocators/deallocators
		UnlockTree(MemTree *memNode);
		~UnlockTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: allocate room for, and copy the memory of the data node subNode
class WriteTree : public CodeTree {
	public:
		// data members
		DataTree *src; // pointer to the source data subnode
		MemTree *dst; // pointer to the destination memory subnode
		// allocators/deallocators
		WriteTree(DataTree *src, MemTree *dst);
		~WriteTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: schedule all of the label nodes in the labelList for execution
class ScheduleTree : public CodeTree {
	public:
		// data members
		vector<LabelTree *> labelList; // vector of the label nodes to schedule
		// allocators/deallocators
		ScheduleTree(const vector<LabelTree *> &labelList);
		~ScheduleTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// main code generation function

int gen(Tree *treeRoot, SymbolTable *stRoot, CodeTree *&codeRoot);

#endif
