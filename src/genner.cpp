#include "genner.h"

#include "outputOperators.h"

// genner-global variables

int gennerErrorCode;

// IRTree functions
IRTree::~IRTree() {}

// LabelTree functions
LabelTree::LabelTree(const string &id, const vector<CodeTree *> &seqList) : id(id), seqList(seqList) {category = CATEGORY_LABEL;}
LabelTree::~LabelTree() {
	for (vector<CodeTree *>::const_iterator iter = seqList.begin(); iter != seqList.end(); iter++) {
		delete (*iter);
	}
}
string LabelTree::toString(unsigned int tabDepth) const {
	string acc(id);
	acc += '(';
	for (vector<CodeTree *>::const_iterator iter = seqList.begin(); iter != seqList.end(); iter++) {
		acc += (*iter)->toString(tabDepth+1);
		if (iter + 1 != seqList.end()) {
			acc += ',';
		}
	}
	acc += ')';
	return acc;
}

// DataTree functions
DataTree::~DataTree() {}

// ConstTree functions
ConstTree::ConstTree(const vector<unsigned char> &data) : data(data) {category = CATEGORY_CONST;}
ConstTree::ConstTree(uint32_t dataInit) {
	category = CATEGORY_CONST;
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
TempTree::TempTree(OpTree *opNode) : opNode(opNode) {category = CATEGORY_TEMP;}
TempTree::~TempTree() {delete opNode;}
string TempTree::toString(unsigned int tabDepth) const {
	string acc("(");
	acc += opNode->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// ReadTree functions
ReadTree::ReadTree(DataTree *address) : address(address) {category = CATEGORY_READ;}
ReadTree::~ReadTree() {delete address;}
string ReadTree::toString(unsigned int tabDepth) const {
	string acc("R(");
	acc += address->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// OpTree functions
OpTree::~OpTree() {}

// UnOpTree functions
UnOpTree::UnOpTree(int kind, DataTree *subNode) : kind(kind), subNode(subNode) {category = CATEGORY_UNOP;}
UnOpTree::~UnOpTree() {delete subNode;}
string UnOpTree::toString(unsigned int tabDepth) const {
	string acc(kindToString(kind));
	acc += '(';
	acc += subNode->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// BinOpTree functions
BinOpTree::BinOpTree(int kind, DataTree *subNodeLeft, DataTree *subNodeRight) : kind(kind), subNodeLeft(subNodeLeft), subNodeRight(subNodeRight) {category = CATEGORY_BINOP;}
BinOpTree::~BinOpTree() {delete subNodeLeft; delete subNodeRight;}
string BinOpTree::toString(unsigned int tabDepth) const {
	string acc(kindToString(kind));
	acc += '(';
	acc += subNodeLeft->toString(tabDepth+1);
	acc += ',';
	acc += subNodeRight->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// CodeTree functions
CodeTree::~CodeTree() {}

// LockTree functions
LockTree::LockTree(DataTree *address) : address(address) {category = CATEGORY_LOCK;}
LockTree::~LockTree() {delete address;}
string LockTree::toString(unsigned int tabDepth) const {
	string acc("L(");
	acc += address->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// UnlockTree functions
UnlockTree::UnlockTree(DataTree *address) : address(address) {category = CATEGORY_UNLOCK;}
UnlockTree::~UnlockTree() {delete address;}
string UnlockTree::toString(unsigned int tabDepth) const {
	string acc("U(");
	acc += address->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// CondTree functions
CondTree::CondTree(DataTree *test, CodeTree *trueBranch, CodeTree *falseBranch) : test(test), trueBranch(trueBranch), falseBranch(falseBranch) {category = CATEGORY_COND;}
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
JumpTree::JumpTree(DataTree *address) : address(address) {category = CATEGORY_JUMP;}
JumpTree::~JumpTree() {delete address;}
string JumpTree::toString(unsigned int tabDepth) const {
	string acc("J(");
	acc += address->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// WriteTree functions
WriteTree::WriteTree(DataTree *source, DataTree *address) : source(source), address(address) {category = CATEGORY_WRITE;}
WriteTree::~WriteTree() {delete source; delete address;}
string WriteTree::toString(unsigned int tabDepth) const {
	string acc("W(");
	acc += source->toString(tabDepth+1);
	acc += ',';
	acc += address->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// ScheduleTree functions
ScheduleTree::ScheduleTree(const vector<LabelTree *> &labelList) : labelList(labelList) {category = CATEGORY_SCHEDULE;}
ScheduleTree::~ScheduleTree() {}
string ScheduleTree::toString(unsigned int tabDepth) const {
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
