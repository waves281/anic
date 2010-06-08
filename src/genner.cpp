#include "genner.h"

#include "outputOperators.h"

// genner-global variables

int gennerErrorCode;

// CodeTree functions
CodeTree::~CodeTree() {}

// LabelCodeTree functions
LabelCodeTree::LabelCodeTree(const string &id) : id(id) {category = CATEGORY_LABEL;}
LabelCodeTree::~LabelCodeTree() {}
LabelCodeTree::operator string() const {
	return "LabelCodeTree";
}

// DataCodeTree functions
DataCodeTree::~DataCodeTree() {}

// ConstCodeTree functions
ConstCodeTree::ConstCodeTree(const vector<unsigned int> &data) : data(data) {category = CATEGORY_CONST; offset = 0;}
ConstCodeTree::~ConstCodeTree() {}
ConstCodeTree::operator string() const {
	return "ConstCodeTree";
}

// TempCodeTree functions
TempCodeTree::TempCodeTree(unsigned int size) : size(size) {category = CATEGORY_TEMP; offset = 0;}
TempCodeTree::~TempCodeTree() {}
TempCodeTree::operator string() const {
	return "TempCodeTree";
}

// UnOpCodeTree functions
UnOpCodeTree::UnOpCodeTree(int kind, DataCodeTree *subNode) : kind(kind), subNode(subNode) {category = CATEGORY_UNOP;}
UnOpCodeTree::~UnOpCodeTree() {delete subNode;}
UnOpCodeTree::operator string() const {
	return "UnOpCodeTree";
}

// BinOpCodeTree functions
BinOpCodeTree::BinOpCodeTree(int kind, DataCodeTree *subNodeLeft, DataCodeTree *subNodeRight) : kind(kind), subNodeLeft(subNodeLeft), subNodeRight(subNodeRight) {category = CATEGORY_BINOP;}
BinOpCodeTree::~BinOpCodeTree() {delete subNodeLeft; delete subNodeRight;}
BinOpCodeTree::operator string() const {
	return "BinOpCodeTree";
}

// LockCodeTree functions
LockCodeTree::LockCodeTree(DataCodeTree *subNode) : subNode(subNode) {category = CATEGORY_LOCK;}
LockCodeTree::~LockCodeTree() {}
LockCodeTree::operator string() const {
	return "LockCodeTree";
}

// UnlockCodeTree functions
UnlockCodeTree::UnlockCodeTree(DataCodeTree *subNode) : subNode(subNode) {category = CATEGORY_UNLOCK;}
UnlockCodeTree::~UnlockCodeTree() {}
UnlockCodeTree::operator string() const {
	return "UnlockCodeTree";
}

// CopyCodeTree functions
CopyCodeTree::CopyCodeTree(DataCodeTree *source) : source(source) {}
CopyCodeTree::~CopyCodeTree() {}
CopyCodeTree::operator string() const {
	return "CopyCodeTree";
}

// SeqCodeTree functions
SeqCodeTree::SeqCodeTree() {}
SeqCodeTree::~SeqCodeTree() {}
SeqCodeTree::operator string() const {
	return "SeqCodeTree";
}

// main genning function; makes no assumptions about codeRoot's value; it's just a return parameter
int gen(Tree *treeRoot, SymbolTable *stRoot, CodeTree *&codeRoot) {

	// initialize error code
	gennerErrorCode = 0;
	
	codeRoot = NULL; // LOL

	// finally, return to the caller
	return gennerErrorCode ? 1 : 0;
}
