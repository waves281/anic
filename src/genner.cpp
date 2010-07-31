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
void LabelTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void SeqTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
}

// DataTree functions
DataTree::DataTree(int category) : IRTree(category) {}
DataTree::~DataTree() {}

// WordTree functions
WordTree::WordTree(uint32_t data) : DataTree(CATEGORY_WORD), data(data) {}
WordTree::~WordTree() {}
string WordTree::toString(unsigned int tabDepth) const {
	string acc("W[");
	char tempS[3];
	sprintf(tempS, "%08X", data);
	acc += tempS;
	acc += ']';
	return acc;
}
void WordTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
}

// ArrayTree functions
ArrayTree::ArrayTree(const vector<unsigned char> &data) : DataTree(CATEGORY_ARRAY), data(data) {}
ArrayTree::~ArrayTree() {}
string ArrayTree::toString(unsigned int tabDepth) const {
	string acc("A[");
	for (vector<unsigned char>::const_iterator iter = data.begin(); iter != data.end(); iter++) {
		char tempS[3];
		sprintf(tempS, "%02X", (*iter));
		acc += tempS;
	}
	acc += ']';
	return acc;
}
void ArrayTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void TempTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void ReadTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
}

// OpTree functions
OpTree::OpTree(int category, int kind) : IRTree(category), kind(kind) {}
OpTree::~OpTree() {}
string OpTree::kindToString() const {
	switch(kind) {
		case UNOP_NOT_BOOL:
			return "!";
		case UNOP_COMPLEMENT_INT:
			return "~";
		case UNOP_DPLUS_INT:
			return "++";
		case UNOP_DMINUS_INT:
			return "--";
		case BINOP_DOR_BOOL:
			return "||";
		case BINOP_DAND_BOOL:
			return "&&";
		case BINOP_OR_INT:
			return "|";
		case BINOP_XOR_INT:
			return "^";
		case BINOP_AND_INT:
			return "&";
		case BINOP_DEQUALS:
			return "==";
		case BINOP_NEQUALS:
			return "!=";
		case BINOP_LT:
			return "<";
		case BINOP_GT:
			return ">";
		case BINOP_LE:
			return "<=";
		case BINOP_GE:
			return ">=";
		case BINOP_LS_INT:
			return "<<";
		case BINOP_RS_INT:
			return ">>";
		case BINOP_TIMES_INT:
		case BINOP_TIMES_FLOAT:
			return "*";
		case BINOP_DIVIDE_INT:
		case BINOP_DIVIDE_FLOAT:
			return "/";
		case BINOP_MOD_INT:
		case BINOP_MOD_FLOAT:
			return "%";
		case BINOP_PLUS_INT:
		case BINOP_PLUS_FLOAT:
			return "+";
		case UNOP_MINUS_INT:
		case UNOP_MINUS_FLOAT:
		case BINOP_MINUS_INT:
		case BINOP_MINUS_FLOAT:
			return "-";
		case CONVOP_INT2FLOAT:
			return "int2Float";
		case CONVOP_FLOAT2INT:
			return "float2Int";
		case CONVOP_BOOL2STRING:
			return "bool2String";
		case CONVOP_INT2STRING:
			return "int2String";
		case CONVOP_FLOAT2STRING:
			return "float2String";
		case CONVOP_CHAR2STRING:
			return "char2String";
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
void UnOpTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void BinOpTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void ConvOpTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void LockTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void UnlockTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void CondTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void JumpTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void WriteTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void CopyTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
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
void SchedTree::asmDump(string &asmString) const {
	asmString += ""; // LOL
}

// main code generation function; asmDump is the assembler text generated
int gen(IRTree *codeRoot, string &asmString) {

	// initialize error code
	gennerErrorCode = 0;
	
	// recursively generate the assembly code for the program
	codeRoot->asmDump(asmString);

	// finally, return to the caller
	return gennerErrorCode ? 1 : 0;
}
