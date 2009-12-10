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

	// finally, return to the caller
	if (parserErrorCode) {
		// deallocate the output vector, since we're just going to return null
		delete root;
		return NULL;
	} else {
		return root;
	}
}
