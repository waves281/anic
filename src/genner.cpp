#include "genner.h"

#include "outputOperators.h"

// genner-global variables

int gennerErrorCode;

// CodeTree functions
CodeTree::~CodeTree() {}

// LabelCodeTree functions
LabelCodeTree::LabelCodeTree(const string &id, const vector<CodeTree *> &seqList) : id(id), seqList(seqList) {category = CATEGORY_LABEL;}
LabelCodeTree::~LabelCodeTree() {
	for (vector<CodeTree *>::const_iterator iter = seqList.begin(); iter != seqList.end(); iter++) {
		delete (*iter);
	}
}
string LabelCodeTree::toString(unsigned int tabDepth) const {
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

// DataCodeTree functions
DataCodeTree::~DataCodeTree() {}

// ConstCodeTree functions
ConstCodeTree::ConstCodeTree(const vector<uint32_t> &data) : data(data) {category = CATEGORY_CONST; offset = 0;}
ConstCodeTree::~ConstCodeTree() {}
DataCodeTree *ConstCodeTree::operator+(unsigned int offset) const {
	ConstCodeTree *retVal = new ConstCodeTree(*((ConstCodeTree *)this));
	retVal->offset += offset;
	return retVal;
}
DataCodeTree *ConstCodeTree::operator-(unsigned int offset) const {
	ConstCodeTree *retVal = new ConstCodeTree(*((ConstCodeTree *)this));
	retVal->offset -= offset;
	return retVal;
}
string ConstCodeTree::toString(unsigned int tabDepth) const {
	string acc("[");
	for (vector<uint32_t>::const_iterator iter = data.begin(); iter != data.end(); iter++) {
		acc += *iter;
		if (iter + 1 != data.end()) {
			acc += '|';
		}
	}
	acc += ']';
	return acc;
}

// TempCodeTree functions
TempCodeTree::TempCodeTree(unsigned int size) : size(size) {category = CATEGORY_TEMP; offset = 0;}
TempCodeTree::~TempCodeTree() {}
DataCodeTree *TempCodeTree::operator+(unsigned int offset) const {
	TempCodeTree *retVal = new TempCodeTree(*((TempCodeTree *)this));
	retVal->offset += offset;
	return retVal;
}
DataCodeTree *TempCodeTree::operator-(unsigned int offset) const {
	TempCodeTree *retVal = new TempCodeTree(*((TempCodeTree *)this));
	retVal->offset -= offset;
	return retVal;
}
string TempCodeTree::toString(unsigned int tabDepth) const {
	string acc("(");
	acc += size;
	acc += ')';
	return acc;
}

// UnOpCodeTree functions
UnOpCodeTree::UnOpCodeTree(int kind, DataCodeTree *subNode) : kind(kind), subNode(subNode) {category = CATEGORY_UNOP;}
UnOpCodeTree::~UnOpCodeTree() {delete subNode;}
string UnOpCodeTree::toString(unsigned int tabDepth) const {
	string acc(kindToString(kind));
	acc += '(';
	acc += subNode->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// BinOpCodeTree functions
BinOpCodeTree::BinOpCodeTree(int kind, DataCodeTree *subNodeLeft, DataCodeTree *subNodeRight) : kind(kind), subNodeLeft(subNodeLeft), subNodeRight(subNodeRight) {category = CATEGORY_BINOP;}
BinOpCodeTree::~BinOpCodeTree() {delete subNodeLeft; delete subNodeRight;}
string BinOpCodeTree::toString(unsigned int tabDepth) const {
	string acc(kindToString(kind));
	acc += '(';
	acc += subNodeLeft->toString(tabDepth+1);
	acc += ',';
	acc += subNodeRight->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// LockCodeTree functions
LockCodeTree::LockCodeTree(DataCodeTree *subNode) : subNode(subNode) {category = CATEGORY_LOCK;}
LockCodeTree::~LockCodeTree() {}
string LockCodeTree::toString(unsigned int tabDepth) const {
	string acc("!(");
	acc += subNode->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// UnlockCodeTree functions
UnlockCodeTree::UnlockCodeTree(DataCodeTree *subNode) : subNode(subNode) {category = CATEGORY_UNLOCK;}
UnlockCodeTree::~UnlockCodeTree() {}
string UnlockCodeTree::toString(unsigned int tabDepth) const {
	string acc("?(");
	acc += subNode->toString(tabDepth+1);
	acc += ')';
	return acc;
}

// CopyCodeTree functions
CopyCodeTree::CopyCodeTree(DataCodeTree *source) : source(source) {}
CopyCodeTree::~CopyCodeTree() {}
string CopyCodeTree::toString(unsigned int tabDepth) const {
	string acc("*(");
	acc += source->toString(tabDepth+1);
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
