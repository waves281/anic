#include "parser.h"

// parser-global variables

int parserErrorCode;
bool parserEventuallyGiveUp;

// tree functions

// constructors
Tree::Tree() : next(NULL), back(NULL), child(NULL), parent(NULL), status(TypeStatus()), handled(false) {}
Tree::Tree(Token &t) : t(t), next(NULL), back(NULL), child(NULL), parent(NULL), status(TypeStatus()), handled(false) {}
Tree::Tree(Token &t, Tree *next, Tree *back, Tree *child, Tree *parent) : t(t), next(next), back(back), child(child), parent(parent), status(TypeStatus()), handled(false) {}

// destructor
Tree::~Tree() {
	delete next;
	delete child;
}

// comparison oparators

bool Tree::operator==(int tokenType) {
	return (t.tokenType == tokenType);
}

bool Tree::operator!=(int tokenType) {
	return (!operator==(tokenType));
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
void Tree::operator+=(Tree *next) {
	this->next = next;
}
void Tree::operator-=(Tree *back) {
	this->back = back;
}
void Tree::operator*=(Tree *child) {
	this->child = child;
}
void Tree::operator&=(Tree *parent) {
	this->parent = parent;
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

// exported tree parsing functions

// SuffixedIdentifier -> string parser function
string sid2String(Tree *sid) {
	string retVal;
	Tree *cur = sid; // SuffixedIdentifier
	for(;;) { // interloop invariant: cur is a SuffixedIdentifier
		// log this part of the name
		retVal += cur->child->t.s; // ID or DPERIOD
		// advance
		cur = cur->child->next; // NULL or PERIOD
		if (cur != NULL) { // if we're not at the end yet (this is a PERIOD)
			retVal += '.';
			cur = cur->next; // ArrayAccess or SuffixedIdentifier
			if (cur->t.tokenType == TOKEN_ArrayAccess) { // skip over the ArrayAccess as necessary
				retVal += "[]";
				cur = cur->next; // NULL or PERIOD
				if (cur != NULL) { // if we're not at the end yet (this is a PERIOD)
					cur = cur->next; // SuffixedIdentifier
				} else { // else if we're at the end
					break;
				}
			}
		} else { // else if we're at the end
			break;
		}
	}
	return retVal;
}

string idHead(string &id) {
	string retVal;
	for (unsigned int i=0; i<id.size(); i++) {
		if (id[i] == '.') {
			break;
		} else {
			retVal += id[i];
		}
	}
	return retVal;
}

string idTail(string &id) {
	for (unsigned int i=0; i<id.size(); i++) {
		if (id[i] == '.') {
			if (id.length() > i) {
				return id.substr(i+1);
			}
		}
	}
	return "";
}

string idEnd(string &id) {
	string retVal;
	for (unsigned int i=0; i<id.size(); i++) {
		if (id[i] == '.') {
			retVal.clear();
		} else {
			retVal += id[i];
		}
	}
	return retVal;
}

// main parsing functions

void shiftToken(Tree *&treeCur, Token &t) {
	Tree *treeToAdd = new Tree(t, NULL, treeCur, NULL, NULL);
	// link right from the current node
	if (treeCur != NULL) {
		*treeCur += treeToAdd;
	}
	// update treeCur to point to the newly shifted node
	treeCur = treeToAdd;
}

void promoteToken(Tree *&treeCur, Token &t) {
	Tree *treeToAdd = new Tree(t, NULL, (treeCur != NULL) ? treeCur->back : NULL, treeCur, (treeCur != NULL) ? treeCur->parent : NULL);
	// relatch on the left
	if (treeCur != NULL && treeCur->back != NULL) {
		*(treeCur->back) += treeToAdd;
		*treeCur -= NULL;
	}
	// link down from parent above
	if (treeCur != NULL && treeCur->parent != NULL) {
		*(treeCur->parent) *= treeToAdd;
	}
	// link up from the current node below
	if (treeCur != NULL) {
		*treeCur &= treeToAdd;
	}
	// set the newly promoted node as the current one
	treeCur = treeToAdd;
}

// treeCur is guaranteed not to be NULL in this case
void shiftPromoteNullToken(Tree *&treeCur, Token &t) {
	Tree *treeToAdd = new Tree(t, NULL, treeCur, NULL, NULL);
	// link in the newly allocated node
	*treeCur += treeToAdd;
	// set treeCur to the newly allocated node
	treeCur = treeToAdd;
}

int parse(vector<Token> *lexeme, vector<Tree *> *parseme, string &fileName) {

	// initialize error code
	parserErrorCode = 0;
	parserEventuallyGiveUp = eventuallyGiveUp;

	// initialize parser structures
#include "../tmp/ruleRhsLengthRaw.h"
#include "../tmp/ruleLhsTokenTypeRaw.h"
#include "../tmp/ruleLhsTokenStringRaw.h"
#include "../tmp/parserNodeRaw.h"

	// iterate through the lexemes and do the actual parsing
	// initialize the current bit of tree that we're examining
	Tree *treeCur = NULL;
	// initialize the state stack and push the initial state onto it
	stack<unsigned int> stateStack;
	stateStack.push(0);

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
			shiftToken(treeCur, t);
			stateStack.push(transition.n);

			// log the token in the parseme
			parseme[t.tokenType].push_back(treeCur);

			VERBOSE( cout << "\tSHIFT\t" << curState << "\t->\t" << transition.n << "\t[" << tokenType2String(t.tokenType) << "]\n"; )

		} else if (transition.action == ACTION_REDUCE) {
			unsigned int numRhs = ruleRhsLength[transition.n];
			int tokenType = ruleLhsTokenType[transition.n];
			if (numRhs > 1) {
				treeCur = treeCur->goBack(numRhs-1);
			}
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
			if (numRhs != 0 || treeCur == NULL) { // if it's not the NULL-shifting promotion case
				promoteToken(treeCur, t);
			} else { // else if it is the NULL-shifting promotion case
				shiftPromoteNullToken(treeCur, t); // note: the above case handles treeCur == NULL
			}
			// take the goto branch of the new transition
			int tempState = stateStack.top();
			stateStack.push(parserNode[tempState][tokenType].n);

			// log the nonterminal in the parseme
			parseme[tokenType].push_back(treeCur);

			VERBOSE(
				const char *tokenString = ruleLhsTokenString[transition.n];
				cout << "\tREDUCE\t" << curState << "\t->\t" << stateStack.top() << "\t<" << tokenString << ">\n";
			)

			goto transitionParserState;
		} else if (transition.action == ACTION_ACCEPT) {

			VERBOSE( cout << "\tACCEPT\n"; )

			break;
		} else if (transition.action == ACTION_ERROR) {
			string errorString = "syntax error at ";
			if (t.tokenType == TOKEN_CQUOTE || t.tokenType == TOKEN_SQUOTE) {
				errorString += "quoted literal";
			} else {
				errorString += "\'" + t.s + "\'";
			}
			parserError(fileName, t.row, t.col, errorString);
			break;
		}
	}

	// clean up if there was an error
	if (parserErrorCode) {
		// deallocate the unfinished tree, since there was an error anyway
		delete treeCur;
	}
	// finally, return to the caller
	return parserErrorCode;
}
