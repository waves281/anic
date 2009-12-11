#include "mainDefs.h"
#include "system.h"

#include "parser.h"
#include "../var/parserStruct.h"

// tree functions

// constructors
Tree::Tree() : nextInternal(NULL), backInternal(NULL), childInternal(NULL), parentInternal(NULL) {}
Tree::Tree(Token &t) : tInternal(t), nextInternal(NULL), backInternal(NULL), childInternal(NULL), parentInternal(NULL) {}
Tree::Tree(Token &t, Tree *next, Tree *back, Tree *child, Tree *parent) : tInternal(t), nextInternal(next), backInternal(back), childInternal(child), parentInternal(parent) {}

// destructor
Tree::~Tree() {
	delete nextInternal;
	delete childInternal;
}

// accessors
Token &Tree::t() {
	return tInternal;
}

// traversal operators
// binary traversers
Tree *Tree::operator+(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->nextInternal;
		} else {
			return NULL;
		}
	}
	return cur;
}
Tree *Tree::operator-(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->backInternal;
		} else {
			return NULL;
		}
	}
	return cur;
}
Tree *Tree::operator*(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->childInternal;
		} else {
			return NULL;
		}
	}
	return cur;
}
Tree *Tree::operator&(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->parentInternal;
		} else {
			return NULL;
		}
	}
	return cur;
}
// unary traversers
Tree *Tree::operator+() {
	return this->operator+(1);
}
Tree *Tree::operator-() {
	return this->operator-(1);
}
Tree *Tree::operator*() {
	return this->operator*(1);
}
Tree *Tree::operator&() {
	return this->operator&(1);
}
// binary attatchers
Tree &Tree::operator+=(Tree *&next) {
	nextInternal = next;
	return *next;
}
Tree &Tree::operator*=(Tree *&child) {
	childInternal = child;
	return *child;
}
Tree &Tree::operator&=(Tree *&parent) {
	parentInternal = parent;
	return *parent;
}
// generalized traverser
Tree *Tree::traverse(char *s) {
	Tree *cur = this;
	for(;;) {
		if (cur == NULL || s[0] == '\0') {
			 return cur;
		} else if (s[0] == '>') {
			cur = cur->nextInternal;
			s++;
		} else if (s[0] == '<') {
			cur = cur->backInternal;
			s++;
		} else if (s[0] == 'v') {
			cur = cur->childInternal;
			s++;
		} else if (s[0] == '^') {
			cur = cur->parentInternal;
			s++;
		} else {
			throw string("INTERNAL ERROR: illegal tree traverser");
		}
	}
}

// main parsing functions

int shiftToken(Tree *&treeCur, Token &t, Tree *&root) {
	Tree *treeToAdd = new Tree(t, NULL, treeCur, NULL, NULL);
	if (treeCur != NULL) { // if this is any subsequent token
		*treeCur += treeToAdd;
		treeCur = treeToAdd;
	} else { // else if this is the first token
		root = treeToAdd;
		treeCur = root;
	}
	// return normally
	return 0;
}

int promoteToken(Tree *&treeCur, int tokenType, Tree *&root) {
	Token t;
	t.tokenType = tokenType;
	t.row = treeCur->t().row;
	t.col = treeCur->t().col;
	Tree *treeToAdd = new Tree(t, NULL, NULL, treeCur, &(*treeCur));
	if (&(*treeCur) != NULL) { // if this not the root (the parent pointer is non-null)
		*(&(*treeCur)) *= treeToAdd; // update treeCur's parent to point down to the new node
		(*treeCur) &= treeToAdd; // update treeCur to point up to the new node
	} else { // else if this is the root
		root = treeToAdd;
		(*treeCur) &= treeToAdd; // update treeCur to point up the new node
	}
	// finally, set treeCur to the newly allocated node
	treeCur = treeToAdd;
	// then, return normally
	return 0;
}

Tree *parse(vector<Token> *lexeme, char *fileName, int verboseOutput, int optimizationLevel) {
	// local error code
	int parserErrorCode = 0;
	// initialize lexer structure
	// int ruleLength[NUM_RULES], ruleLhs[NUM_RULES], and ParserNode parserNode[NUM_RULES][NUM_TOKENS] are hereby defined and usable
	PARSER_STRUCT;

	// iterate through the lexemes and do the actual parsing

	// initialize the state stack and push the initial state onto it
	stack<unsigned int> stateStack;
	stateStack.push(0);
	// iterate through the lexemes

	Tree *root = NULL; // tree root pointer, starts off as the first token in the input
	Tree *treeCur = root; // the current bit of tree that we're examining

	for(vector<Token>::iterator lexemeIter = lexeme->begin(); lexemeIter != lexeme->end(); lexemeIter++) {

transitionParserState: ;

		// get the current state off the top of the stack
		unsigned int curState = stateStack.top();
		// peek at the next token of input
		Token &t = *lexemeIter;
		// get the transition node for the current state
		ParserNode transition = parserNode[curState][t.tokenType];

		// branch based on the type of action dictated by the transition
		if (transition.action == ACTION_SHIFT) {
			shiftToken(treeCur, t, root);
			stateStack.push(transition.n);

			VERBOSE( cout << "\tSHIFT " << curState << " -> " << transition.n << endl; )

		} else if (transition.action == ACTION_REDUCE) {
			unsigned int numRhs = ruleLength[transition.n];
			int ruleLhsTokenType = ruleLhs[transition.n];
			treeCur = (*treeCur) - (numRhs-1);
			for (unsigned int i=0; i<numRhs; i++) {
				stateStack.pop();
			}
			promoteToken(treeCur, ruleLhsTokenType, root);
			// take the goto branch of the new transition
			curState = stateStack.top();
			stateStack.push(parserNode[curState][ruleLhsTokenType].n);

			VERBOSE( cout << "\tREDUCE " << curState << " -> " << stateStack.top() << endl; )

			goto transitionParserState;
		} else if (transition.action == ACTION_ACCEPT) {

			VERBOSE( cout << "\tACCEPT " << endl; )

			break;
		} else if (transition.action == ACTION_ERROR) {
			string errorString = "syntax error at ";
			if (t.tokenType == TOKEN_CQUOTE || t.tokenType == TOKEN_SQUOTE) {
				errorString += "quoted literal";
			} else {
				errorString += "\'" + t.s + "\'";
			}
			printParserError(fileName, t.row, t.col, errorString);
			break;
		}
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
