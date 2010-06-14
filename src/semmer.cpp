#include "semmer.h"

#include "outputOperators.h"

// semmer-global variables

int semmerErrorCode;

Type *nullType;
Type *errType;
StdType *stdBoolType;
StdType *stdIntType;
StdType *stdFloatType;
StdType *stdCharType;
StdType *stdStringType;
StdType *stdNullLitType;
StdType *stdBoolLitType;
ObjectType *stringerType;
FilterType *boolUnOpType;
FilterType *intUnOpType;
FilterType *boolBinOpType;
FilterType *intBinOpType;
FilterType *floatBinOpType;
FilterType *boolCompOpType;
FilterType *intCompOpType;
FilterType *floatCompOpType;
FilterType *charCompOpType;
FilterType *stringCompOpType;

// SymbolTable functions

// allocators/deallocators
SymbolTable::SymbolTable(int kind, const string &id, Tree *defSite) : kind(kind), id(id), defSite(defSite), parent(NULL) {
	if (defSite != NULL) {
		defSite->env = this;
	}
}
SymbolTable::SymbolTable(int kind, const char *id, Tree *defSite) : kind(kind), id(id), defSite(defSite), parent(NULL) {
	if (defSite != NULL) {
		defSite->env = this;
	}
}
SymbolTable::SymbolTable(int kind, const string &id, Type *defType) : kind(kind), id(id), parent(NULL) {TypeStatus status(defType, NULL); defSite = new Tree(status); defSite->env = this;}
SymbolTable::SymbolTable(int kind, const char *id, Type *defType) : kind(kind), id(id), parent(NULL) {TypeStatus status(defType, NULL); defSite = new Tree(status); defSite->env = this;}
SymbolTable::SymbolTable(const SymbolTable &st) : kind(st.kind), id(st.id), defSite(st.defSite), parent(st.parent), children(st.children) {}
SymbolTable::~SymbolTable() {
	// delete all of the child nodes
	for (map<string, SymbolTable *>::const_iterator childIter = children.begin(); childIter != children.end(); childIter++) {
		delete (*childIter).second;
	}
}

// copy assignment operator
SymbolTable &SymbolTable::operator=(const SymbolTable &st) {
	kind = st.kind;
	defSite = st.defSite;
	if (id != st.id) { // if the id is changing
		if (parent != NULL) { // ... and there exists a parent, fix up the parent's children map to use the new id
			parent->children.erase(id);
			parent->children.insert(make_pair(st.id, this));
		}
		id = st.id; // either way, update the id
	}
	children = st.children;
	return *this;
}

// concatenators
SymbolTable &SymbolTable::operator*=(SymbolTable *st) {
	// first, check for conflicting bindings
	if (st->kind == KIND_STD || st->kind == KIND_DECLARATION || st->kind == KIND_PARAMETER) { // if this is a conflictable (non-special system-level binding)
		// per-symbol loop
		map<string, SymbolTable *>::const_iterator conflictFind = children.find(st->id);
		if (conflictFind != children.end()) { // if we've found a conflict
			SymbolTable *conflictSt = (*conflictFind).second;
			Token curDefToken;
			if (st->defSite != NULL) { // if there is a definition site for the current symbol
				curDefToken = st->defSite->t;
			} else { // otherwise, it must be a standard definition, so make up the token as if it was
				curDefToken.fileName = STANDARD_LIBRARY_STRING;
				curDefToken.row = 0;
				curDefToken.col = 0;
			}
			Token prevDefToken;
			if (conflictSt->defSite != NULL) { // if there is a definition site for the previous symbol
				prevDefToken = conflictSt->defSite->t;
			} else { // otherwise, it must be a standard definition, so make up the token as if it was
				prevDefToken.fileName = STANDARD_LIBRARY_STRING;
				prevDefToken.row = 0;
				prevDefToken.col = 0;
			}
			semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"redefinition of '"<<st->id<<"'");
			semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
			delete st;
			return *this;
		}
	}	// binding is now known to be conflict-free, so log it normally
	children.insert(make_pair(st->id, st));
	if (st != NULL) {
		st->parent = this;
		return *st;
	} else {
		return *this;
	}
}

// Main semantic analysis functions

void catStdNodes(SymbolTable *&stRoot) {
	*stRoot *= new SymbolTable(KIND_STD, "int", stdIntType);
	*stRoot *= new SymbolTable(KIND_STD, "float", stdFloatType);
	*stRoot *= new SymbolTable(KIND_STD, "bool", stdBoolType);
	*stRoot *= new SymbolTable(KIND_STD, "char", stdCharType);
	*stRoot *= new SymbolTable(KIND_STD, "string", stdStringType);
	*stRoot *= new SymbolTable(KIND_STD, "null", stdNullLitType);
	*stRoot *= new SymbolTable(KIND_STD, "true", stdBoolLitType);
	*stRoot *= new SymbolTable(KIND_STD, "false", stdBoolLitType);
}

void catStdLib(SymbolTable *&stRoot) {
	// standard root
	StdType *stdLibType = new StdType(STD_STD, SUFFIX_LATCH); stdLibType->operable = false;
	SymbolTable *stdLib = new SymbolTable(KIND_STD, STANDARD_LIBRARY_STRING, stdLibType);
	// system nodes
	// streams
	*stdLib *= new SymbolTable(KIND_STD, "inInt", new StdType(STD_INT, SUFFIX_STREAM, 1));
	*stdLib *= new SymbolTable(KIND_STD, "inFloat", new StdType(STD_FLOAT, SUFFIX_STREAM, 1));
	*stdLib *= new SymbolTable(KIND_STD, "inChar", new StdType(STD_CHAR, SUFFIX_STREAM, 1));
	*stdLib *= new SymbolTable(KIND_STD, "inString", new StdType(STD_STRING, SUFFIX_STREAM, 1));
	*stdLib *= new SymbolTable(KIND_STD, "out", stringerType);
	*stdLib *= new SymbolTable(KIND_STD, "err", stringerType);
	// control nodes
	*stdLib *= new SymbolTable(KIND_STD, "randInt", new StdType(STD_INT, SUFFIX_STREAM, 1));
	*stdLib *= new SymbolTable(KIND_STD, "delay", new FilterType(new StdType(STD_INT), nullType, SUFFIX_LATCH));
	// standard library
	// generators
	*stdLib *= new SymbolTable(KIND_STD, "gen", new FilterType(new StdType(STD_INT), new StdType(STD_INT, SUFFIX_STREAM, 1), SUFFIX_LATCH));
	// concatenate the library to the root
	*stRoot *= stdLib;
}

void initStdTypes() {
	// build the standard types
	nullType = new StdType(STD_NULL); nullType->operable = false;
	errType = new ErrorType();
	stdBoolType = new StdType(STD_BOOL); stdBoolType->operable = false;
	stdIntType = new StdType(STD_INT); stdIntType->operable = false;
	stdFloatType = new StdType(STD_FLOAT); stdFloatType->operable = false;
	stdCharType = new StdType(STD_CHAR); stdCharType->operable = false;
	stdStringType = new StdType(STD_STRING); stdStringType->operable = false;
	stdNullLitType = new StdType(STD_NULL);
	stdBoolLitType = new StdType(STD_BOOL, SUFFIX_LATCH);
	// build the stringerType
	vector<TypeList *> instructorTypes;
	vector<TypeList *> outstructorTypes;
	outstructorTypes.push_back(new TypeList(stdStringType));
	stringerType = new ObjectType(instructorTypes, outstructorTypes, SUFFIX_STREAM, 1); stringerType->operable = false;
	// build some auxiliary types
	// latches
	Type *boolLatchType = new StdType(STD_BOOL, SUFFIX_LATCH);
	Type *intLatchType = new StdType(STD_INT, SUFFIX_LATCH);
	Type *floatLatchType = new StdType(STD_FLOAT, SUFFIX_LATCH);
	// pairs
	vector<Type *> boolPair;
	boolPair.push_back(stdBoolType); boolPair.push_back(stdBoolType);
	Type *boolPairType = new TypeList(boolPair);
	
	vector<Type *> intPair;
	intPair.push_back(stdIntType); intPair.push_back(stdIntType);
	Type *intPairType = new TypeList(intPair);
	
	vector<Type *> floatPair;
	floatPair.push_back(stdFloatType); floatPair.push_back(stdFloatType);
	Type *floatPairType = new TypeList(floatPair);
	
	vector<Type *> charPair;
	charPair.push_back(stdCharType); charPair.push_back(stdCharType);
	Type *charPairType = new TypeList(charPair);
	
	vector<Type *> stringPair;
	stringPair.push_back(stdStringType); stringPair.push_back(stdStringType);
	Type *stringPairType = new TypeList(stringPair);
	// build the boolUnOpType
	boolUnOpType = new FilterType(stdBoolType, boolLatchType, SUFFIX_LATCH); boolUnOpType->operable = false;
	// build the intUnOpType
	intUnOpType = new FilterType(stdIntType, intLatchType, SUFFIX_LATCH); intUnOpType->operable = false;
	// build the boolBinOpType
	boolBinOpType = new FilterType(boolPairType, boolLatchType, SUFFIX_LATCH); boolBinOpType->operable = false;
	// build the intBinOpType
	intBinOpType = new FilterType(intPairType, intLatchType, SUFFIX_LATCH); intBinOpType->operable = false;
	// build the floatBinOpType
	floatBinOpType = new FilterType(floatPairType, floatLatchType, SUFFIX_LATCH); floatBinOpType->operable = false;
	// build the boolCompOpType
	boolCompOpType = new FilterType(boolPairType, boolLatchType, SUFFIX_LATCH); boolCompOpType->operable = false;
	// build the intCompOpType
	intCompOpType = new FilterType(intPairType, boolLatchType, SUFFIX_LATCH); intCompOpType->operable = false;
	// build the floatCompOpType
	floatCompOpType = new FilterType(floatPairType, boolLatchType, SUFFIX_LATCH); floatCompOpType->operable = false;
	// build the charCompOpType
	charCompOpType = new FilterType(charPairType, boolLatchType, SUFFIX_LATCH); charCompOpType->operable = false;
	// build the stringCompOpType
	stringCompOpType = new FilterType(stringPairType, boolLatchType, SUFFIX_LATCH); stringCompOpType->operable = false;
}

SymbolTable *genDefaultDefs() {
	// generate the root block node
	SymbolTable *stRoot = new SymbolTable(KIND_BLOCK, BLOCK_NODE_STRING);
	// concatenate in the standard types
	catStdNodes(stRoot);
	// concatenate in the standard library
	catStdLib(stRoot);
	// finally, return the generated default symtable
	return stRoot;
}

// recursively extracts the appropriate nodes from the given tree and appropriately populates the passed containers
void buildSt(Tree *tree, SymbolTable *st, vector<SymbolTable *> &importList) {
	// base case
	if (tree == NULL) {
		return;
	}
	// log the current symbol environment in the tree (this pointer will potentially be overridden by a SymbolTable() constructor)
	tree->env = st;
	// recursive cases
	if (*tree == TOKEN_Block || *tree == TOKEN_Object) { // if it's a block-style node
		// allocate the new block definition node
		// generate an identifier for the node
		int kind;
		string fakeId;
		if (*tree == TOKEN_Block) { // if it's a block node, use a regular identifier
			kind = KIND_BLOCK;
			fakeId = BLOCK_NODE_STRING;
		} else { // else if it's an object node, generate a fake identifier from a hash of the Tree node
			kind = KIND_OBJECT;
			fakeId = OBJECT_NODE_STRING;
			fakeId += (unsigned int)tree;
		}
		SymbolTable *blockDef = new SymbolTable(kind, fakeId, tree);
		// latch the new node into the SymbolTable trunk
		*st *= blockDef;
		// recurse
		buildSt(tree->child, blockDef, importList); // child of Block or Object
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Filter) { // if it's a filter node
		// allocate the new filter definition node
		// generate a fake identifier for the filter node from a hash of the Tree node
		string fakeId(FILTER_NODE_STRING);
		fakeId += (unsigned int)tree;
		SymbolTable *filterDef = new SymbolTable(KIND_FILTER, fakeId, tree);
		// parse out the header's parameter declarations and add them to the st
		Tree *pl = (*(tree->child) == TOKEN_FilterHeader) ? tree->child->child->next : NULL; // RSQUARE, ParamList, RetList, or NULL
		if (pl != NULL && *pl == TOKEN_ParamList) { // if there is a parameter list to process
			for (Tree *param = pl->child; param != NULL; param = (param->next != NULL) ? param->next->next->child : NULL) { // per-param loop
				// allocate the new parameter definition node
				SymbolTable *paramDef = new SymbolTable(KIND_PARAMETER, param->child->next->t.s, param);
				// ... and link it into the filter definition node
				*filterDef *= paramDef;
			}
		} // if there is a parameter list to process
		// latch the new node into the SymbolTable trunk
		*st *= filterDef;
		// recurse
		buildSt(tree->child, filterDef, importList); // child of Filter
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Instructor || *tree == TOKEN_LastInstructor) { // if it's an Instructor-style node
		// allocate the new instructor definition node
		// generate a fake identifier for the instructor node from a hash of the Tree node
		string fakeId(INSTRUCTOR_NODE_STRING);
		fakeId += (unsigned int)tree;
		SymbolTable *consDef = new SymbolTable(KIND_INSTRUCTOR, fakeId, tree);
		// .. and link it in
		*st *= consDef;
		// link in the parameters of this instructor, if any
		Tree *conscn = tree->child->next; // NULL, SEMICOLON, LSQUARE, or NonRetFilterHeader
		if (conscn != NULL && *conscn == TOKEN_NonRetFilterHeader && *(conscn->child->next) == TOKEN_ParamList) { // if there is actually a parameter list on this instructor
			Tree *pl = conscn->child->next; // ParamList
			for (Tree *param = pl->child; param != NULL; param = (param->next != NULL) ? param->next->next->child : NULL) { // per-param loop
				// allocate the new parameter definition node
				SymbolTable *paramDef = new SymbolTable(KIND_PARAMETER, param->child->next->t.s, param);
				// ... and link it into the instructor definition node
				*consDef *= paramDef;
			}
		}
		// recurse
		buildSt(tree->child, consDef, importList); // child of Instructor
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Outstructor) { // if it's an Outstructor-style node
		// allocate the new outstructor definition node
		// generate a fake identifier for the outstructor node from a hash of the Tree node
		string fakeId(OUTSTRUCTOR_NODE_STRING);
		fakeId += (unsigned int)tree;
		SymbolTable *consDef = new SymbolTable(KIND_OUTSTRUCTOR, fakeId, tree);
		// .. and link it in
		*st *= consDef;
		// recurse
		buildSt(tree->child, consDef, importList); // child of Outstructor
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Declaration || *tree == TOKEN_LastDeclaration) { // if it's a Declaration-style node
		Token defToken = tree->child->t; // ID, AT, or DAT
		if (defToken.tokenType != TOKEN_ID || (defToken.s != "null" && defToken.s != "true" && defToken.s != "false")) { // if this isn't a standard literal override, proceed normally
			Tree *dcn = tree->child->next;
			if (*dcn == TOKEN_EQUALS) { // standard static declaration
				// allocate the new declaration node
				SymbolTable *newDef = new SymbolTable(KIND_DECLARATION, tree->child->t.s, tree);
				// ... and link it in
				*st *= newDef;
				// recurse
				buildSt(tree->child, newDef, importList); // child of Declaration
				buildSt(tree->next, st, importList); // right
			} else if (*dcn == TOKEN_ERARROW) { // flow-through declaration
				// allocate the new definition node
				SymbolTable *newDef = new SymbolTable(KIND_DECLARATION, tree->child->t.s, tree);
				// ... and link it in
				*st *= newDef;
				// recurse
				buildSt(tree->child, newDef, importList); // child of Declaration
				buildSt(tree->next, st, importList); // right
			} else if (*(tree->child) == TOKEN_AT || *(tree->child) == TOKEN_DAT) { // import-style declaration
				// allocate the new definition node
				SymbolTable *newDef = new SymbolTable((*(tree->child) == TOKEN_AT) ? KIND_CLOSED_IMPORT : KIND_OPEN_IMPORT, IMPORT_DECL_STRING, tree);
				// ... and link it in
				*st *= newDef;
				// also, since it's an import declaration, log it to the import list
				importList.push_back(newDef);
				// recurse
				buildSt(tree->child, newDef, importList); // child of Declaration
				buildSt(tree->next, st, importList); // right
			}
		} else { // else if this is a standard literal override, flag an error
			semmerError(defToken.fileName,defToken.row,defToken.col,"redefinition of standard literal '"<<defToken.s<<"'");
		}
	} else { // else if it's not a declaration node
		// recurse normally
		buildSt(tree->child, st, importList); // down
		buildSt(tree->next, st, importList); // right
	}
}

// chops up the passed in string into its period-delimited components
vector<string> chopId(const string &s) {
	vector<string> retVal;
	string acc;
	for (unsigned int i=0; i < s.size(); i++) {
		if (s[i] != '.') { // if this character isn't a period
			acc += s[i];
		} else if (s[i+1] == '.') { // else if this character is a period and the next one is as well (can only occur at the beginning of an identifier)
			i++;
			acc += "..";
		} else {
			retVal.push_back(acc);
			acc.clear();
		}
	}
	if (acc.size() > 0) { // commit the last token if necessary
		retVal.push_back(acc);
	}
	return retVal;
}

// rebuilds a string representation of this identifier from a chopped list and a depth
string rebuildId(const vector<string> &choppedList, unsigned int depth) {
	string acc;
	for (unsigned int i=0; i <= depth; i++) {
		acc += choppedList[i];
		if (i != depth) {
			acc += '.';
		}
	}
	return acc;
}

// reports errors
pair<SymbolTable *, bool> bindId(const string &s, SymbolTable *env, const TypeStatus &inStatus = TypeStatus()) {
	vector<string> id = chopId(s); // chop up the input identifier into its components
	SymbolTable *stRoot = NULL; // the latch point of the binding
	if (id[0] == "..") { // if the identifier begins with a recall
		Type *recallType = inStatus.type;
		if (recallType) { // if there's a recall binding passed in, use a fake SymbolTable node for it
			// generate a fake identifier for the recall binding node from a hash of the recall identifier's Type object
			string fakeId(FAKE_RECALL_NODE_PREFIX);
			fakeId += (unsigned int)inStatus;
			// check if a SymbolTable node with this identifier already exists -- if so, use it
			map<string, SymbolTable *>::const_iterator fakeFind = env->children.find(fakeId);
			if (fakeFind != env->children.end()) { // if we found a match, use it
				stRoot = (*fakeFind).second;
			} else { // else if we didn't find a match, create a new fake latch point to use
				SymbolTable *fakeStNode = new SymbolTable(KIND_FAKE, fakeId);
				// attach the new fake node to the main SymbolTable
				*env *= fakeStNode;
				// accept the new fake node as the latch point
				stRoot = fakeStNode;
			}
		} else { // else if there is no known recall binding, return an error
			return make_pair((SymbolTable *)NULL, false);
		}
	} else { // else if it's a regular identifier
		for (SymbolTable *stCur = env; stCur != NULL; stCur = stCur->parent) { // scan for a latch point for the beginning of the identifier
			if ((stCur->kind == KIND_STD ||
					stCur->kind == KIND_DECLARATION ||
					stCur->kind == KIND_PARAMETER) &&
					stCur->id == id[0]) { // if this is a valid latch point, log it and break
				stRoot = stCur;
				break;
			} else if (stCur->kind == KIND_BLOCK ||
					stCur->kind == KIND_OBJECT ||
					stCur->kind == KIND_INSTRUCTOR ||
					stCur->kind == KIND_FILTER) { // else if this is a valid basis block, scan its children for a latch point
				map<string, SymbolTable *>::const_iterator latchFind = stCur->children.find(id[0]);
				if (latchFind != stCur->children.end()) { // if we've found a latch point in the children
					stRoot = (*latchFind).second;
					break;
				}
			}
		}
	}
	if (stRoot != NULL) { // if we managed to find a latch point, verify the rest of the binding
		bool needsConstantization = false; // whether this identifier needs to be constantized due to going though a constant reference in the chain
		SymbolTable *stCur = stRoot; // the basis under which we're hoping to bind the current sub-identifier (KIND_STD, KIND_DECLARATION, or KIND_PARAMETER)
		for (unsigned int i = 1; i < id.size(); i++) { // for each sub-identifier of the identifier we're trying to find the binding for
			bool success = false;
			Type *stCurType = errType;
			if (stCur->kind == KIND_STD) { // if it's a standard system-level binding, look in the list of children for a match to this sub-identifier
				map<string, SymbolTable *>::const_iterator childFind = stCur->children.find(id[i]);
				if (childFind != stCur->children.end()) { // if there's a match to this sub-identifier, proceed
					if (*(stCur->defSite->status.type) == STD_STD) { // if it's the root std node, just log the child as stCur and continue in the derivation
						stCur = (*childFind).second;
						success = true;
					} else { // else if it's not the root std node, use the subidentifier's type for derivation, as usual
						stCurType = (*childFind).second->defSite->status.type;
					}
				}
			} else if (stCur->kind == KIND_DECLARATION) { // else if it's a Declaration binding, carefully get its type (we can't derive flow-through types so naively, but these cannot be sub-identified anyway)
				Tree *discriminant = stCur->defSite->child->next->next; // TypedStaticTerm, NonEmptyTerms, or NULL
				if (*discriminant == TOKEN_TypedStaticTerm || *discriminant == TOKEN_BlankInstantiation) { // if it's a Declaration that could possibly have sub-identifiers, derive its type
					stCurType = getStatusDeclaration(stCur->defSite);
				}
			} else if (stCur->kind == KIND_PARAMETER) { // else if it's a Param binding, naively get its type
				stCurType = getStatusParam(stCur->defSite);
			} else if (stCur->kind == KIND_FAKE) { // else if it's a faked SymbolTable node, get its type from the fake Tree node we created for it
				stCurType = stCur->defSite->status.type;
			}
			if (*stCurType) { // if we managed to derive a type for this SymbolTable node
				// handle some special cases based on the suffix of the type we just derived
				if (stCurType->suffix == SUFFIX_LIST || stCurType->suffix == SUFFIX_STREAM) { // else if it's a list or a stream, flag an error, since we can't traverse down those
					Token curToken = stCur->defSite->t;
					semmerError(curToken.fileName,curToken.row,curToken.col,"member access on unmembered identifier '"<<rebuildId(id, i)<<"'");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (identifier type is "<<stCurType<<")");
					stCurType = errType;
				} else if (stCurType->suffix == SUFFIX_ARRAY || stCurType->suffix == SUFFIX_POOL) { // else if it's an array or pool, ensure that we're accessing it using a subscript
					if (id[i] == "[]" || id[i] == "[:]") { // if we're accessing it via a subscript, accept it and proceed deeper into the binding
						// if it's an array type, flag the fact that it must be constantized
						if (stCurType->suffix == SUFFIX_ARRAY) {
							needsConstantization = true;
						}
						// we're about to fake a SymbolTable node for this subscript access
						// but first, check if a SymbolTable node has already been faked for this member
						map<string, SymbolTable *>::const_iterator fakeFind = stCur->children.find(id[i]);
						if (fakeFind != stCur->children.end()) { // if we've already faked a SymbolTable node for this member, accept it and proceed deeper into the binding
							stCur = (*fakeFind).second;
						} else { // else if we haven't yet faked a SymbolTable node for this member, do so now
							// note that the member type that we're using here *will* be defined by this point;
							// if we got here, the ObjectType was in-place defined in a Param (which cannot be recursive), and getStatusParam catches recursion errors
							Type *mutableStCurType = stCurType;
							if (id[i] == "[]") { // if it's an expression access (as opposed to a range access), decrease the type's depth
								mutableStCurType = mutableStCurType->copy(); // make a mutable copy of the type
								mutableStCurType->decreaseDepth();
							}
							SymbolTable *fakeStNode = new SymbolTable(KIND_FAKE, id[i], mutableStCurType);
							// attach the new fake node to the main SymbolTable
							*stCur *= fakeStNode;
							// accept the new fake node and proceed deeper into the binding
							stCur = fakeStNode;
						}
						success = true; // all of the above branches lead to success
					} else {
						Token curToken = stCur->defSite->t;
						semmerError(curToken.fileName,curToken.row,curToken.col,"non-subscript access on identifier '"<<rebuildId(id, i)<<"'");
						semmerError(curToken.fileName,curToken.row,curToken.col,"-- (identifier type is "<<stCurType<<")");
						stCurType = errType;
					}
				} else if (stCurType->category == CATEGORY_OBJECTTYPE) { // else if it's an Object constant or latch (the only other category that can have sub-identifiers)
					// if it's a constant type, flag the fact that it must be constantized
					if (stCurType->suffix == SUFFIX_CONSTANT) {
						needsConstantization = true;
					}
					// proceed with binding the sub-identifier as normal by trying to find a match in the Object's members
					ObjectType *stCurTypeCast = ((ObjectType *)stCurType);
					unsigned int j;
					for (j=0; j < stCurTypeCast->memberNames.size(); j++) {
						if ((stCurTypeCast->memberNames)[j] == id[i]) { // if we found a match for this sub-identifier, break
							break;
						}
					}
					if (j != stCurTypeCast->memberNames.size()) { // if we managed to find a matching sub-identifier
						if ((stCurTypeCast->memberDefSites)[j] != NULL) { // if the member has a real definition site, accept it and proceed deeper into the binding
							stCur = ((stCurTypeCast->memberDefSites)[j])->env;
						} else { // else if the member has no real definition site, we'll need to fake a SymbolTable node for it
							// but first, check if a SymbolTable node has already been faked for this member
							map<string, SymbolTable *>::const_iterator fakeFind = stCur->children.find(id[i]);
							if (fakeFind != stCur->children.end()) { // if we've already faked a SymbolTable node for this member, accept it and proceed deeper into the binding
								stCur = (*fakeFind).second;
							} else { // else if we haven't yet faked a SymbolTable node for this member, do so now
								// note that the member type that we're using here *will* be defined by this point;
								// if we got here, the ObjectType was in-place defined in a Param (which cannot be recursive), and getStatusParam catches recursion errors
								SymbolTable *fakeStNode = new SymbolTable(KIND_FAKE, id[i], (stCurTypeCast->memberTypes)[j]);
								// attach the new fake node to the main SymbolTable
								*stCur *= fakeStNode;
								// accept the new fake node and proceed deeper into the binding
								stCur = fakeStNode;
							}
						}
						success = true; // all of the above branches lead to success
					}
				}
			}
			if (!success) { // if we didn't find a binding for this sub-identifier, return failure
				return make_pair((SymbolTable *)NULL, false);
			} // else if we managed to find a binding for this sub-identifier, continue onto trying to bind the next one
		}
		// if we managed to bind all of the sub-identifiers, return the tail of the binding as well as whether we need to post-constantize it
		return make_pair(stCur, needsConstantization);
	} else { // else if we failed to find an initial latch point, return failure
		return make_pair((SymbolTable *)NULL, false);
	}
}

void subImportDecls(vector<SymbolTable *> importList) {
	bool stdExplicitlyImported = false;
	for(;;) { // per-change loop
		// per-import loop
		vector<SymbolTable *> newImportList;
		for (vector<SymbolTable *>::const_iterator importIter = importList.begin(); importIter != importList.end(); importIter++) {
			// extract the import path out of the iterator
			Tree *importdcn = (*importIter)->defSite->child->next;
			Tree *importSid = (*importdcn == TOKEN_NonArrayedIdentifier || *importdcn == TOKEN_ArrayedIdentifier) ?
				(*importIter)->defSite->child->next :
				(*importIter)->defSite->child->next->next; // NonArrayedIdentifier or ArrayedIdentifier
			string importPath = *importSid; // NonArrayedIdentifier or ArrayedIdentifier
			SymbolTable *importParent = (*importIter)->parent;
			// standard import special-casing
			if (importPath == "std") { // if it's the standard import
				if (!stdExplicitlyImported) { // if it's the first standard import, flag it as handled and let it slide
					(*importIter)->id = STANDARD_IMPORT_DECL_STRING;
					stdExplicitlyImported = true;
					continue;
				}
			}
			// otherwise, try to find a non-std binding for this import
			SymbolTable *binding = bindId(importPath, *importIter).first;
			if (binding != NULL) { // if we found a valid binding
				string importPathTip = binding->id; // must exist if binding succeeed
				if ((*importIter)->kind == KIND_CLOSED_IMPORT) { // if this is a closed-import
					// check to make sure that this import doesn't cause a binding conflict
					string importPathTip = binding->id; // must exist if binding succeeed
					map<string, SymbolTable *>::const_iterator conflictFind = importParent->children.find(importPathTip);
					if (conflictFind == importParent->children.end()) { // there was no conflict, so just copy the binding in place of the import placeholder node
						**importIter = *binding;
					} else { // else if there was a conflict, flag an error
						Token curDefToken = importSid->child->t; // child of NonArrayedIdentifier or ArrayedIdentifier
						Token prevDefToken;
						if ((*conflictFind).second->defSite != NULL) { // if there is a definition site for the previous symbol
							prevDefToken = (*conflictFind).second->defSite->t;
						} else { // otherwise, it must be a standard definition, so make up the token as if it was
							prevDefToken.fileName = STANDARD_LIBRARY_STRING;
							prevDefToken.row = 0;
							prevDefToken.col = 0;
						}
						semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"name conflict in open-importing '"<<importPathTip<<"'");
						semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"-- (conflicting identifier is '"<<(*conflictFind).second->id<<"')");
						semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (conflicting definition was here)");
					}
				} else if ((*importIter)->kind == KIND_OPEN_IMPORT) { // else if this is an open-import
					// try to find an object-style child in the binding's children
					map<string, SymbolTable *>::const_iterator bindingChildIter;
					for (bindingChildIter = binding->children.begin(); bindingChildIter != binding->children.end(); bindingChildIter++) {
						if ((*bindingChildIter).second->kind == KIND_OBJECT) {
							break;
						}
					}
					if (bindingChildIter != binding->children.end()) { // if we found an object in the open-import binding's children
						SymbolTable *bindingBase = (*bindingChildIter).second; // KIND_OBJECT; this node's children are the ones we're going to import in
						// add in the imported nodes, scanning for conflicts along the way
						bool firstInsert = true;
						for (map<string, SymbolTable *>::const_iterator bindingBaseIter = bindingBase->children.begin();
								bindingBaseIter != bindingBase->children.end();
								bindingBaseIter++) {
								// check for member naming conflicts (constructor type conflicts will be resolved later)
								map<string, SymbolTable *>::const_iterator conflictFind = (*importIter)->children.find((*bindingBaseIter).second->id);
								if (conflictFind == (*importIter)->children.end()) { // if there were no member naming conflicts
									if (firstInsert) { // if this is the first insertion, copy in place of the import placeholder node
										**importIter = *((*bindingBaseIter).second);
										firstInsert = false;
									} else { // else if this is not the first insertion, latch in a copy of the child
										SymbolTable *baseChildCopy = new SymbolTable(*((*bindingBaseIter).second));
										*((*importIter)->parent) *= baseChildCopy;
									}
								} else { // else if there is a member naming conflict
									Token curDefToken = importSid->child->t; // child of NonArrayedIdentifier or ArrayedIdentifier
									Token prevDefToken;
									if ((*conflictFind).second->defSite != NULL) { // if there is a definition site for the previous symbol
										prevDefToken = (*conflictFind).second->defSite->t;
									} else { // otherwise, it must be a standard definition, so make up the token as if it was
										prevDefToken.fileName = STANDARD_LIBRARY_STRING;
										prevDefToken.row = 0;
										prevDefToken.col = 0;
									}
									semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"name conflict in importing '"<<importPathTip<<"'");
									semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (conflicting definition was here)");
								}
						}
					} else { // else if we didn't find a binding in the open-import's children, flag an error
						Token curDefToken = importSid->child->t; // child of NonArrayedIdentifier or ArrayedIdentifier
						Token prevDefToken;
						if (binding->defSite != NULL) { // if there is a definition site for the previous symbol
							prevDefToken = binding->defSite->t;
						} else { // otherwise, it must be a standard definition, so make up the token as if it was
							prevDefToken.fileName = STANDARD_LIBRARY_STRING;
							prevDefToken.row = 0;
							prevDefToken.col = 0;
						}
						semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"open import on non-object '"<<importPathTip<<"'");
						semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (importing from here)");
					}
				}
			} else { // else if no binding could be found
				newImportList.push_back(*importIter); // log the failed node for rebinding during the next round
			}
		} // per-import loop
		if (newImportList.size() == importList.size()) { // if the import table has stabilized
			for (vector<SymbolTable *>::const_iterator importIter = newImportList.begin(); importIter != newImportList.end(); importIter++) {
				Token curToken = (*importIter)->defSite->t;
				Tree *importdcn = (*importIter)->defSite->child->next;
				Tree *importSid = (*importdcn == TOKEN_NonArrayedIdentifier || *importdcn == TOKEN_ArrayedIdentifier) ?
					(*importIter)->defSite->child->next :
					(*importIter)->defSite->child->next->next; // NonArrayedIdentifier or ArrayedIdentifier
				string importPath = *importSid; // NonArrayedIdentifier or ArrayedIdentifier
				semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve import '"<<importPath<<"'");
			}
			break;
		} else { // else if the import table hasn't stabilized yet, do another substitution round on the failed binding list
			importList = newImportList;
		}
	} // per-change loop
}

// derives the types of all named nodes in the passed-in SymbolTable
void typeSt(SymbolTable *root) {
	if (root->kind == KIND_DECLARATION || root->kind == KIND_PARAMETER || root->kind == KIND_INSTRUCTOR || root->kind == KIND_OUTSTRUCTOR) { // if it's a named node, derive its type
		getStatusSymbolTable(root);
	}
	// recurse on this node's children
	for (map<string, SymbolTable *>::const_iterator iter = root->children.begin(); iter != root->children.end(); iter++) {
		typeSt((*iter).second);
	}
}

TypeStatus getStatusSymbolTable(SymbolTable *st, const TypeStatus &inStatus) {
	Tree *tree = st->defSite; // set up the tree varaible that the header expects
	GET_STATUS_HEADER;
	if (st->kind == KIND_DECLARATION) { // if the symbol was defined as a Declaration-style node
		returnStatus(getStatusDeclaration(tree, inStatus));
	} else if (st->kind == KIND_PARAMETER) { // else if the symbol was defined as a Param-style node
		returnStatus(getStatusParam(tree, inStatus)); // Param
	} else if (st->kind == KIND_INSTRUCTOR) { // else if the symbol was defined as an instructor-style node
		returnStatus(getStatusInstructor(tree, inStatus)); // Instructor
	} else if (st->kind == KIND_OUTSTRUCTOR) { // else if the symbol was defined as an outstructor-style node
		returnStatus(getStatusOutstructor(tree, inStatus)); // OutStructor
	} else if (st->kind == KIND_FAKE) { // else if the symbol was fake-defined as part of bindId()
		returnTypeRet(tree->status.type, inStatus);
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// typing function definitions

// reports errors
TypeStatus getStatusIdentifier(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	string id = *tree; // string representation of this identifier
	pair<SymbolTable *, bool> binding = bindId(id, tree->env, inStatus);
	SymbolTable *st = binding.first;
	if (st != NULL) { // if we found a binding
		TypeStatus stStatus = getStatusSymbolTable(st, inStatus);
		if (*stStatus) { // if we successfully extracted a type for this SymbolTable entry
			Type *mutableStType = stStatus;
			if (binding.second) { // do the upstream-mandated constantization if needed
				mutableStType = mutableStType->copy();
				mutableStType->constantizeType();
			}
			returnType(mutableStType);
		}
	} else { // else if we couldn't find a binding
		Token curToken = tree->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve '"<<id<<"'");
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusPrimaryBase(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *pbc = tree->child; // NonArrayedIdentifier, ArrayedIdentifier, SingleAccessor, PrimLiteral, or BracketedExp
	if (*pbc == TOKEN_NonArrayedIdentifier || *pbc == TOKEN_ArrayedIdentifier) {
		returnStatus(getStatusIdentifier(pbc, inStatus));
	} else if (*pbc == TOKEN_SingleAccessor) { // if it's an accessed term
		// first, derive the subtype
		Tree *subSI = pbc->next; // NonArrayedIdentifier or ArrayedIdentifier
		TypeStatus subStatus = getStatusIdentifier(subSI, inStatus); // NonArrayedIdentifier or ArrayedIdentifier
		if (*subStatus) { // if we successfully derived a subtype
			if ((*subStatus).suffix == SUFFIX_LATCH || (*subStatus).suffix == SUFFIX_STREAM) { // if the derived type is a latch or a stream
				// copy the Type so that our mutations don't propagate to the NonArrayedIdentifier or ArrayedIdentifier
				TypeStatus mutableSubStatus = subStatus;
				mutableSubStatus.type = mutableSubStatus.type->copy();
				// next, make sure the subtype is compatible with the accessor
				if (mutableSubStatus.type->delatch()) {
					returnStatus(mutableSubStatus);
				} else {
					Token curToken = pbc->child->t; // SLASH
					semmerError(curToken.fileName,curToken.row,curToken.col,"delatch of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<subStatus<<")");
					mutableSubStatus.type->erase();
				}
			} else { // else if the derived type isn't a latch or stream (and thus can't be delatched), error
				Token curToken = pbc->t;
				semmerError(curToken.fileName,curToken.row,curToken.col,"delatch of non-latch, non-stream '"<<subSI<<"'");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<inStatus<<")");
			}
		}
	} else if (*pbc == TOKEN_Instantiation) {
		returnStatus(getStatusInstantiation(pbc, inStatus));
	} else if (*pbc == TOKEN_Filter) {
		returnStatus(getStatusFilter(pbc, inStatus));
	} else if (*pbc == TOKEN_Object) {
		returnStatus(getStatusObject(pbc, inStatus));
	} else if (*pbc == TOKEN_PrimLiteral) {
		returnStatus(getStatusPrimLiteral(pbc, inStatus));
	} else if (*pbc == TOKEN_BracketedExp) {
		returnStatus(getStatusExp(pbc->child->next, inStatus)); // move past the bracket to the actual Exp node
	} else if (*pbc == TOKEN_PrimaryBase) { // postfix operator application
		TypeStatus baseStatus = getStatusPrimaryBase(pbc, inStatus); // derive the status of the base node
		if (*baseStatus) { // if we managed to derive the status of the base node
			if (*baseStatus >> *stdIntType) { // if the base can be converted into an int, return int
				returnType(new StdType(STD_INT, SUFFIX_LATCH));
			} else { // else if we couldn't apply the operator to the type of the subnode, flag an error
				Token curToken = pbc->next->child->t; // the actual operator token
				semmerError(curToken.fileName,curToken.row,curToken.col,"postfix operation '"<<curToken.s<<"' on invalid type");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<baseStatus<<")");
			}
		}
	}
	GET_STATUS_CODE;
	if (*pbc == TOKEN_NonArrayedIdentifier || *pbc == TOKEN_ArrayedIdentifier ||
			*pbc == TOKEN_Instantiation || *pbc == TOKEN_Filter || *pbc == TOKEN_Object || *pbc == TOKEN_PrimLiteral || *pbc == TOKEN_BracketedExp) {
		returnCode(pbc->code());
	} else if (*pbc == TOKEN_SingleAccessor) { // if it's an accessed term
		// first, derive the subtype
		Tree *subSI = pbc->next; // NonArrayedIdentifier or ArrayedIdentifier
		TypeStatus subStatus = getStatusIdentifier(subSI, inStatus); // NonArrayedIdentifier or ArrayedIdentifier
		if (*subStatus) { // if we successfully derived a subtype
			if ((*subStatus).suffix == SUFFIX_LATCH) { // if we're delatching from a latch
				// KOL delatching-from-latch code
			} else if ((*subStatus).suffix == SUFFIX_STREAM) { // else if we're delatching from a stream
				// KOL delatching-from-stream code
			}
		}
	} else if (*pbc == TOKEN_PrimaryBase) { // postfix operator application
		Tree *op = pbc->next->child; // the actual operator token
		if (*op == TOKEN_DPLUS) {
			returnCode(new TempTree(new BinOpTree(BINOP_PLUS_INT, pbc->castCode(*stdIntType), new WordTree(1))));
		} else /* if (*op == TOKEN_DMINUS) */ {
			returnCode(new TempTree(new BinOpTree(BINOP_MINUS_INT, pbc->castCode(*stdIntType), new WordTree(1))));
		}
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusPrimary(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *primaryc = tree->child; // PrimaryBase or PrefixOrMultiOp
	if (*primaryc == TOKEN_PrimaryBase) { // if it's a raw PrimaryBase
		returnStatus(getStatusPrimaryBase(primaryc, inStatus));
	} else if (*primaryc == TOKEN_PrefixOrMultiOp) { // else if it's a PrefixOrMultiOp'ed Primary node
		TypeStatus subStatus = getStatusPrimary(primaryc->next, inStatus); // derive the status of the sub-node
		if (*subStatus) { // if we managed to derive the status of the sub-node
			Tree *pomocc = primaryc->child->child;
			if (*pomocc == TOKEN_NOT) {
				if (*subStatus >> *stdBoolType) {
					returnType(new StdType(STD_BOOL, SUFFIX_LATCH));
				}
			} else if (*pomocc == TOKEN_COMPLEMENT) {
				if (*subStatus >> *stdIntType) {
					returnType(new StdType(STD_INT, SUFFIX_LATCH));
				}
			} else /* if (*pomocc == TOKEN_PLUS || *pomocc == TOKEN_MINUS) */ {
				if (*subStatus >> *stdIntType) {
					returnType(new StdType(STD_INT, SUFFIX_LATCH));
				}
				if (*subStatus >> *stdFloatType) {
					returnType(new StdType(STD_FLOAT, SUFFIX_LATCH));
				}
			}
			// we couldn't derive a valid type for this prefix operation, so flag an error
			Token curToken = primaryc->child->child->t; // the actual operator token
			semmerError(curToken.fileName,curToken.row,curToken.col,"prefix operation '"<<curToken.s<<"' on invalid type");
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<subStatus<<")");
		}
	}
	GET_STATUS_CODE;
	if (*primaryc == TOKEN_PrimaryBase) {
		returnCode(primaryc->code());
	} else if (*primaryc == TOKEN_PrefixOrMultiOp) {
		Tree *primarycn = primaryc->next;
		Tree *pomocc = primaryc->child->child;
		if (*pomocc == TOKEN_NOT) {
			returnCode(new TempTree(new UnOpTree(UNOP_NOT_BOOL, primarycn->castCode(*stdBoolType))));
		} else if (*pomocc == TOKEN_COMPLEMENT) {
			returnCode(new TempTree(new UnOpTree(UNOP_COMPLEMENT_INT, primarycn->castCode(*stdIntType))));
		} else if (*pomocc == TOKEN_PLUS) {
			if (*(primarycn->status) >> *stdIntType) {
				returnCode(primarycn->castCode(*stdIntType));
			} else /* if (*(primarycn->status) >> *stdFloatType) */ {
				returnCode(primarycn->castCode(*stdFloatType));
			}
		} else if (*pomocc == TOKEN_MINUS) {
			if (*(primarycn->status) >> *stdIntType) {
				returnCode(new TempTree(new UnOpTree(UNOP_MINUS_INT, primarycn->castCode(*stdIntType))));
			} else /* if (*(primarycn->status) >> *stdFloatType) */ {
				returnCode(new TempTree(new UnOpTree(UNOP_MINUS_FLOAT, primarycn->castCode(*stdFloatType))));
			}
		}
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusExp(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *expc = tree->child;
	if (*expc == TOKEN_Primary) {
		returnStatus(getStatusPrimary(expc, inStatus));
	} else if (*expc == TOKEN_Exp) {
		Tree *expLeft = expc;
		Tree *op = expLeft->next;
		Tree *expRight = op->next;
		TypeStatus left = getStatusExp(expLeft, inStatus);
		TypeStatus right = getStatusExp(expRight, inStatus);
		if (*left && *right) { // if we derived the types of both operands successfully
			if (!(left->suffix == SUFFIX_CONSTANT || left->suffix == SUFFIX_LATCH)) {
				Token curToken = expLeft->t; // Exp
				semmerError(curToken.fileName,curToken.row,curToken.col,"left operand of expression is not a constant or latch");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (operand type is "<<left<<")");
			} else if (!(right->suffix == SUFFIX_CONSTANT || right->suffix == SUFFIX_LATCH)) {
				Token curToken = expRight->t; // Exp
				semmerError(curToken.fileName,curToken.row,curToken.col,"right operand of expression is not a constant or latch");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (operand type is "<<right<<")");
			} else {
				switch (op->t.tokenType) {
					case TOKEN_DOR:
					case TOKEN_DAND:
						if ((*left >> *stdBoolType) && (*right >> *stdBoolType)) {
							returnType(new StdType(STD_BOOL, SUFFIX_LATCH));
						}
						break;
					case TOKEN_OR:
					case TOKEN_XOR:
					case TOKEN_AND:
						if ((*left >> *stdIntType) && (*right >> *stdIntType)) {
							returnType(new StdType(STD_INT, SUFFIX_LATCH));
						}
						break;
					case TOKEN_DEQUALS:
					case TOKEN_NEQUALS:
					case TOKEN_LT:
					case TOKEN_GT:
					case TOKEN_LE:
					case TOKEN_GE:
						if (left->isComparable(*right)) {
							returnType(new StdType(STD_BOOL, SUFFIX_LATCH));
						}
						break;
					case TOKEN_LS:
					case TOKEN_RS:
						if ((*left >> *stdIntType) && (*right >> *stdIntType)) {
							returnType(new StdType(STD_INT, SUFFIX_LATCH));
						}
						break;
					case TOKEN_TIMES:
					case TOKEN_DIVIDE:
					case TOKEN_MOD:
					case TOKEN_PLUS:
					case TOKEN_MINUS:
						if ((*left >> *stdIntType) && (*right >> *stdIntType)) {
							returnType(new StdType(STD_INT, SUFFIX_LATCH));
						}
						if ((*left >> *stdFloatType) && (*right >> *stdFloatType)) {
							returnType(new StdType(STD_FLOAT, SUFFIX_LATCH));
						}
						// if both terms are convertible to string, return string
						if ((*left >> *stdStringType) && (*right >> *stdStringType)) {
							returnType(new StdType(STD_STRING, SUFFIX_LATCH));
						}
						break;
					default: // can't happen; the above should cover all cases
						break;
				}
				// if we couldn't resolve a type for this expression (or else we would have returned it above)
				Token curToken = op->t; // the actual operator token
				Token curTokenLeft = expLeft->t; // the left operand
				Token curTokenRight = expRight->t; // the right operand
				semmerError(curToken.fileName,curToken.row,curToken.col,"infix operation '"<<curToken.s<<"' on invalid operands");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (left operand type is "<<left<<")");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (right operand type is "<<right<<")");
			}
		}
	}
	GET_STATUS_CODE;
	if (*expc == TOKEN_Primary) {
		returnCode(expc->code());
	} else {
		Tree *expLeft = expc;
		Tree *op = expLeft->next;
		Tree *expRight = op->next;
		switch (op->t.tokenType) {
			case TOKEN_DOR:
				returnCode(new TempTree(new BinOpTree(BINOP_DOR_BOOL, expLeft->castCode(*stdBoolType), expRight->castCode(*stdBoolType))));
			case TOKEN_DAND:
				returnCode(new TempTree(new BinOpTree(BINOP_DAND_BOOL, expLeft->castCode(*stdBoolType), expRight->castCode(*stdBoolType))));
			case TOKEN_OR:
				returnCode(new TempTree(new BinOpTree(BINOP_OR_INT, expLeft->castCode(*stdIntType), expRight->castCode(*stdIntType))));
			case TOKEN_XOR:
				returnCode(new TempTree(new BinOpTree(BINOP_XOR_INT, expLeft->castCode(*stdIntType), expRight->castCode(*stdIntType))));
			case TOKEN_AND:
				returnCode(new TempTree(new BinOpTree(BINOP_AND_INT, expLeft->castCode(*stdIntType), expRight->castCode(*stdIntType))));
			case TOKEN_DEQUALS:
				returnCode(new TempTree(new BinOpTree(BINOP_DEQUALS, expLeft->castCommonCode(expRight->typeRef()), expRight->castCommonCode(expLeft->typeRef()))));
			case TOKEN_NEQUALS:
				returnCode(new TempTree(new BinOpTree(BINOP_NEQUALS, expLeft->castCommonCode(expRight->typeRef()), expRight->castCommonCode(expLeft->typeRef()))));
			case TOKEN_LT:
				returnCode(new TempTree(new BinOpTree(BINOP_LT, expLeft->castCommonCode(expRight->typeRef()), expRight->castCommonCode(expLeft->typeRef()))));
			case TOKEN_GT:
				returnCode(new TempTree(new BinOpTree(BINOP_GT, expLeft->castCommonCode(expRight->typeRef()), expRight->castCommonCode(expLeft->typeRef()))));
			case TOKEN_LE:
				returnCode(new TempTree(new BinOpTree(BINOP_LE, expLeft->castCommonCode(expRight->typeRef()), expRight->castCommonCode(expLeft->typeRef()))));
			case TOKEN_GE:
				returnCode(new TempTree(new BinOpTree(BINOP_GE, expLeft->castCommonCode(expRight->typeRef()), expRight->castCommonCode(expLeft->typeRef()))));
			case TOKEN_LS:
				returnCode(new TempTree(new BinOpTree(BINOP_LS_INT, expLeft->castCode(*stdIntType), expRight->castCode(*stdIntType))));
			case TOKEN_RS:
				returnCode(new TempTree(new BinOpTree(BINOP_RS_INT, expLeft->castCode(*stdIntType), expRight->castCode(*stdIntType))));
			case TOKEN_TIMES:
				if ((*(expLeft->status) >> *stdIntType) && (*(expRight->status) >> *stdIntType)) {
					returnCode(new TempTree(new BinOpTree(BINOP_TIMES_INT, expLeft->castCode(*stdIntType), expRight->castCode(*stdIntType))));
				} else /* if ((*(expLeft->status) >> *stdFloatType) && (*(expRight->status) >> *stdFloatType)) */ {
					returnCode(new TempTree(new BinOpTree(BINOP_TIMES_FLOAT, expLeft->castCode(*stdFloatType), expRight->castCode(*stdFloatType))));
				}
			case TOKEN_DIVIDE:
				if ((*(expLeft->status) >> *stdIntType) && (*(expRight->status) >> *stdIntType)) {
					returnCode(new TempTree(new BinOpTree(BINOP_DIVIDE_INT, expLeft->castCode(*stdIntType), expRight->castCode(*stdIntType))));
				} else /* if ((*(expLeft->status) >> *stdFloatType) && (*(expRight->status) >> *stdFloatType)) */ {
					returnCode(new TempTree(new BinOpTree(BINOP_DIVIDE_FLOAT, expLeft->castCode(*stdFloatType), expRight->castCode(*stdFloatType))));
				}
			case TOKEN_MOD:
				if ((*(expLeft->status) >> *stdIntType) && (*(expRight->status) >> *stdIntType)) {
					returnCode(new TempTree(new BinOpTree(BINOP_MOD_INT, expLeft->castCode(*stdIntType), expRight->castCode(*stdIntType))));
				} else /* if ((*(expLeft->status) >> *stdFloatType) && (*(expRight->status) >> *stdFloatType)) */ {
					returnCode(new TempTree(new BinOpTree(BINOP_MOD_FLOAT, expLeft->castCode(*stdFloatType), expRight->castCode(*stdFloatType))));
				}
			case TOKEN_PLUS:
				if ((*(expLeft->status) >> *stdIntType) && (*(expRight->status) >> *stdIntType)) {
					returnCode(new TempTree(new BinOpTree(BINOP_PLUS_INT, expLeft->castCode(*stdIntType), expRight->castCode(*stdIntType))));
				} else if ((*(expLeft->status) >> *stdFloatType) && (*(expRight->status) >> *stdFloatType)) {
					returnCode(new TempTree(new BinOpTree(BINOP_PLUS_FLOAT, expLeft->castCode(*stdFloatType), expRight->castCode(*stdFloatType))));
				} else /* if ((*(expLeft->status) >> *stdStringType) && (*(expRight->status) >> *stdStringType)) */ {
					returnCode(new TempTree(new BinOpTree(BINOP_PLUS_STRING, expLeft->castCode(*stdStringType), expRight->castCode(*stdStringType))));
				}
			case TOKEN_MINUS:
				if ((*(expLeft->status) >> *stdIntType) && (*(expRight->status) >> *stdIntType)) {
					returnCode(new TempTree(new BinOpTree(BINOP_MINUS_INT, expLeft->castCode(*stdIntType), expRight->castCode(*stdIntType))));
				} else /* if ((*(expLeft->status) >> *stdFloatType) && (*(expRight->status) >> *stdFloatType)) */ {
					returnCode(new TempTree(new BinOpTree(BINOP_MINUS_FLOAT, expLeft->castCode(*stdFloatType), expRight->castCode(*stdFloatType))));
				}
			default: // can't happen; the above should cover all cases
				break;
		}
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPrimOpNode(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *ponc = tree->child->child; // the operator token itself
	// generate the type based on the specific operator it is
	switch (ponc->t.tokenType) {
		case TOKEN_NOT:
			returnType(new StdType(STD_NOT, SUFFIX_LATCH));
		case TOKEN_COMPLEMENT:
			returnType(new StdType(STD_COMPLEMENT, SUFFIX_LATCH));
		case TOKEN_DPLUS:
			returnType(new StdType(STD_DPLUS, SUFFIX_LATCH));
		case TOKEN_DMINUS:
			returnType(new StdType(STD_DMINUS, SUFFIX_LATCH));
		case TOKEN_DOR:
			returnType(new StdType(STD_DOR, SUFFIX_LATCH));
		case TOKEN_DAND:
			returnType(new StdType(STD_DAND, SUFFIX_LATCH));
		case TOKEN_OR:
			returnType(new StdType(STD_OR, SUFFIX_LATCH));
		case TOKEN_XOR:
			returnType(new StdType(STD_XOR, SUFFIX_LATCH));
		case TOKEN_AND:
			returnType(new StdType(STD_AND, SUFFIX_LATCH));
		case TOKEN_DEQUALS:
			returnType(new StdType(STD_DEQUALS, SUFFIX_LATCH));
		case TOKEN_NEQUALS:
			returnType(new StdType(STD_NEQUALS, SUFFIX_LATCH));
		case TOKEN_LT:
			returnType(new StdType(STD_LT, SUFFIX_LATCH));
		case TOKEN_GT:
			returnType(new StdType(STD_GT, SUFFIX_LATCH));
		case TOKEN_LE:
			returnType(new StdType(STD_LE, SUFFIX_LATCH));
		case TOKEN_GE:
			returnType(new StdType(STD_GE, SUFFIX_LATCH));
		case TOKEN_LS:
			returnType(new StdType(STD_LS, SUFFIX_LATCH));
		case TOKEN_RS:
			returnType(new StdType(STD_RS, SUFFIX_LATCH));
		case TOKEN_TIMES:
			returnType(new StdType(STD_TIMES, SUFFIX_LATCH));
		case TOKEN_DIVIDE:
			returnType(new StdType(STD_DIVIDE, SUFFIX_LATCH));
		case TOKEN_MOD:
			returnType(new StdType(STD_MOD, SUFFIX_LATCH));
		case TOKEN_PLUS:
			returnType(new StdType(STD_PLUS, SUFFIX_LATCH));
		case TOKEN_MINUS:
			returnType(new StdType(STD_MINUS, SUFFIX_LATCH));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPrimLiteral(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *plc = tree->child;
	if (*plc == TOKEN_INUM) {
		returnType(new StdType(STD_INT, SUFFIX_LATCH));
	} else if (*plc == TOKEN_FNUM) {
		returnType(new StdType(STD_FLOAT, SUFFIX_LATCH));
	} else if (*plc == TOKEN_CQUOTE) {
		returnType(new StdType(STD_CHAR, SUFFIX_LATCH));
	} else if (*plc == TOKEN_SQUOTE) {
		returnType(new StdType(STD_STRING, SUFFIX_LATCH));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusBlock(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	bool pipeTypesValid = true;
	TypeStatus curStatus = inStatus;
	for (Tree *pipe = tree->child->next->child; pipe != NULL; pipe = (pipe->next != NULL) ? pipe->next->child : NULL) { // Pipe or LastPipe
		// try to get a type for this pipe
		Tree *pipec = pipe->child;
		if (*pipec == TOKEN_Declaration || *pipec == TOKEN_LastDeclaration) { // if it's a declaration-style Pipe, ignore the returned retType (it's used for recursion detection instead)
			TypeStatus thisDeclarationPipeStatus = getStatusPipe(pipe, inStatus);
			if (!(*thisDeclarationPipeStatus)) { // if we failed to derive a type for this declaration-style Pipe, flag this fact
				pipeTypesValid = false;
			}
		} else { // else if it's not a declaration-style Pipe, we must pay attention to the retType
			TypeStatus thisPipeStatus = getStatusPipe(pipe, curStatus);
			if (*thisPipeStatus) { // if we successfully derived a type for this Pipe, log its return type into the current status
				curStatus.retType = thisPipeStatus.retType;
			} else { // else if we failed to derive a type for this Pipe, flag this fact
				pipeTypesValid = false;
			}
		}
	}
	if (pipeTypesValid) { // if we managed to derive a type for all of the enclosed pipes, set the return status to be the appropriate filter type
		if (curStatus.retType == NULL) { // if there were no returns in this block, set this block as returning the null type
			curStatus.retType = nullType;
		}
		returnType(new FilterType(inStatus, curStatus.retType, SUFFIX_LATCH));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusFilterHeader(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	TypeStatus from = TypeStatus(nullType, inStatus);
	TypeStatus to = TypeStatus(nullType, inStatus);
	Tree *treeCur = tree->child->next; // ParamList or RetList
	if (*treeCur == TOKEN_ParamList) {
		from = getStatusParamList(treeCur, inStatus);
		// advance to handle the possible RetList
		treeCur = treeCur->next; // RetList or RSQUARE
	}
	if (*treeCur == TOKEN_RetList) {
		to = getStatusTypeList(treeCur->child->next, inStatus); // TypeList
	}
	if (*from && *to) { // if we succeeded in deriving both the from- and to- statuses
		returnType(new FilterType(from, to, SUFFIX_LATCH));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusFilter(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// fake a type for this node in order to allow for recursion
	Type *&fakeType = tree->status.type;
	fakeType = new FilterType(nullType, nullType, SUFFIX_LATCH);
	TypeStatus startStatus; // the status that we're going to feed into the Block subnode derivation
	startStatus.retType = NULL; // make no initial presuppositions about what type the Block should return
	// derive the declared type of the filter
	Tree *filterCur = tree->child; // Block or FilterHeader
	if (*filterCur == TOKEN_Block) { // if it's an implicit block-defined filter, its type is a consumer of the input type
		((FilterType *)fakeType)->from = (inStatus.type->category == CATEGORY_TYPELIST) ? ((TypeList *)(inStatus.type)) : new TypeList(inStatus.type);
		// set the type to feed into the block derivation to be the one coming in to this filter
		startStatus = inStatus.type;
	} else if (*filterCur == TOKEN_FilterHeader) { // else if it's an explicit header-defined filter, its type is the type of the header
		TypeStatus tempStatus = getStatusFilterHeader(filterCur, inStatus); // derive the (possibly recursive) type of the filter header
		if (*tempStatus) { // if we successfully derived a type for the header
			// log the derived type into the fake type that we previously created
			((FilterType *)fakeType)->from = ((FilterType *)(tempStatus.type))->from;
			((FilterType *)fakeType)->to = ((FilterType *)(tempStatus.type))->to;
			// nullify the type to feed into the block derivation, since we have an explicit parameter list
			startStatus = nullType;
			// advance to the Block definition node
			filterCur = filterCur->next;
		} else { // else if we failed to derive a type for the header, move the fakeType to the LCURLY or LSQUARE below, and set it as erroneous
			tree->child->child->status = fakeType; // LCURLY or LSQUARE
			fakeType = errType;
		}
	}
	if (*fakeType) { // if we successfully derived a type for the header, verify the filter definition Block
		TypeStatus blockStatus = getStatusBlock(filterCur, startStatus); // derive the definition Block's Type
		if (*blockStatus) { // if we successfully derived a type for the definition Block (meaning there were no return type inconsistencies)
			if ((*(((FilterType *)(blockStatus.type))->to) == *nullType && *(((FilterType *)fakeType)->to) == *nullType) ||
					(*(((FilterType *)(blockStatus.type))->to) >> *(((FilterType *)fakeType)->to))) { // if the header and Block return types are compatible
				// log the header type as the return status
				returnType(fakeType);
			} else { // else if the header and Block don't match
				Token curToken = filterCur->child->t; // LCURLY
				semmerError(curToken.fileName,curToken.row,curToken.col,"block returns unexpected type "<<((FilterType *)(blockStatus.type))->to);
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (expected type is "<<((FilterType *)fakeType)->to<<")");
			}
		}
	} else { // else if we derived an erroneous type for the header
		Token curToken = tree->child->child->t; // LCURLY or LSQUARE
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve filter header's type");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusInstructor(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *conscn = tree->child->next; // NULL, SEMICOLON, LSQUARE, or NonRetFilterHeader
	if (conscn == NULL || *conscn == TOKEN_SEMICOLON || *conscn == TOKEN_LSQUARE) {
		returnType(new TypeList());
	} else if (*conscn == TOKEN_NonRetFilterHeader) {
		TypeStatus headerStatus = getStatusFilterHeader(conscn, inStatus);
		if (*headerStatus) { // if we managed to derive a type for the header
			// fake a type for this node in order to allow for recursion in the upcoming Block
			Type *&fakeType = tree->status.type;
			fakeType = ((FilterType *)(headerStatus.type))->from;
			// verify the instructor definition Block
			Tree *block = conscn->next; // Block
			TypeStatus startStatus = inStatus;
			startStatus.retType = errType; // ensure that the Block does not return anything
			TypeStatus blockStatus = getStatusBlock(block, startStatus); // derive the definition Block's Type
			if (*blockStatus) { // if we successfully derived a type for the definition Block, log the header's from-type as the return status
				returnType(((FilterType *)(headerStatus.type))->from);
			} else { // else if we failed to derive a type for the Block, move the fakeType to the EQUALS below
				tree->child->status = fakeType; // EQUALS
			}
		} else { // else if we failed to derive a type for the header, flag an error
			Token curToken = conscn->child->t; // LSQUARE
			semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve instructor's header type");
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusOutstructor(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *rfh = tree->child->next; // RetFilterHeader
	TypeStatus headerStatus = getStatusFilterHeader(rfh, inStatus);
	if (*headerStatus) { // if we managed to derive a type for the header
		// fake a type for this node in order to allow for recursion in the upcoming Block
		Type *&fakeType = tree->status.type;
		fakeType = ((FilterType *)(headerStatus.type))->to;
		// verify the outstructor definition Block
		Tree *block = rfh->next; // Block
		TypeStatus startStatus = inStatus;
		startStatus.retType = NULL; // clear the Block's return type
		TypeStatus blockStatus = getStatusBlock(block, startStatus); // derive the definition Block's Type
		if (*blockStatus) { // if we successfully derived a type for the definition Block
			if (*(((FilterType *)(blockStatus.type))->to) >> *fakeType) { // if the return types are compatible, log the header's to-type as the return status
				returnType(((FilterType *)(headerStatus.type))->to);
			} else { // if the return types are not compatible, flag an error
				Token curToken = block->child->t; // LCURLY
				semmerError(curToken.fileName,curToken.row,curToken.col,"outstructor returns unexpected type "<<((FilterType *)(blockStatus.type))->to);
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (expected type is "<<fakeType<<")");
			}
		} else { // else if we failed to derive a type for the Block, move the fakeType to the EQUALS below
			tree->child->status = fakeType; // EQUALS
		}
	} else { // else if we failed to derive a type for the header, flag an error
		Token curToken = rfh->child->t; // LSQUARE
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve outstructor's header type");
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusObject(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// fake a type for this node in order to allow for recursion
	Type *&fakeType = tree->status.type;
	fakeType = new ObjectType(SUFFIX_LATCH);
	// fake names and types for all members, and log their definition sites
	bool failed = false;
	vector<string> memberNames;
	vector<Type *> memberTypes;
	vector<Tree *> memberDefSites;
	vector<Token> memberTokens;
	SymbolTable *objectSt = tree->env;
	for (map<string, SymbolTable *>::const_iterator memberIter = objectSt->children.begin(); memberIter != objectSt->children.end(); memberIter++) {
		if ((*memberIter).second->kind == KIND_DECLARATION) { // if it's a declaration-style node
			Tree *defSite = (*memberIter).second->defSite; // Declaration
			Token defToken = defSite->child->t; // ID
			memberNames.push_back(defToken.s); // ID
			memberTypes.push_back(NULL);
			memberDefSites.push_back(defSite); // Declaration
			memberTokens.push_back(defToken); // ID
		}
	}
	// commit the fake member names and types, as well as their definition sites
	((ObjectType *)fakeType)->memberNames = memberNames;
	((ObjectType *)fakeType)->memberTypes = memberTypes;
	((ObjectType *)fakeType)->memberDefSites = memberDefSites;
	// derive types for all of the intructors and outstructors
	vector<TypeList *> instructorTypes;
	vector<Token> instructorTokens;
	vector<TypeList *> outstructorTypes;
	vector<Token> outstructorTokens;
	for (map<string, SymbolTable *>::const_iterator memberIter = objectSt->children.begin(); memberIter != objectSt->children.end(); memberIter++) {
		if ((*memberIter).second->kind == KIND_INSTRUCTOR) { // if it's an instructor-style node
			Tree *defSite = (*memberIter).second->defSite; // Instructor
			TypeStatus insStatus = getStatusInstructor(defSite, inStatus); // Instructor
			if (*insStatus) { // if we successfully derived a type for this instructor
				// check if there's already an instructor of this type
				vector<TypeList *>::const_iterator iter1;
				vector<Token>::const_iterator iter2;
				for (iter1 = instructorTypes.begin(), iter2 = instructorTokens.begin(); iter1 != instructorTypes.end(); iter1++, iter2++) {
					if (**iter1 == *insStatus) { // if we've found an outstructor with this same type, break
						break;
					}
				}
				if (iter1 == instructorTypes.end()) { // if there were no conflicts, log the instructor's type in the list
					instructorTypes.push_back((TypeList *)(insStatus.type));
					instructorTokens.push_back(defSite->child->t); // EQUALS
				} else { // otherwise, flag the conflict as an error
					Token curDefToken = defSite->child->t; // EQUALS
					Token prevDefToken = *iter2;
					semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate instructor of type "<<insStatus);
					semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
					failed = true;
				}
			} else { // otherwise, if we failed to derive a type for this instructor
				failed = true;
			}
		} else if ((*memberIter).second->kind == KIND_OUTSTRUCTOR) { // else if it's an outstructor-style node
			Tree *defSite = (*memberIter).second->defSite; // Outstructor
			TypeStatus outsStatus = getStatusOutstructor(defSite, inStatus); // Outstructor
			if (*outsStatus) { // if we successfully derived a type for this outstructor
				// check if there's already a outstructor of this type
				vector<TypeList *>::const_iterator iter1;
				vector<Token>::const_iterator iter2;
				for (iter1 = outstructorTypes.begin(), iter2 = outstructorTokens.begin(); iter1 != outstructorTypes.end(); iter1++, iter2++) {
					if (**iter1 == *outsStatus) { // if we've found an outstructor with this same type, break
						break;
					}
				}
				if (iter1 == outstructorTypes.end()) { // if there were no conflicts, log the outstructor's type in the list
					outstructorTypes.push_back((TypeList *)(outsStatus.type));
					outstructorTokens.push_back(defSite->child->t); // EQUALS
				} else { // otherwise, flag the conflict as an error
					Token curDefToken = defSite->child->t; // EQUALS
					Token prevDefToken = *iter2;
					semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate outstructor of type "<<outsStatus);
					semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
					failed = true;
				}
			} else { // otherwise, if we failed to derive a type for this outstructor
				failed = true;
			}
		}
	}
	// derive types for all of the members that haven't had their types derived already
	for (unsigned int i=0; i < ((ObjectType *)fakeType)->memberTypes.size(); i++) {
		if ((((ObjectType *)fakeType)->memberTypes)[i] == NULL) { // if we haven't yet derived a type for this member, do so now
			TypeStatus memberStatus = getStatusDeclaration(memberDefSites[i], inStatus);
			if (*memberStatus) { // if we successfully derived a type for this Declaration
				(((ObjectType *)fakeType)->memberTypes)[i] = memberStatus.type;
			} else { // else if we failed to derive a type
				(((ObjectType *)fakeType)->memberTypes)[i] = errType;
				failed = true;
			}
		}
	}
	if (!failed) { // if we successfully derived the lists
		// validate all of the regular pipes in this Object definition
		for (Tree *pipe = tree->child->next->next->child; pipe != NULL; pipe = (pipe->next != NULL) ? pipe->next->child : NULL) { // invariant: pipe is a Pipe or LastPipe
			Tree *pipec = pipe->child;
			if (*pipec != TOKEN_Declaration && *pipec != TOKEN_LastDeclaration) { // if it's not a declaration-style Pipe, derive and validate its status
				TypeStatus pipeStatus = getStatusPipe(pipe);
				if (!(*pipeStatus)) { // if we failed to derive a type for this Pipe, flag an error
					failed = true;
				}
			}
		}
		if (!failed) { // if we successfully validated all of the remaining pipes
			// log the derived lists into the fake type that we previously created
			((ObjectType *)fakeType)->instructorTypes = instructorTypes;
			((ObjectType *)fakeType)->outstructorTypes = outstructorTypes;
			// propagate the change to all copies
			((ObjectType *)fakeType)->propagateToCopies();
			// finally, log the completed type as the return status
			returnType(fakeType);
		} else { // else if we failed to validate the remaining pipes, move the fakeType to the LCURLY below
			tree->child->status = fakeType; // LCURLY
		}
	} else { // else if we failed to derive the lists, move the fakeType to the LCURLY below
		tree->child->status = fakeType; // LCURLY
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusType(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *typeSuffix = tree->child->next; // TypeSuffix
	// derive the suffix and depth first, since we'll need to know then to construct the Type object
	bool failed = false;
	int suffixVal;
	int depthVal = 0;
	if (typeSuffix->child == NULL) {
		suffixVal = SUFFIX_CONSTANT;
	} else if (*(typeSuffix->child) == TOKEN_SLASH && typeSuffix->child->next == NULL) {
		suffixVal = SUFFIX_LATCH;
	} else if (*(typeSuffix->child) == TOKEN_LSQUARE) {
		suffixVal = SUFFIX_LIST;
	} else if (*(typeSuffix->child) == TOKEN_DSLASH) {
		suffixVal = SUFFIX_STREAM;
	} else if (*(typeSuffix->child) == TOKEN_ArrayTypeSuffix) {
		suffixVal = SUFFIX_ARRAY;
		Tree *ats = typeSuffix->child; // ArrayTypeSuffix
		for(;;) {
			depthVal++;
			// validate that this suffix expression is valid
			TypeStatus expStatus = getStatusExp(ats->child->next, inStatus); // Exp
			if (!(*expStatus >> *stdIntType)) { // if the expression is incompatible with an integer, flag a bad expression error
				Token curToken = ats->child->t; // LSQUARE
				semmerError(curToken.fileName,curToken.row,curToken.col,"array subscript is invalid");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (subscript type is "<<expStatus<<")");
				failed = true;
			}
			// advance
			if (ats->child->next->next->next != NULL) {
				ats = ats->child->next->next->next; // ArrayTypeSuffix
			} else {
				break;
			}
		}
	} else { // *(typeSuffix->child) == TOKEN_PoolTypeSuffix
		suffixVal = SUFFIX_POOL;
		Tree *pts = typeSuffix->child; // PoolTypeSuffix
		for(;;) {
			depthVal++;
			// validate that this suffix expression is valid
			TypeStatus expStatus = getStatusExp(pts->child->next->next, inStatus); // Exp
			if (!(*expStatus >> *stdIntType)) { // if the expression is incompatible with an integer, flag a bad expression error
				Token curToken = pts->child->next->t; // LSQUARE
				semmerError(curToken.fileName,curToken.row,curToken.col,"pool subscript is invalid");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (subscript type is "<<expStatus<<")");
				failed = true;
			}
			// advance
			if (pts->child->next->next->next->next != NULL) {
				pts = pts->child->next->next->next->next; // PoolTypeSuffix
			} else {
				break;
			}
		}
	}
	if (!failed) { // if all of the suffixes were valid
		Tree *typec = tree->child; // NonArrayedIdentifier, FilterType, or ObjectType
		if (*typec == TOKEN_NonArrayedIdentifier || *typec == TOKEN_ArrayedIdentifier) { // if it's an identifier-defined type
			TypeStatus idStatus = getStatusIdentifier(typec, inStatus); // NonArrayedIdentifier
			if (*idStatus) { // if we managed to derive a type for the instantiation identifier
				if (idStatus.type != stdNullLitType && idStatus.type != stdBoolLitType) { // if the type isn't defined by a standard literal
					idStatus.type = idStatus.type->copy(); // make a copy of the identifier's type, so that the below mutation doesn't propagate to it
					idStatus->suffix = suffixVal;
					idStatus->depth = depthVal;
					returnStatus(idStatus);
				} else { // else if the type is defined by a standard literal, flag an error
					Token curToken = typec->child->t; // guaranteed to be ID, since only NonArrayedIdentifier or ArrayedIdentifier nodes generate inoperable types
					semmerError(curToken.fileName,curToken.row,curToken.col,"standard literal '"<<typec<<"' is not a type");
				}
			}
		} else if (*typec == TOKEN_FilterType) { // else if it's an in-place-defined filter type
			TypeStatus from = nullType;
			TypeStatus to = nullType;
			Tree *sub = typec->child->next; // TypeList or RetList
			if (*sub == TOKEN_TypeList) { // if there is a from-list
				from = getStatusTypeList(sub, inStatus); // TypeList
				// advance (in order to properly handle the possible trailing RetList)
				sub = sub->next; // RetList
			}
			if (*sub == TOKEN_RetList) { // if there is a to-list
				to = getStatusTypeList(sub->child->next, inStatus); // TypeList
			}
			returnType(new FilterType(from, to, suffixVal, depthVal));
		} else if (*typec == TOKEN_ObjectType) { // else if it's an in-place-defined object type
			Tree *otcn = typec->child->next; // RCURLY or ObjectTypeList
			if (*otcn == TOKEN_RCURLY) { // if it's a blank object type
				returnType(new ObjectType(suffixVal, depthVal));
			} else /* if (*otcn == TOKEN_ObjectTypeList) */ { // else if it's a custom-defined object type
				vector<TypeList *> instructorTypes;
				vector<Token> instructorTokens;
				vector<TypeList *> outstructorTypes;
				vector<Token> outstructorTokens;
				vector<string> memberNames;
				vector<Type *> memberTypes;
				vector<Tree *> memberDefSites;
				vector<Token> memberTokens;
				bool failed = false;
				for(Tree *cur = otcn->child; cur != NULL; cur = (cur->next != NULL) ? cur->next->next->child : NULL) { // invariant: cur is a child of ObjectTypeList
					if (*cur == TOKEN_InstructorType) { // if it's an instructor type
						TypeStatus insStatus;
						if (cur->child->next == NULL || cur->child->next->next == NULL) { // if it's an implicitly null instructor, log it as such
							insStatus = TypeStatus(new TypeList(), inStatus);
						} else { // else if it's an explicitly described instructor, get its type from the subnode
							insStatus = getStatusTypeList(cur->child->next->next, inStatus); // TypeList
						}
						if (*insStatus) { // if we successfully derived a type for this instructor
							// check if there's already a instructor of this type
							vector<TypeList *>::const_iterator iter1;
							vector<Token>::const_iterator iter2;
							for (iter1 = instructorTypes.begin(), iter2 = instructorTokens.begin(); iter1 != instructorTypes.end(); iter1++, iter2++) {
								if (**iter1 == *insStatus) {
									break;
								}
							}
							if (iter1 == instructorTypes.end()) { // if there were no conflicts, add the instructor's type to the list
								instructorTypes.push_back((TypeList *)(insStatus.type));
								instructorTokens.push_back(cur->child->t); // EQUALS
							} else { // otherwise, flag the conflict as an error
								Token curDefToken = cur->child->t; // EQUALS
								Token prevDefToken = *iter2;
								semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate instructor of type "<<insStatus);
								semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
								failed = true;
							}
						} else { // otherwise, if we failed to derive a type for this instructor
							failed = true;
						}
					} else if (*cur == TOKEN_OutstructorType) { // if it's an outstructor type
						TypeStatus outsStatus = getStatusTypeList(cur->child->next->next->next, inStatus); // TypeList
						if (*outsStatus) { // if we successfully derived a type for this outstructor
							// check if there's already a outstructor of this type
							vector<TypeList *>::const_iterator iter1;
							vector<Token>::const_iterator iter2;
							for (iter1 = outstructorTypes.begin(), iter2 = outstructorTokens.begin(); iter1 != outstructorTypes.end(); iter1++, iter2++) {
								if (**iter1 == *outsStatus) {
									break;
								}
							}
							if (iter1 == outstructorTypes.end()) { // if there were no conflicts, add the outstructor's type to the list
								outstructorTypes.push_back((TypeList *)(outsStatus.type));
								outstructorTokens.push_back(cur->child->t); // EQUALS
							} else { // otherwise, flag the conflict as an error
								Token curDefToken = cur->child->t; // EQUALS
								Token prevDefToken = *iter2;
								semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate outstructor of type "<<outsStatus);
								semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
								failed = true;
							}
						} else { // otherwise, if we failed to derive a type for this outstructor
							failed = true;
						}
					} else if (*cur == TOKEN_MemberType) { // else if it's a member type
						// check for naming conflicts with this member
						string &stringToAdd = cur->child->t.s; // ID
						vector<string>::const_iterator iter1;
						vector<Token>::const_iterator iter2;
						for (iter1 = memberNames.begin(), iter2 = memberTokens.begin(); iter1 != memberNames.end(); iter1++, iter2++) {
							if (*iter1 == stringToAdd) {
								break;
							}
						}
						if (iter1 == memberNames.end()) { // if there were no naming conflicts with this member
							TypeStatus memberStatus = getStatusType(cur->child->next->next, inStatus); // Type
							if (*memberStatus) { // if we successfully derived a type for this Declaration
								memberNames.push_back(stringToAdd); // ID
								memberTypes.push_back(memberStatus.type);
								memberDefSites.push_back(NULL);
								memberTokens.push_back(cur->child->t); // ID
							} else { // else if we failed to derive a type
								failed = true;
							}
							
						} else { // else if there was a naming conflict with this member
							Token curDefToken = cur->child->t;
							Token prevDefToken = *iter2;
							semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate declaration of object type member '"<<stringToAdd<<"'");
							semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous declaration was here)");
							failed = true;
						}
					}
				}
				if (!failed) {
					returnType(new ObjectType(instructorTypes, outstructorTypes, memberNames, memberTypes, memberDefSites, suffixVal, depthVal));
				}
			}
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusTypeList(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	vector<Type *> list;
	bool failed = false;
	for(Tree *treeCur = tree; treeCur != NULL; treeCur = (treeCur->child->next != NULL) ? treeCur->child->next->next : NULL) { // invariant: treeCur is a TypeList
		Tree *type = treeCur->child; // Type
		TypeStatus curTypeStatus = getStatusType(type, inStatus);
		if (*curTypeStatus) { // if we successfully derived a type for this node
			// commit the type to the list
			list.push_back(curTypeStatus.type);
		} else { // else if we failed to derive a type for this node
			failed = true;
			break;
		}
	}
	if (!failed) {
		returnType(new TypeList(list));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusParam(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// check if this is a recursive invocation
	if (tree->status.retType) { // if we previously logged a recursion alert here (and we don't have a true type to return), flag an ill-formed recursion error
		tree->status.retType = NULL; // fix up the retType to serve its original purpose
		Token curToken = tree->child->next->t; // ID
		semmerError(curToken.fileName,curToken.row,curToken.col,"recursive definition of parameter '"<<curToken.s<<"'");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
	} else { // else if there is no recursion alert for this Param, log one, then continue
		tree->status.retType = errType; // log a recursion alert
		// derive the type normally
		returnStatus(getStatusType(tree->child, inStatus)); // Type
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusParamList(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	vector<Type *> list;
	bool failed = false;
	for (Tree *cur = tree->child; cur != NULL; cur = (cur->next != NULL) ? cur->next->next->child : NULL) { // invariant: cur is a Param
		TypeStatus paramStatus = getStatusParam(cur, inStatus);
		if (*paramStatus) { // if we successfully derived a type for this node
			// commit the type to the list
			list.push_back(paramStatus.type);
		} else { // else if we failed to derive a type for this node
			failed = true;
		}
	}
	if (!failed) {
		returnType(new TypeList(list));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusInstantiationSource(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *itc = tree->child; // NonArrayedIdentifier, SingleAccessor, or MultiAccessor
	if (*itc == TOKEN_NonArrayedIdentifier) { // if it's a regular identifier + suffix-style instantiation
		returnStatus(getStatusType(tree, inStatus)); // InstantiationSource (compatible in this form as a Type)
	} else /* if (*itc == TOKEN_SingleAccessor || *itc == TOKEN_MultiAccessor) */ { // else if it's a copy-style instantiation
		TypeStatus idStatus = getStatusIdentifier(itc->next, inStatus); // NonArrayedIdentifier or ArrayedIdentifier
		if (*idStatus) { // if we managed to derive a type for the identifier we're copying from
			TypeStatus mutableIdStatus = idStatus;
			mutableIdStatus.type = mutableIdStatus.type->copy();
			Tree *accessorc = itc->child; // SLASH, DSLASH, or LSQUARE
			if (*accessorc == TOKEN_SLASH) {
				if (mutableIdStatus.type->copyDelatch()) {
					returnStatus(mutableIdStatus);
				} else {
					Token curToken = accessorc->t; // SLASH, DSLASH, or LSQUARE
					semmerError(curToken.fileName,curToken.row,curToken.col,"copy delatch of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<idStatus<<")");
					mutableIdStatus.type->erase();
				}
			} else if (*accessorc == TOKEN_DSLASH) {
				if (mutableIdStatus.type->copyDestream()) {
					returnStatus(mutableIdStatus);
				} else {
					Token curToken = accessorc->t; // SLASH, DSLASH, or LSQUARE
					semmerError(curToken.fileName,curToken.row,curToken.col,"copy destream of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<idStatus<<")");
					mutableIdStatus.type->erase();
				}
			} else /* if (*accessorc == TOKEN_LSQUARE) */ {
				if (mutableIdStatus.type->copyDelist()) {
					returnStatus(mutableIdStatus);
				} else {
					Token curToken = accessorc->t; // SLASH, DSLASH, or LSQUARE
					semmerError(curToken.fileName,curToken.row,curToken.col,"copy delist of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<idStatus<<")");
					mutableIdStatus.type->erase();
				}
			}
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusInstantiation(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *it = tree->child->next; // InstantiationSource
	TypeStatus instantiation = getStatusInstantiationSource(it, inStatus); // NonCopyInstantiationSource or CopyInstantiationSource
	if (*instantiation) { // if we successfully derived a type for the instantiation
		if (it->next->next != NULL) { // if there's an initializer, we need to make sure that the types are compatible
			Tree *st = it->next->next->next; // StaticTerm
			TypeStatus initializer = getStatusStaticTerm(st, inStatus);
			if (*initializer) { //  if we successfully derived a type for the initializer
				// pipe the types into the status
				if (*initializer >> *instantiation) {
					returnStatus(instantiation);
				} else { // if the types are incompatible, throw an error
					Token curToken = st->t; // StaticTerm
					semmerError(curToken.fileName,curToken.row,curToken.col,"incompatible initializer");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (instantiation type is "<<instantiation<<")");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (initializer type is "<<initializer<<")");
				}
			}
		} else { // else if there is no initializer, check if the type requires one
			if (instantiation->category != CATEGORY_OBJECTTYPE) { // if it's not an object type, it definitely doesn't require an initializer
				returnStatus(instantiation);
			} else { // else if it's an object type
				// check if the object type has a null constructor
				ObjectType *instantiationTypeCast = (ObjectType *)(instantiation.type);
				vector<TypeList *>::const_iterator iter;
				for (iter = instantiationTypeCast->instructorTypes.begin(); iter != instantiationTypeCast->instructorTypes.end(); iter++) {
					if (**iter == *nullType) { // if this is a null constructor, break
						break;
					}
				}
				if (iter != instantiationTypeCast->instructorTypes.end()) { // if we managed to find a null constructor, allow the instantiation
					returnStatus(instantiation);
				} else { // else if we didn't find a null constructor, flag an error
					Token curToken = it->t; // InstantiationSource
					semmerError(curToken.fileName,curToken.row,curToken.col,"null instantiation of object without null constructor");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (instantiation type is "<<instantiation<<")");
				}
			}
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
} 

TypeStatus getStatusNode(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *nodec = tree->child;
	if (*nodec == TOKEN_NonArrayedIdentifier || *nodec == TOKEN_ArrayedIdentifier) {
		returnStatus(getStatusIdentifier(nodec, inStatus)); // erroneous recursion handled by Declaration derivation
	} else if (*nodec == TOKEN_Instantiation) {
		returnStatus(getStatusInstantiation(nodec, inStatus)); // erroneous recursion handled by Declaration derivation
	} else if (*nodec == TOKEN_Filter) {
		returnStatus(getStatusFilter(nodec, inStatus)); // allows for recursive definitions
	} else if (*nodec == TOKEN_Object) {
		returnStatus(getStatusObject(nodec, inStatus)); // allows for recursive definitions
	} else if (*nodec == TOKEN_PrimOpNode) {
		returnStatus(getStatusPrimOpNode(nodec, inStatus)); // can't possibly be recursive
	} else if (*nodec == TOKEN_PrimLiteral) {
		returnStatus(getStatusPrimLiteral(nodec, inStatus)); // can't possibly be recursive
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusTypedStaticTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *tstc = tree->child;
	if (*tstc == TOKEN_Node) { // if it's a regular node
		TypeStatus nodeStatus = getStatusNode(tstc, inStatus);
		if (*nodeStatus) { // if we managed to derive a type for the sub-node
			if (nodeStatus->operable) { // if the node is referensible on its own
				Tree *tstcc = tstc->child;
				if ((*tstcc == TOKEN_NonArrayedIdentifier || *tstcc == TOKEN_ArrayedIdentifier) &&
						!(nodeStatus->category == CATEGORY_FILTERTYPE && nodeStatus->suffix == SUFFIX_LATCH) && nodeStatus.type != stdBoolLitType) { // if the Node needs to be constantized
					// first, copy the Type so that our mutations don't propagate to the StaticTerm
					TypeStatus mutableNodeStatus = nodeStatus;
					mutableNodeStatus.type = mutableNodeStatus.type->copy();
					if (mutableNodeStatus->constantizeReference()) { // if the NonArrayedIdentifier or ArrayedIdentifier can be constantized, log it as the return status
						returnStatus(mutableNodeStatus);
					} else { // if the NonArrayedIdentifier or ArrayedIdentifier cannot be constantized, flag an error
						Token curToken = tstc->child->child->t;
						semmerError(curToken.fileName,curToken.row,curToken.col,"constant reference to dynamic term");
						semmerError(curToken.fileName,curToken.row,curToken.col,"-- (term type is "<<nodeStatus<<")");
						mutableNodeStatus->erase();
					}
				} else { // else if the node doesn't need to be constantized, just return the nodeStatus
					returnStatus(nodeStatus);
				}
			} else { // else if it's a standard node that we can't use an access operator on, flag an error
				Token curToken = tstc->child->child->t; // guaranteed to be ID, since only NonArrayedIdentifier or ArrayedIdentifier nodes generate inoperable types
				semmerError(curToken.fileName,curToken.row,curToken.col,"reference to non-referensible node '"<<tstc->child<<"'");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (node type is "<<nodeStatus<<")");
			}
		}
	} else if (*tstc == TOKEN_BracketedExp) { // else if it's an expression
		TypeStatus expStatus = getStatusExp(tstc->child->next, inStatus); // move past the bracket to the actual Exp node
		if (*expStatus) { // if we managed to derive a type for the expression node
			if (expStatus.type->suffix != SUFFIX_LIST && expStatus.type->suffix != SUFFIX_STREAM) { // if the type isn't inherently dynamic, return the status normally
				returnStatus(expStatus);
			} else { // else if the type is inherently dynamic, flag an error
				Token curToken = tstc->child->t; // LBRACKET
				semmerError(curToken.fileName,curToken.row,curToken.col,"expression returns dynamic type");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (expression type is "<<expStatus<<")");
			}
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusAccess(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// first, derive the Type of the Node that we're acting upon
	TypeStatus nodeStatus = getStatusNode(tree->child->next, inStatus); // Node
	if (*nodeStatus) { // if we managed to derive a type for the subnode
		if (nodeStatus->operable && (nodeStatus.type != stdNullLitType && nodeStatus.type != stdBoolLitType)) { // if the identifier allows access operators
			// copy the Type so that our mutations don't propagate to the Node
			TypeStatus mutableNodeStatus = nodeStatus;
			mutableNodeStatus.type = mutableNodeStatus.type->copy();
			// finally, do the mutation and see check it it worked
			Tree *accessorc = tree->child->child; // SLASH, DSLASH, or LSQUARE
			if (*accessorc == TOKEN_SLASH) {
				if (mutableNodeStatus.type->delatch()) {
					returnStatus(mutableNodeStatus);
				} else {
					Token curToken = accessorc->t; // SLASH, DSLASH, or LSQUARE
					semmerError(curToken.fileName,curToken.row,curToken.col,"delatch of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			} else if (*accessorc == TOKEN_DSLASH) {
				if (mutableNodeStatus.type->destream()) {
					returnStatus(mutableNodeStatus);
				} else {
					Token curToken = accessorc->t; // SLASH, DSLASH, or LSQUARE
					semmerError(curToken.fileName,curToken.row,curToken.col,"destream of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			} else /* if (*accessorc == TOKEN_LSQUARE) */ {
				if (mutableNodeStatus.type->delist()) {
					returnStatus(mutableNodeStatus);
				} else {
					Token curToken = accessorc->t; // SLASH, DSLASH, or LSQUARE
					semmerError(curToken.fileName,curToken.row,curToken.col,"delist of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			}
		} else if (!(nodeStatus->operable)) { // else if it's a standard node that we can't use an access operator on, flag an error
			Token curToken = tree->child->child->t; // SLASH, SSLASH, ASLASH, DSLASH, DSSLASH, or DASLASH
			semmerError(curToken.fileName,curToken.row,curToken.col,"access of immutable node '"<<tree->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (node type is "<<nodeStatus<<")");
		} else /* if (nodeStatus.type == stdNullLitType || nodeStatus.type == stdBoolLitType) */ { // else if it's an access of a standard literal, flag an error
			Token curToken = tree->child->child->t; // SLASH, SSLASH, ASLASH, DSLASH, DSSLASH, or DASLASH
			semmerError(curToken.fileName,curToken.row,curToken.col,"access of immutable literal '"<<tree->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusStaticTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *stc = tree->child;
	if (*stc == TOKEN_TypedStaticTerm) {
		returnStatus(getStatusTypedStaticTerm(stc, inStatus));
	} else /* if (*stc == TOKEN_Access) */ {
		returnStatus(getStatusAccess(stc, inStatus));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusDynamicTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *dtc = tree->child;
	if (*dtc == TOKEN_Compound) {
		TypeStatus compoundStatus = getStatusStaticTerm(dtc->child->next, inStatus);
		if (*compoundStatus) { // if we managed to derive the compounding term's type
			Type *curType = inStatus;
			TypeList *curTypeList;
			if (curType->category == CATEGORY_TYPELIST) { // if the current type is already a TypeList, simply make a mutable copy of it
				curTypeList = (TypeList *)(curType->copy()); // create a mutable copy of the incoming Type
			} else { // else if the current type is not a TypeList, we must wrap it in one
				curTypeList = new TypeList(curType);
			}
			// finally, add the compounding term to the ongoing TypeList
			curTypeList->list.push_back(compoundStatus.type);
			returnType(curTypeList);
		}
	} else if (*dtc == TOKEN_Link) {
		TypeStatus linkStatus = getStatusStaticTerm(dtc->child->next);
		if (*linkStatus) { // if we managed to derive a type for the Link subnode
			if (*linkStatus >> *inStatus) { // if the we can back-cast into a successful link, log return the incoming type
				returnStatus(inStatus);
			} else {
				Token curToken = dtc->child->t; // DCOLON
				semmerError(curToken.fileName,curToken.row,curToken.col,"link with incompatible type");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (incoming type is "<<inStatus<<")");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (link type is "<<linkStatus<<")");
			}
		}
	} else if (*dtc == TOKEN_Send) {
		TypeStatus nodeStatus = getStatusNode(dtc->child->next, inStatus);
		if (*nodeStatus) { // if we managed to derive a type for the send destination
			if ((nodeStatus->operable || nodeStatus.type == stringerType) &&
					(nodeStatus.type != stdNullLitType && nodeStatus.type != stdBoolLitType)) { // if the destination allows sends
				if (*inStatus >> *nodeStatus) { // if the Send is valid, proceed normally
					returnType(nullType);
				} else { // else if the Send is invalid, flag an error
					Token curToken = dtc->child->t; // RARROW
					semmerError(curToken.fileName,curToken.row,curToken.col,"send to incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (source type is "<<inStatus<<")");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (destination type is "<<nodeStatus<<")");
				}
			} else if (!(nodeStatus->operable)) { // else if it's a standard node that we can't use an access operator on, flag an error
				Token curToken = dtc->child->t; // RARROW
				semmerError(curToken.fileName,curToken.row,curToken.col,"send to immutable node '"<<dtc->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (node type is "<<nodeStatus<<")");
			} else /* if (nodeStatus.type == stdNullLitType || nodeStatus.type == stdBoolLitType) */ { // else if it's an access of a standard literal, flag an error
				Token curToken = dtc->child->t; // RARROW
				semmerError(curToken.fileName,curToken.row,curToken.col,"send to immutable literal '"<<dtc->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
			}
		}
	} else if (*dtc == TOKEN_Swap) {
		TypeStatus nodeStatus = getStatusNode(dtc->child->next, inStatus);
		if (*nodeStatus) { // if we managed to derive a type for the swap destination
			if ((nodeStatus->operable || nodeStatus.type == stringerType) &&
					(nodeStatus.type != stdNullLitType && nodeStatus.type != stdBoolLitType)) { // if the destination allows swaps
				if (*inStatus >> *nodeStatus) { // if the Send is valid, proceed normally
					returnType(nullType);
				} else { // else if the Send is invalid, flag an error
					Token curToken = dtc->child->t; // RARROW
					semmerError(curToken.fileName,curToken.row,curToken.col,"swap with incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (source type is "<<inStatus<<")");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (destination type is "<<nodeStatus<<")");
				}
			} else if (!(nodeStatus->operable)) { // else if it's a standard node that we can't use an access operator on, flag an error
				Token curToken = dtc->child->t; // LRARROW
				semmerError(curToken.fileName,curToken.row,curToken.col,"swap with immutable node '"<<dtc->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (node type is "<<nodeStatus<<")");
			} else /* if (nodeStatus.type == stdNullLitType || nodeStatus.type == stdBoolLitType) */ { // else if it's an access of a standard literal, flag an error
				Token curToken = dtc->child->t; // LRARROW
				semmerError(curToken.fileName,curToken.row,curToken.col,"swap with immutable literal '"<<dtc->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
			}
		}
	} else if (*dtc == TOKEN_Return) {
		Type *thisRetType = inStatus; // the type that we're returning, inferred from the incoming status
		Type *knownRetType = inStatus.retType; // the current return type (i.e. the one we're expecting, or otherwise NULL)
		if (knownRetType != NULL) { // if there's already a return type logged, make sure it's compatible with this one
			// try a back-cast from the current return type to the known one
			if (*thisRetType >> *knownRetType) { // if the cast succeeded, use the known return type
				returnTypeRet(nullType, knownRetType);
			}
			// try a forward-cast from the known return type to the current one
			if (*thisRetType >> *knownRetType) { // if the cast succeeded, use the new return type
				returnTypeRet(nullType, thisRetType);
			} else { // else if this return type conflicts with the known one, flag an error
				Token curToken = dtc->child->t; // DRARROW
				semmerError(curToken.fileName,curToken.row,curToken.col,"return of unexpected type "<<thisRetType);
				if (*knownRetType == *errType) {
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (not expecting a return here)");
				} else {
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (expected type is "<<knownRetType<<")");
				}
			}
		} else { // else if there is no return type logged, log this one and proceed normally
			returnTypeRet(nullType, thisRetType);
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusSwitchTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	vector<TypeStatus> toStatus; // vector for logging the destination statuses of each branch
	vector<Tree *> toTrees; // vector for logging the tree nodes of each branch
	 // LabeledPipes
	for (Tree *ltCur = tree->child->next->next->child; ltCur != NULL; ltCur = (ltCur->next != NULL) ? ltCur->next->child : NULL) { // invariant: ltc is a LabeledTerm or LastLabeledTerm
		Tree *ltc = ltCur->child; // StaticTerm or COLON
		// if there is a non-default label on this pipe, check its validity
		if (*ltc == TOKEN_StaticTerm) {
			// derive the label's type
			TypeStatus label = getStatusStaticTerm(ltc, inStatus);
			if (!(*label >> *inStatus)) { // if the type doesn't match, throw an error
				Token curToken = ltc->t;
				semmerError(curToken.fileName,curToken.row,curToken.col,"incompatible switch label");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (label type is "<<label<<")");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
			}
		}
		// derive the to-type of this label
		Tree *toTree = (*ltc == TOKEN_StaticTerm) ? ltc->next->next : ltc->next; // SimpleTerm
		TypeStatus to = getStatusSimpleTerm(toTree, inStatus);
		// log the to-type and to-tree of this label
		toStatus.push_back(to);
		toTrees.push_back(toTree);
	} // per-labeled pipe loop
	// verify that all of the to-types are the same
	bool failed = false;
	TypeStatus firstToStatus = toStatus[0];
	Tree *firstToTree = toTrees[0];
	for (unsigned int i=1; i < toStatus.size(); i++) { // for each to-type
		TypeStatus thisToStatus = toStatus[i];
		if (!(thisToStatus->baseEquals(*firstToStatus) && *thisToStatus == *firstToStatus)) { // if the types don't match, throw an error
			Tree *toTree = toTrees[i];
			Token curToken1 = toTree->t;
			Token curToken2 = firstToTree->t;
			semmerError(curToken1.fileName,curToken1.row,curToken1.col,"inconsistent switch destination type");
			semmerError(curToken1.fileName,curToken1.row,curToken1.col,"-- (this type is "<<thisToStatus<<")");
			semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (first type is "<<firstToStatus<<")");
			failed = true;
		}
	}
	if (!failed) {
		returnStatus(firstToStatus);
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusSimpleTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *stc = tree->child;
	if (*stc == TOKEN_StaticTerm) {
		returnStatus(getStatusStaticTerm(stc, inStatus));
	} else if (*stc == TOKEN_SwitchTerm) {
		returnStatus(getStatusSwitchTerm(stc, inStatus));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusSimpleCondTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		returnStatus(getStatusTerm(tree->child->next, TypeStatus(nullType, NULL)));
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusOpenOrClosedCondTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		Tree *trueBranchc = tree->child->next->child; // SimpleTerm or ClosedCondTerm
		Tree *falseBranchc = tree->child->next->next->next->child; // SimpleTerm, ClosedCondTerm, SimpleCondTerm, or ClosedCondTerm
		TypeStatus trueStatus;
		if (*trueBranchc == TOKEN_SimpleTerm) {
			trueStatus = getStatusSimpleTerm(trueBranchc, TypeStatus(nullType, NULL));
		} else /* if (*trueBranchc == TOKEN_ClosedCondTerm) */ {
			trueStatus = getStatusOpenOrClosedCondTerm(trueBranchc, TypeStatus(nullType, NULL));
		}
		TypeStatus falseStatus;
		if (*falseBranchc == TOKEN_SimpleTerm) {
			falseStatus = getStatusSimpleTerm(falseBranchc, TypeStatus(nullType, NULL));
		} else if (*falseBranchc == TOKEN_SimpleCondTerm) {
			falseStatus = getStatusSimpleCondTerm(falseBranchc, TypeStatus(nullType, NULL));
		} else /* if (*falseBranchc == TOKEN_ClosedCondTerm || *falseBranchc == TOKEN_OpenCondTerm) */ {
			falseStatus = getStatusOpenOrClosedCondTerm(falseBranchc, TypeStatus(nullType, NULL));
		}
		if (*trueStatus && *falseStatus) { // if we managed to derive types for both branches
			if (*trueStatus == *falseStatus) { // if the two branches match in type
				returnStatus(trueStatus);
			} else { // else if the two branches don't match in type
				Token curToken1 = tree->child->t; // QUESTION
				Token curToken2 = trueBranchc->t; // SimpleTerm or ClosedCondTerm
				Token curToken3 = falseBranchc->t; // SimpleTerm, ClosedCondTerm, SimpleCondTerm, or ClosedCondTerm
				semmerError(curToken1.fileName,curToken1.row,curToken1.col,"type mismatch in conditional branches");
				semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (true branch type is "<<trueStatus<<")");
				semmerError(curToken3.fileName,curToken3.row,curToken3.col,"-- (false branch type is "<<falseStatus<<")");
			}
		}
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *tc = tree->child;
	if (*tc != TOKEN_DynamicTerm) { // if it's not a DynamicTerm
		Tree *tcc = tree->child->child;
		if (*tcc == TOKEN_SimpleTerm) {
			returnStatus(getStatusSimpleTerm(tcc, inStatus));
		} else if (*tcc == TOKEN_SimpleCondTerm) {
			returnStatus(getStatusSimpleCondTerm(tcc, inStatus));
		} else if (*tcc == TOKEN_ClosedCondTerm || *tcc == TOKEN_OpenCondTerm) {
			returnStatus(getStatusOpenOrClosedCondTerm(tcc, inStatus));
		}
	} else { // else if it's a DynamicTerm
		returnStatus(getStatusDynamicTerm(tc, inStatus));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusNonEmptyTerms(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// scan the pipe left to right
	TypeStatus curStatus = inStatus;
	Tree *curTerm;
	Tree *prevTerm;
	for (curTerm = prevTerm = tree->child; curTerm != NULL; prevTerm = curTerm, curTerm = curTerm->next->child) {
		// derive a type for the next term in the sequence
		TypeStatus nextTermStatus = getStatusTerm(curTerm, curStatus);
		if (*nextTermStatus) { // if we managed to derive a type for this term
			if (*(curTerm->child->child) == TOKEN_SimpleTerm &&
					*(curTerm->child->child->child) == TOKEN_StaticTerm &&
					*(curTerm->child->child->child->child->child) == TOKEN_Node) { // if it's a flow-through Term
				pair<Type *, bool> stdFlowResult(errType, false);
				if (nextTermStatus->category == CATEGORY_STDTYPE) { // if this Term's type is a STDTYPE, try to derive a three-term exceptional type for it
					stdFlowResult = ((StdType *)(nextTermStatus.type))->stdFlowDerivation(curStatus, curTerm->next->child);
				}
				if (*(stdFlowResult.first)) { // if we managed to derive a three-term exceptional type for this term
					curStatus = TypeStatus(stdFlowResult.first, nextTermStatus); // log the three-term exceptional type as the current status
					if (stdFlowResult.second) { // if we used a third term for the derivation, advance curTerm past it
						curTerm = curTerm->next->child;
					}
				} else { // else if this is not a three-term STDTYPE filter exception case
					// derive a type for the flow of the current type into the next term in the sequence
					Type *flowResult = (*curStatus , *nextTermStatus);
					if (*flowResult) { // if the type flow is valid, log it as the current status
						curStatus = TypeStatus(flowResult, nextTermStatus);
					} else if (*curStatus == *nullType) { // else if the flow is not valid, but the incoming type is null, log the next term's status as the current one
						curStatus = nextTermStatus;
					} else { // else if the type flow is not valid and the incoming type is not null, flag an error
						Token curToken = curTerm->t; // Term
						Token prevToken = prevTerm->t; // Term
						semmerError(curToken.fileName,curToken.row,curToken.col,"term does not accept incoming type");
						semmerError(curToken.fileName,curToken.row,curToken.col,"-- (term's type is "<<nextTermStatus<<")");
						semmerError(prevToken.fileName,prevToken.row,prevToken.col,"-- (incoming type is "<<curStatus<<")");
						// short-circuit the derivation of this NonEmptyTerms
						curStatus = errType;
						break;
					}
				}
			} else { // else if it's not a flow-through Term, log the next term's status as the current one
				curStatus = nextTermStatus;
			}
		} else { // otherwise, if we failed to derive a type for this term, flag an error
			Token curToken = curTerm->t;
			semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve term's output type");
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<curStatus<<")");
			// short-circuit the derivation for this NonEmptyTerms
			curStatus = errType;
			break;
		}
	}
	// if we succeeded in deriving an output type, return it
	if (*curStatus) {
		returnStatus(curStatus);
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusDeclaration(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// check if this is a recursive invocation
	Type *&fakeRetType = tree->status.retType;
	if (fakeRetType != NULL) { // if we previously logged a recursion alert here (and we don't have a memoized type to return), flag an ill-formed recursion error
		Token curToken = tree->child->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"irresolvable recursive definition of '"<<curToken.s<<"'");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
	} else { // else if there is no recursion alert for this Declaration and it's not a standard literal override, continue
		// if the sub-node is not recursion-safe, institute a recursion warning for this Declaration
		Tree *declarationSub = tree->child->next->next; // TypedStaticTerm, NonEmptyTerms, or NULL
		if (!(declarationSub != NULL && *declarationSub == TOKEN_TypedStaticTerm && *(declarationSub->child) == TOKEN_Node &&
				(*(declarationSub->child->child) == TOKEN_Object || *(declarationSub->child->child) == TOKEN_Filter))) { // only Objects and Filters are exempt
			fakeRetType = errType; // log a recursion alert
		}
		// proceed with the normal derivation
		if (*(tree->child) != TOKEN_AT && *(tree->child) != TOKEN_DAT) { // if it's a non-import declaration
			// attempt to derive the type of this Declaration
			if (*declarationSub == TOKEN_TypedStaticTerm) { // if it's a regular declaration
				returnTypeRet(getStatusTypedStaticTerm(declarationSub), NULL);
			} else if (*declarationSub == TOKEN_BlankInstantiation) { // else if it's a blank instantiation declaration
				returnTypeRet(getStatusInstantiation(declarationSub), NULL);
			} else if (*declarationSub == TOKEN_NonEmptyTerms) { // else if it's a regular flow-through declaration
				// first, set the identifier's type to the type of the NonEmptyTerms stream (an inputType consumer) in order to allow for recursion
				tree->status.type = new FilterType(inStatus, nullType, SUFFIX_LATCH);
				// then, verify types for the declaration sub-block
				TypeStatus netsStatus = getStatusNonEmptyTerms(declarationSub, inStatus);
				// delete the temporary filter type
				delete (tree->status.type);
				// finally, return the type of the declaration sub-block
				returnTypeRet(netsStatus, NULL);
			}
		} else { // otherwise, if it's an import declaration, do nothing; typing of the import will be handled at the definition site
			returnTypeRet(nullType, NULL);
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPipe(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *pipec = tree->child; // Declaration, NonEmptyTerms, or LastDeclaration
	if (*pipec == TOKEN_Declaration || *pipec == TOKEN_LastDeclaration) { // if it's a Declaration-style pipe
		returnStatus(getStatusDeclaration(pipec, inStatus));
	} else if (*pipec == TOKEN_NonEmptyTerms) { // else if it's a raw NonEmptyTerms pipe
		returnStatus(getStatusNonEmptyTerms(pipec, inStatus));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

void typePipes(Tree *treeRoot) {
	TypeStatus rootStatus(nullType, stdIntType);
	for (Tree *programCur = treeRoot; programCur != NULL; programCur = programCur->next) {
		for (Tree *pipeCur = programCur->child->child; pipeCur != NULL; pipeCur = (pipeCur->next != NULL) ? pipeCur->next->child : NULL) {
			getStatusPipe(pipeCur, rootStatus);
		}
	}
}

// main semming function; makes no assumptions about stRoot's value; it's just a return parameter
int sem(Tree *treeRoot, SymbolTable *&stRoot) {

	// initialize error code
	semmerErrorCode = 0;

	VERBOSE( printNotice("building symbol table..."); )

	// initialize the standard types used for comparison
	initStdTypes();
	
	// initialize the symbol table root with the default definitions
	stRoot = genDefaultDefs();

	// populate the symbol table with definitions from the user parseme, and log the used imports
	vector<SymbolTable *> importList; // import Declaration nodes
	buildSt(treeRoot, stRoot, importList); // get user definitions/imports
	subImportDecls(importList); // resolve and substitute import declarations into the symbol table

	VERBOSE( printNotice("tracing data flow..."); )

	// derive types of all identifiers in the SymbolTable
	typeSt(stRoot);
	// derive types for the remaining pipes
	typePipes(treeRoot);
	
	VERBOSE( cout << stRoot; )

	// if there were no errors, free the error type node
	if (!semmerErrorCode) {
		delete errType;
	}

	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
