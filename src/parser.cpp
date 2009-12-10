#include "mainDefs.h"
#include "system.h"

#include "parser.h"
#include "../var/parserStruct.h"

// tree functions

// constructors
Tree::Tree() : nextInternal(NULL), backInternal(NULL), childInternal(NULL), parentInternal(NULL) {}
Tree::Tree(Tree *next, Tree *back, Tree *child, Tree *parent) : nextInternal(next), backInternal(back), childInternal(child), parentInternal(parent) {}

// destructor
Tree::~Tree() {
	delete nextInternal;
	delete childInternal;
}

// traversal operators
// binary traversers
Tree &Tree::operator+(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->nextInternal;
		} else {
			throw new string("out-of-bounds tree traversal in operator+");
		}
	}
	return *cur;
}
Tree &Tree::operator-(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->backInternal;
		} else {
			throw new string("out-of-bounds tree traversal in operator-");
		}
	}
	return *cur;
}
Tree &Tree::operator*(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->childInternal;
		} else {
			throw new string("out-of-bounds tree traversal in operator*");
		}
	}
	return *cur;
}
Tree &Tree::operator&(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->parentInternal;
		} else {
			throw new string("out-of-bounds tree traversal in operator&");
		}
	}
	return *cur;
}
// unary traversers
Tree &Tree::operator+() {
	return this->operator+(1);
}
Tree &Tree::operator-() {
	return this->operator-(1);
}
Tree &Tree::operator*() {
	return this->operator*(1);
}
Tree &Tree::operator&() {
	return this->operator&(1);
}

// main parsing functions

Tree *parse(vector<Token> *lexeme, char *fileName) {
	// local error code
	int parserErrorCode = 0;
	// initialize lexer structure
	// int ruleLength[NUM_RULES] and ParserNode parserNode[NUM_RULES][NUM_TOKENS] are hereby defined and usable
	PARSER_STRUCT;
	// allocate the root pointer
	Tree *root = new Tree();

	// iterate through the lexemes and do the actual parsing

	// initialize the state stack and push the initial state onto it
	stack<unsigned int> stateStack;
	stateStack.push(0);
	// iterate through the lexemes
	for (vector<Token>::iterator lexemeIter = lexeme->begin(); lexemeIter != lexeme->end(); lexemeIter++) {
		// get the current state off the top of the stack
		unsigned int curState = stateStack.top();
		stateStack.pop();
		// get the transition node for the current state
		ParserNode transition = parserNode[curState][lexemeIter->tokenType];
		// branch based on the type of action dictated by the transition
		if (transition.action == ACTION_SHIFT) {

		} else if (transition.action == ACTION_REDUCE) {

		} else if (transition.action == ACTION_ACCEPT) {

		} else if (transition.action == ACTION_GOTO) {

		} else if (transition.action == ACTION_ERROR) {

		}
		stateStack.push(curState);
	}

	// finally, return to the caller
	if (parserErrorCode) {
		// deallocate the output vector, since we're just going to return null
		delete root;
		return NULL;
	} else {
		return root;
	}
}
