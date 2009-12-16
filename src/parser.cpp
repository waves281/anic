#include "mainDefs.h"
#include "system.h"

#include "parser.h"
#include "../var/parserStruct.h"

// tree functions

// constructors
Tree::Tree() : next(NULL), back(NULL), child(NULL), parent(NULL) {}
Tree::Tree(Token &t) : t(t), next(NULL), back(NULL), child(NULL), parent(NULL) {}
Tree::Tree(Token &t, Tree *next, Tree *back, Tree *child, Tree *parent) : t(t), next(next), back(back), child(child), parent(parent) {}

// destructor
Tree::~Tree() {
	delete next;
	delete child;
}

// traversal operators
// binary traversers
Tree *Tree::goNext(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->next;
		} else {
			return NULL;
		}
	}
	return cur;
}
Tree *Tree::goBack(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->back;
		} else {
			return NULL;
		}
	}
	return cur;
}
Tree *Tree::goChild(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->child;
		} else {
			return NULL;
		}
	}
	return cur;
}
Tree *Tree::goParent(unsigned int n) {
	Tree *cur = this;
	for (unsigned int i=0; i<n; i++) {
		if (cur != NULL) {
			cur = cur->parent;
		} else {
			return NULL;
		}
	}
	return cur;
}
// binary attatchers
Tree &Tree::operator+=(Tree *&next) {
	this->next = next;
	return *next;
}
Tree &Tree::operator-=(Tree *&back) {
	this->back = back;
	return *back;
}
Tree &Tree::operator*=(Tree *&child) {
	this->child = child;
	return *child;
}
Tree &Tree::operator&=(Tree *&parent) {
	this->parent = parent;
	return *parent;
}
void Tree::operator+=(int x) {
	next = (Tree *)x;
}
void Tree::operator-=(int x) {
	back = (Tree *)x;
}
void Tree::operator*=(int x) {
	child = (Tree *)x;
}
void Tree::operator&=(int x) {
	parent = (Tree *)x;
}
// generalized traverser
Tree *Tree::operator()(char *s) {
	Tree *cur = this;
	for(;;) {
		if (cur == NULL || s[0] == '\0') {
			 return cur;
		} else if (s[0] == '>') {
			cur = cur->next;
			s++;
		} else if (s[0] == '<') {
			cur = cur->back;
			s++;
		} else if (s[0] == 'v') {
			cur = cur->child;
			s++;
		} else if (s[0] == '^') {
			cur = cur->parent;
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

int promoteToken(Tree *&treeCur, Token &t, Tree *&root) {
	Tree *treeToAdd = new Tree(t, NULL, NULL, treeCur, treeCur != NULL ? treeCur->parent : NULL);
	if (treeCur != NULL) {
		if (treeCur->parent != NULL) { // if this not the root (the parent pointer is non-null)
			*(treeCur->parent) *= treeToAdd; // update treeCur's parent to point down to the new node
			*treeCur &= treeToAdd; // update treeCur to point up to the new node
		} else { // else if this is the root
			root = treeToAdd;
			*treeCur &= treeToAdd; // update treeCur to point up the new node
		}
	} else {
		if (root == NULL) {
			root = treeToAdd;
		}
	}
	// finally, set treeCur to the newly allocated node
	treeCur = treeToAdd;
	// then, return normally
	return 0;
}

Tree *parse(vector<Token> *lexeme, char *fileName, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp) {
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
			unsigned int numRhs = ruleRhsLength[transition.n];
			int tokenType = ruleLhsTokenType[transition.n];
			treeCur = treeCur->goBack(numRhs-1);
			for (unsigned int i=0; i<numRhs; i++) {
				stateStack.pop();
			}
			// create the token that the promoted node will have
			Token t;
			t.tokenType = tokenType;
			t.fileName = fileName;
			t.row = treeCur != NULL ? treeCur->t.row : 0;
			t.col = treeCur != NULL ? treeCur->t.col : 0;
			// promote the current token, as appropriate
			promoteToken(treeCur, t, root);
			// take the goto branch of the new transition
			int tempState = stateStack.top();
			stateStack.push(parserNode[tempState][tokenType].n);

			VERBOSE(
				char *tokenString = ruleLhsTokenString[transition.n];
				cout << "\tREDUCE " << curState << " -> " << stateStack.top() << "\t[ " << tokenString << " ]" << endl;
			)

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
