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
#define CATEGORY_SEQ 1
#define CATEGORY_CONST 2
#define CATEGORY_TEMP 3
#define CATEGORY_READ 4
#define CATEGORY_UNOP 5
#define CATEGORY_BINOP 6
#define CATEGORY_CONVOP 7
#define CATEGORY_LOCK 8
#define CATEGORY_UNLOCK 9
#define CATEGORY_COND 10
#define CATEGORY_JUMP 11
#define CATEGORY_WRITE 12
#define CATEGORY_COPY 13
#define CATEGORY_SCHED 14

// forward declarations
class IRTree;
	class LabelTree;
	class SeqTree;
	class DataTree;
		class ConstTree;
		class TempTree;
		class ReadTree;
	class OpTree;
		class UnOpTree;
		class BinOpTree;
		class ConvTree;
	class CodeTree;
		class LockTree;
		class UnlockTree;
		class CondTree;
		class JumpTree;
		class WriteTree;
		class CopyTree;
		class ScheduleTree;

// IRTree classes

class IRTree {
	public:
		// data members
		int category; // the category that this IRTree belongs to
		// allocators/deallocators
		IRTree(int category);
		virtual ~IRTree();
};

// LabelTree classes

// usage: identifier-labeled sequential code block; an atomic unit of execution
class LabelTree : public IRTree {
	public:
		// data members
		string id; // string representation of this label
		SeqTree *code; // pointer to the tree node containing the sequence of code trees to run
		// allocators/deallocators
		LabelTree(const string &id, SeqTree *code);
		~LabelTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// SeqTree classes

// usage: a sequence of code to execute
class SeqTree : public IRTree {
	public:
		// data members
		string id; // string representation of this label
		vector<CodeTree *> codeList; // vector representing the sequence of code trees to execute in order
		// allocators/deallocators
		SeqTree(const vector<CodeTree *> &seqList);
		~SeqTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// DataTree classes

// usage: abstract class; never used directly
class DataTree : public IRTree {
	public:
		// allocators/deallocators
		DataTree(int category);
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
		// data members
		int kind; // the kind of operator to apply to the subnode(s); the valid values are defined above the subclass that they apply to
		// allocators/deallocators
		OpTree(int category, int kind);
		virtual ~OpTree();
		// core methods
		string kindToString() const;
		virtual string toString(unsigned int tabDepth = 1) const = 0;
};

// definitions of arithmetic operator kinds
#define UNOP_NOT_BOOL 0

#define UNOP_COMPLEMENT_INT 1

#define UNOP_DPLUS_INT 2
#define UNOP_DMINUS_INT 3

#define BINOP_DOR_BOOL 4
#define BINOP_DAND_BOOL 5

#define BINOP_OR_INT 6
#define BINOP_XOR_INT 7
#define BINOP_AND_INT 8

#define BINOP_DEQUALS 9
#define BINOP_NEQUALS 10
#define BINOP_LT 11
#define BINOP_GT 12
#define BINOP_LE 13
#define BINOP_GE 14

#define BINOP_LS_INT 15
#define BINOP_RS_INT 16

#define BINOP_TIMES_INT 17
#define BINOP_DIVIDE_INT 18
#define BINOP_MOD_INT 19
#define BINOP_TIMES_FLOAT 20
#define BINOP_DIVIDE_FLOAT 21
#define BINOP_MOD_FLOAT 22

#define UNOP_MINUS_INT 23
#define UNOP_MINUS_FLOAT 24

#define BINOP_PLUS_INT 25
#define BINOP_MINUS_INT 26
#define BINOP_PLUS_FLOAT 27
#define BINOP_MINUS_FLOAT 28

// usage: perform the given kind of unary operation on the subnode
class UnOpTree : public OpTree {
	public:
		// data members
		DataTree *subNode; // pointer to the data subnode that we're applying the operator to
		// allocators/deallocators
		UnOpTree(int kind, DataTree *subNode);
		~UnOpTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: perform the given kind of binary operation on subNodeLeft and subNodeRight
class BinOpTree : public OpTree {
	public:
		// data members
		DataTree *subNodeLeft; // pointer to the data subnode representing the left operand
		DataTree *subNodeRight; // pointer to the data subnode representing the right operand
		// allocators/deallocators
		BinOpTree(int kind, DataTree *subNodeLeft, DataTree *subNodeRight);
		~BinOpTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// definitions of conversion operator kinds
#define CONVOP_INT2FLOAT 29
#define CONVOP_FLOAT2INT 30

#define CONVOP_BOOL2STRING 29
#define CONVOP_INT2STRING 30
#define CONVOP_FLOAT2STRING 31
#define CONVOP_CHAR2STRING 32

// usage: perform the given kind of data representation conversion operation on the specified subNode
class ConvOpTree : public OpTree {
	public:
		// data members
		DataTree *subNode; // pointer to the data subnode representing the source of the conversion
		// allocators/deallocators
		ConvOpTree(int kind, DataTree *subNode);
		~ConvOpTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// CodeTree classes

// usage: abstract class; never used directly
class CodeTree : public IRTree {
	public:
		// allocators/deallocators
		CodeTree(int category);
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

// usage: use the value of a test word to index into the given jumpTable and jump to the code contained there; the test word is guaranteed to be a valid index into the jumpTable
class JumpTree : public CodeTree {
	public:
		// data members
		DataTree *test; // pointer to the data subnode specifying the value of the test condition
		vector<SeqTree *> jumpTable; // vector of pointers to the sequential code trees to run for each index value
		// allocators/deallocators
		JumpTree(DataTree *test, const vector<SeqTree *> &jumpTable);
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
		WriteTree(DataTree *source, DataTree *address);
		~WriteTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: copy a length of memory bytes from the specified sourceAddress to the specified destinationAddress
class CopyTree : public CodeTree {
	public:
		// data members
		DataTree *sourceAddress; // pointer to the data subnode specifying the base address to start copying from
		DataTree *destinationAddress; // pointer to the data subnode specifying the destination address to copy into
		uint32_t length;
		// allocators/deallocators
		CopyTree(DataTree *sourceAddress, DataTree *destinationAddress, uint32_t length);
		~CopyTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// usage: schedule all of the label nodes in the labelList for execution
class SchedTree : public CodeTree {
	public:
		// data members
		vector<LabelTree *> labelList; // vector of the label nodes to schedule
		// allocators/deallocators
		SchedTree(const vector<LabelTree *> &labelList);
		~SchedTree();
		// core methods
		string toString(unsigned int tabDepth) const;
};

// main code generation function

int gen(Tree *treeRoot, SymbolTable *stRoot, CodeTree *&codeRoot);

#endif
