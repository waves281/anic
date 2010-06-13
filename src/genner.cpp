#include "genner.h"

#include "outputOperators.h"

// genner-global variables

int gennerErrorCode;

// IRTree functions
IRTree::IRTree(int category) : category(category) {}
IRTree::~IRTree() {}

// LabelTree functions
LabelTree::LabelTree(const string &id, SeqTree *code) : IRTree(CATEGORY_LABEL), id(id), code(code) {}
LabelTree::~LabelTree() {
	delete code;
}
string LabelTree::toString(unsigned int tabDepth) const {
	string acc(id);
	acc += '(';
	acc += code->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// SeqTree functions
SeqTree::SeqTree(const vector<CodeTree *> &codeList) : IRTree(CATEGORY_SEQ), codeList(codeList) {}
SeqTree::~SeqTree() {
	for (vector<CodeTree *>::const_iterator iter = codeList.begin(); iter != codeList.end(); iter++) {
		delete (*iter);
	}
}
string SeqTree::toString(unsigned int tabDepth) const {
	string acc;
	for (vector<CodeTree *>::const_iterator iter = codeList.begin(); iter != codeList.end(); iter++) {
		acc += (*iter)->toString(tabDepth+1);
		if (iter + 1 != codeList.end()) {
			acc += ',';
		}
	}
	return acc;
}

// DataTree functions
DataTree::DataTree(int category) : IRTree(category) {}
DataTree::~DataTree() {}

// ConstTree functions
ConstTree::ConstTree(const vector<unsigned char> &data) : DataTree(CATEGORY_CONST), data(data) {}
ConstTree::ConstTree(uint32_t dataInit) : DataTree(CATEGORY_CONST) {
	data.push_back((unsigned char)((dataInit >> 3*8) & 0xFF));
	data.push_back((unsigned char)((dataInit >> 2*8) & 0xFF));
	data.push_back((unsigned char)((dataInit >> 1*8) & 0xFF));
	data.push_back((unsigned char)((dataInit >> 0*8) & 0xFF));
}
ConstTree::~ConstTree() {}
string ConstTree::toString(unsigned int tabDepth) const {
	string acc("[");
	for (vector<unsigned char>::const_iterator iter = data.begin(); iter != data.end(); iter++) {
		char tempS[3];
		sprintf(tempS, "%02x", (*iter));
		acc += tempS;
	}
	acc += ']';
	return acc;
}

// TempTree functions
TempTree::TempTree(OpTree *opNode) : DataTree(CATEGORY_TEMP), opNode(opNode) {}
TempTree::~TempTree() {delete opNode;}
string TempTree::toString(unsigned int tabDepth) const {
	string acc("(");
	acc += opNode->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// ReadTree functions
ReadTree::ReadTree(DataTree *address) : DataTree(CATEGORY_READ), address(address) {}
ReadTree::~ReadTree() {delete address;}
string ReadTree::toString(unsigned int tabDepth) const {
	string acc("R(");
	acc += address->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// OpTree functions
OpTree::OpTree(int category, int kind) : IRTree(category), kind(kind) {}
OpTree::~OpTree() {}
string OpTree::kindToString() const {
	switch(kind) { // KOL
		case UNOP_NOT_BOOL:
			return "!";
			break;
		case UNOP_COMPLEMENT_INT:
			return "~";
			break;
		case UNOP_DPLUS_INT:
			return "++";
			break;
		case UNOP_DMINUS_INT:
			return "--";
			break;
		case BINOP_DOR_BOOL:
			return "||";
			break;
		case BINOP_DAND_BOOL:
			return "&&";
			break;
		case BINOP_OR_INT:
			return "|";
			break;
		case BINOP_XOR_INT:
			return "^";
			break;
		case BINOP_AND_INT:
			return "&";
			break;
		case BINOP_DEQUALS:
			return "==";
			break;
		case BINOP_NEQUALS:
			return "!=";
			break;
		case BINOP_LT:
			return "<";
			break;
		case BINOP_GT:
			return ">";
			break;
		case BINOP_LE:
			return "<=";
			break;
		case BINOP_GE:
			return ">=";
			break;
		case BINOP_LS_INT:
			return "<<";
			break;
		case BINOP_RS_INT:
			return ">>";
			break;
		case BINOP_TIMES_INT:
		case BINOP_TIMES_FLOAT:
			return "*";
			break;
		case BINOP_DIVIDE_INT:
		case BINOP_DIVIDE_FLOAT:
			return "/";
			break;
		case BINOP_MOD_INT:
		case BINOP_MOD_FLOAT:
			return "%";
			break;
		case BINOP_PLUS_INT:
		case BINOP_PLUS_FLOAT:
			return "+";
			break;
		case UNOP_MINUS_INT:
		case UNOP_MINUS_FLOAT:
		case BINOP_MINUS_INT:
		case BINOP_MINUS_FLOAT:
			return "-";
			break;
		case CONVOP_INT2FLOAT:
			return "int2Float";
			break;
		case CONVOP_FLOAT2INT:
			return "float2Int";
			break;
		// can't happen; the above should cover all cases
		default:
			return "";
	}
}

// UnOpTree functions
UnOpTree::UnOpTree(int kind, DataTree *subNode) : OpTree(CATEGORY_UNOP, kind), subNode(subNode) {}
UnOpTree::~UnOpTree() {delete subNode;}
string UnOpTree::toString(unsigned int tabDepth) const {
	string acc(kindToString());
	acc += '(';
	acc += subNode->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// BinOpTree functions
BinOpTree::BinOpTree(int kind, DataTree *subNodeLeft, DataTree *subNodeRight) : OpTree(CATEGORY_BINOP, kind), subNodeLeft(subNodeLeft), subNodeRight(subNodeRight) {}
BinOpTree::~BinOpTree() {delete subNodeLeft; delete subNodeRight;}
string BinOpTree::toString(unsigned int tabDepth) const {
	string acc(kindToString());
	acc += '(';
	acc += subNodeLeft->toString(tabDepth+1);
	acc += ',';
	acc += subNodeRight->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// ConvOpTree functions
ConvOpTree::ConvOpTree(int kind, DataTree *subNode) : OpTree(CATEGORY_CONVOP, kind), subNode(subNode) {}
ConvOpTree::~ConvOpTree() {delete subNode;}
string ConvOpTree::toString(unsigned int tabDepth) const {
	string acc(kindToString());
	acc += '(';
	acc += subNode->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// CodeTree functions
CodeTree::CodeTree(int category) : IRTree(category) {}
CodeTree::~CodeTree() {}

// LockTree functions
LockTree::LockTree(DataTree *address) : CodeTree(CATEGORY_LOCK), address(address) {}
LockTree::~LockTree() {delete address;}
string LockTree::toString(unsigned int tabDepth) const {
	string acc("L(");
	acc += address->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// UnlockTree functions
UnlockTree::UnlockTree(DataTree *address) : CodeTree(CATEGORY_UNLOCK), address(address) {}
UnlockTree::~UnlockTree() {delete address;}
string UnlockTree::toString(unsigned int tabDepth) const {
	string acc("U(");
	acc += address->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// CondTree functions
CondTree::CondTree(DataTree *test, CodeTree *trueBranch, CodeTree *falseBranch) : CodeTree(CATEGORY_COND), test(test), trueBranch(trueBranch), falseBranch(falseBranch) {}
CondTree::~CondTree() {delete test; delete trueBranch; delete falseBranch;}
string CondTree::toString(unsigned int tabDepth) const {
	string acc("?(");
	acc += test->toString(tabDepth+1);
	acc += ',';
	acc += trueBranch->toString(tabDepth+1);
	acc += ',';
	acc += falseBranch->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// JumpTree functions
JumpTree::JumpTree(DataTree *test, const vector<SeqTree *> &jumpTable) : CodeTree(CATEGORY_JUMP), test(test), jumpTable(jumpTable) {}
JumpTree::~JumpTree() {
	for (vector<SeqTree *>::const_iterator iter = jumpTable.begin(); iter != jumpTable.end(); iter++) {
		delete (*iter);
	}
}
string JumpTree::toString(unsigned int tabDepth) const {
	string acc("J(");
	for (unsigned int i = 0; i < jumpTable.size(); i++) {
		acc += i;
		acc += ':';
		acc += (jumpTable[i])->toString(tabDepth+1);
		if (i != jumpTable.size() - 1) {
			acc += ',';
		}
	}
	acc += ')';
	return acc;
}

// WriteTree functions
WriteTree::WriteTree(DataTree *source, DataTree *address) : CodeTree(CATEGORY_WRITE), source(source), address(address) {}
WriteTree::~WriteTree() {delete source; delete address;}
string WriteTree::toString(unsigned int tabDepth) const {
	string acc("W(");
	acc += source->toString(tabDepth+1);
	acc += ',';
	acc += address->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// CopyTree functions
CopyTree::CopyTree(DataTree *sourceAddress, DataTree *destinationAddress, uint32_t length) : CodeTree(CATEGORY_COPY), sourceAddress(sourceAddress), destinationAddress(destinationAddress), length(length) {}
CopyTree::~CopyTree() {delete sourceAddress; delete destinationAddress;}
string CopyTree::toString(unsigned int tabDepth) const {
	string acc("C(");
	acc += sourceAddress->toString(tabDepth+1);
	acc += ',';
	acc += destinationAddress->toString(tabDepth+1);
	acc += ',';
	acc += length;
	acc += ')';
	return acc;
}

// SchedTree functions
SchedTree::SchedTree(const vector<LabelTree *> &labelList) : CodeTree(CATEGORY_SCHED), labelList(labelList) {}
SchedTree::~SchedTree() {}
string SchedTree::toString(unsigned int tabDepth) const {
	string acc("#(");
	for (vector<LabelTree *>::const_iterator iter = labelList.begin(); iter != labelList.end(); iter++) {
		acc += (*iter)->toString(tabDepth+1);
		if (iter + 1 != labelList.end()) {
			acc += ',';
		}
	}
	acc += ')';
	return acc;
}

// main genning function; makes no assumptions about codeRoot's value; it's just a return parameter
int gen(Tree *treeRoot, SymbolTable *stRoot, CodeTree *&codeRoot) {

	// initialize error code
	gennerErrorCode = 0;
	
	codeRoot = NULL; // LOL

	// finally, return to the caller
	return gennerErrorCode ? 1 : 0;
}
