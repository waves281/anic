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
#define CATEGORY_UNOP 4
#define CATEGORY_BINOP 5
#define CATEGORY_LOCK 6
#define CATEGORY_UNLOCK 7
#define CATEGORY_COND 8
#define CATEGORY_JUMP 9
#define CATEGORY_WRITE 10
#define CATEGORY_SCHEDULE 11

// forward declarations
class IRTree;
	class LabelTree;
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
		class CondTree;
		class JumpTree;
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

// usage: read memory from the specified memory address
class ReadTree : public DataTree {
	public:
		// data members
		DataTree *address; // pointer to the data subnode specifying the memory address to read from
		// allocators/deallocators
		ReadTree(DataTree *address);
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

// usage: grab a lock on the specified memory address
class LockTree : public CodeTree {
	public:
		// data members
		DataTree *address; // pointer to the data subnode specifying the memory address to lock
		// allocators/deallocators
		LockTree(DataTree *address);
		~LockTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: release a lock on the specified memory address
class UnlockTree : public CodeTree {
	public:
		// data members
		DataTree *address; // pointer to the data subnode specifying the memory address to unlock
		// allocators/deallocators
		UnlockTree(DataTree *address);
		~UnlockTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: if the boolean data word test is true, run trueBranch; otherwise, run falseBranch
class CondTree : public CodeTree {
	public:
		// data members
		DataTree *test; // pointer to the data subnode specifying the boolean value of the test condition
		CodeTree *trueBranch; // pointer to the code tree to jump to on a true test value
		CodeTree *falseBranch; // pointer to the code tree to jump to on a true test value
		// allocators/deallocators
		CondTree(DataTree *test, CodeTree *trueBranch, CodeTree *falseBranch);
		~CondTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: unconditionally jump to the specified memory address
class JumpTree : public CodeTree {
	public:
		// data members
		DataTree *address; // pointer to the data subnode specifying the address of the location to jump to
		// allocators/deallocators
		JumpTree(DataTree *address);
		~JumpTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: write the source data to the specified destination memory address
class WriteTree : public CodeTree {
	public:
		// data members
		DataTree *source; // pointer to the source data subnode
		DataTree *address; // pointer to the subnode specifying the destination memory address
		// allocators/deallocators
		WriteTree(DataTree *sorce, DataTree *address);
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
