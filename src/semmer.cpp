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
StdType *stdBoolLitType;
StdType *inIntType;
StdType *inFloatType;
StdType *inCharType;
StdType *inStringType;
ObjectType *stringerType;
ObjectType *boolUnOpType;
ObjectType *intUnOpType;
ObjectType *boolBinOpType;
ObjectType *intBinOpType;
ObjectType *floatBinOpType;
ObjectType *boolCompOpType;
ObjectType *intCompOpType;
ObjectType *floatCompOpType;
ObjectType *charCompOpType;
ObjectType *stringCompOpType;
StdType *stdLibType;
SymbolTree *stdLib;
IRTree *nopCode;

// SymbolTree functions
SymbolTree::SymbolTree(int kind, const string &id, Tree *defSite, SymbolTree *copyImportSite) : kind(kind), id(id), defSite(defSite), copyImportSite(copyImportSite), parent(NULL),
		offsetKindInternal(OFFSET_NULL), numRaws(0), numBlocks(0), numPartitions(0) {
	if (defSite != NULL) {
		defSite->env = this;
	}
}
SymbolTree::SymbolTree(int kind, const char *id, Tree *defSite, SymbolTree *copyImportSite) : kind(kind), id(id), defSite(defSite), copyImportSite(copyImportSite), parent(NULL), 
		offsetKindInternal(OFFSET_NULL), numRaws(0), numBlocks(0), numPartitions(0) {
	if (defSite != NULL) {
		defSite->env = this;
	}
}
SymbolTree::SymbolTree(int kind, const string &id, Type *defType, SymbolTree *copyImportSite) : kind(kind), id(id), copyImportSite(copyImportSite), parent(NULL),
		offsetKindInternal(OFFSET_NULL), numRaws(0), numBlocks(0), numPartitions(0){
	TypeStatus status(defType, NULL); defSite = new Tree(status); defSite->env = this;
}
SymbolTree::SymbolTree(int kind, const char *id, Type *defType, SymbolTree *copyImportSite) : kind(kind), id(id), copyImportSite(copyImportSite), parent(NULL),
		offsetKindInternal(OFFSET_NULL), numRaws(0), numBlocks(0), numPartitions(0){
	TypeStatus status(defType, NULL); defSite = new Tree(status); defSite->env = this;
}
SymbolTree::SymbolTree(const SymbolTree &st, SymbolTree *parent, SymbolTree *copyImportSite) : kind(st.kind), id(st.id), defSite(st.defSite), copyImportSite(copyImportSite), parent(parent), children(st.children),
	offsetKindInternal(st.offsetKindInternal), offsetIndexInternal(st.offsetIndexInternal), numRaws(st.numRaws), numBlocks(st.numBlocks), numPartitions(st.numPartitions) {}
SymbolTree::~SymbolTree() {}
unsigned int SymbolTree::addRaw() {return (numRaws++);}
unsigned int SymbolTree::addBlock() {return (numBlocks++);}
unsigned int SymbolTree::addPartition() {return (numPartitions++);}
unsigned int SymbolTree::addShare() {return (numShares++);}
void SymbolTree::getOffset() {
	if (kind != KIND_STD && parent != NULL && defSite->status.type != NULL) { // if this is a node for which we can derive the offset from the corresponding Type, do so
		offsetKindInternal = defSite->status.type->offsetKind();
		switch (offsetKindInternal) {
			case OFFSET_RAW:
				offsetIndexInternal = parent->addRaw();
				break;
			case OFFSET_BLOCK:
				offsetIndexInternal = parent->addBlock();
				break;
			case OFFSET_PARTITION:
				offsetIndexInternal = parent->addPartition();
				break;
			case OFFSET_SHARE:
				offsetIndexInternal = parent->addShare();
				break;
			default:
				// can't happen; Type::offsetKind() doesn't return OFFSET_NULL or OFFSET_FREE
				break;
		}
	} else { // if this is a standard node, a parentless node, or one with a NULL type, set it to free allocation
		offsetKindInternal = OFFSET_FREE;
	}
}
int SymbolTree::offsetKind() {
	if (offsetKindInternal == OFFSET_NULL) {
		getOffset();
	}
	return offsetKindInternal;
}
unsigned int SymbolTree::offsetIndex() {
	if (offsetKindInternal == OFFSET_NULL) {
		getOffset();
	}
	return offsetIndexInternal;
}
Tree *SymbolTree::offsetExp() const {
	return (defSite->status.type->offsetExp);
}
SymbolTree &SymbolTree::operator=(const SymbolTree &st) {
	kind = st.kind;
	defSite = st.defSite;
	copyImportSite = st.copyImportSite;
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
SymbolTree &SymbolTree::operator*=(SymbolTree *st) {
	// first, check for conflicting bindings
	if (st->kind == KIND_STD || st->kind == KIND_DECLARATION || st->kind == KIND_PARAMETER) { // if this is a conflictable (non-special system-level binding)
		// per-symbol loop
		map<string, SymbolTree *>::const_iterator conflictFind = children.find(st->id);
		if (conflictFind != children.end()) { // if we've found a conflict
			SymbolTree *conflictSt = (*conflictFind).second;
			Token curDefToken;
			if (st->defSite != NULL) { // if there is a definition site for the current symbol
				curDefToken = st->defSite->t;
			} else { // otherwise, it must be a standard definition, so make up the token as if it was
				curDefToken.fileIndex = STANDARD_LIBRARY_FILE_INDEX;
				curDefToken.row = 0;
				curDefToken.col = 0;
				exit(1);
			}
			Token prevDefToken;
			if (conflictSt->defSite != NULL) { // if there is a definition site for the previous symbol
				prevDefToken = conflictSt->defSite->t;
			} else { // otherwise, it must be a standard definition, so make up the token as if it was
				prevDefToken.fileIndex = STANDARD_LIBRARY_FILE_INDEX;
				prevDefToken.row = 0;
				prevDefToken.col = 0;
				exit(1);
			}
			semmerError(curDefToken.fileIndex,curDefToken.row,curDefToken.col,"redefinition of '"<<st->id<<"'");
			semmerError(prevDefToken.fileIndex,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
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
string SymbolTree::toString(unsigned int tabDepth) {
	if (kind == KIND_FAKE) {
		return "";
	}
	string acc("\t");
	for (unsigned int i = 0; i < tabDepth; i++) {
		if (i < (tabDepth-2)) {
			acc += "| ";
		} else if (i == (tabDepth-2)) {
			acc += "|-";
		} else if (i == (tabDepth-1)) {
			acc += "--";
		}
	}
	if (kind != KIND_BLOCK && kind != KIND_CLOSED_IMPORT && kind != KIND_OPEN_IMPORT) {
		COLOR( acc += SET_TERM(BRIGHT_CODE AND WHITE_CODE); )
		if (kind == KIND_INSTRUCTOR) {
			acc += INSTRUCTOR_NODE_STRING;
		} else if (kind == KIND_OUTSTRUCTOR) {
			acc += OUTSTRUCTOR_NODE_STRING;
		} else if (kind == KIND_FILTER) {
			acc += FILTER_NODE_STRING;
		} else if (kind == KIND_OBJECT) {
			acc += OBJECT_NODE_STRING;
		} else if (kind == KIND_INSTANTIATION) {
			acc += INSTANTIATION_NODE_STRING;
		} else {
			acc += id;
		}
		COLOR( acc += SET_TERM(RESET_CODE); )
		if (kind != KIND_OBJECT) {
			Type *defType = defSite->status.type;
			if (defType != NULL) {
				acc += " : ";
				acc += defType->toString();
			}
		}
		acc += " <";
		COLOR( acc += SET_TERM(BRIGHT_CODE AND BLACK_CODE); )
		switch(offsetKind()) {
			case OFFSET_RAW: {
				acc += "RAW:";
				char offsetString[MAX_INT_STRING_LENGTH];
				sprintf(offsetString, "%u", offsetIndex());
				acc += offsetString;
				break;
			}
			case OFFSET_BLOCK: {
				acc += "BLK:";
				char offsetString[MAX_INT_STRING_LENGTH];
				sprintf(offsetString, "%u", offsetIndex());
				acc += offsetString;
				break;
			}
			case OFFSET_PARTITION: {
				acc += "PRT:";
				char offsetString[MAX_INT_STRING_LENGTH];
				sprintf(offsetString, "%u", offsetIndex());
				acc += offsetString;
				break;
			}
			case OFFSET_SHARE: {
				acc += "SHA:";
				char offsetString[MAX_INT_STRING_LENGTH];
				sprintf(offsetString, "%u", offsetIndex());
				acc += offsetString;
				break;
			}
			case OFFSET_FREE: {
				acc += "FRE";
				break;
			}
			default: // can't happen; the above should cover all cases
				break;
		}
		COLOR( acc += SET_TERM(RESET_CODE); )
		acc += '>';
	} else {
		COLOR( acc += SET_TERM(BRIGHT_CODE AND WHITE_CODE); )
		acc += id;
		COLOR( acc += SET_TERM(RESET_CODE); )
	}
	acc += '\n';
	for (map<string, SymbolTree *>::iterator childIter = children.begin(); childIter != children.end(); childIter++) {
		SymbolTree *childCur = (*childIter).second;
		if (childCur != NULL) {
			acc += childCur->toString(tabDepth+1);
		}
	}
	return acc;
}
SymbolTree::operator string() {
	return toString(1);
}

// Main semantic analysis functions

void catStdNodes(SymbolTree *&stRoot) {
	*stRoot *= new SymbolTree(KIND_STD, "int", stdIntType);
	*stRoot *= new SymbolTree(KIND_STD, "float", stdFloatType);
	*stRoot *= new SymbolTree(KIND_STD, "bool", stdBoolType);
	*stRoot *= new SymbolTree(KIND_STD, "char", stdCharType);
	*stRoot *= new SymbolTree(KIND_STD, "string", stdStringType);
	*stRoot *= new SymbolTree(KIND_STD, "true", stdBoolLitType);
	*stRoot *= new SymbolTree(KIND_STD, "false", stdBoolLitType);
}

void catStdLib(SymbolTree *&stRoot) {
	// system nodes
	// streams
	*stdLib *= new SymbolTree(KIND_STD, "inInt", inIntType);
	*stdLib *= new SymbolTree(KIND_STD, "inFloat", inFloatType);
	*stdLib *= new SymbolTree(KIND_STD, "inChar", inCharType);
	*stdLib *= new SymbolTree(KIND_STD, "inString", inStringType);
	*stdLib *= new SymbolTree(KIND_STD, "out", stringerType);
	*stdLib *= new SymbolTree(KIND_STD, "err", stringerType);
	// control nodes
	*stdLib *= new SymbolTree(KIND_STD, "randInt", new StdType(STD_INT, SUFFIX_LATCH));
	*stdLib *= new SymbolTree(KIND_STD, "delay", new FilterType(new StdType(STD_INT), nullType, SUFFIX_LATCH));
	// standard library
	// generators
	*stdLib *= new SymbolTree(KIND_STD, "gen", new FilterType(new StdType(STD_INT), new StdType(STD_INT, SUFFIX_STREAM, 1), SUFFIX_LATCH));
	// concatenate the library to the root
	*stRoot *= stdLib;
}

void initSemmerGlobals() {
	// build the standard types
	nullType = new StdType(STD_NULL); nullType->referensible = false;
	errType = new ErrorType();
	stdBoolType = new StdType(STD_BOOL); stdBoolType->referensible = false;
	stdIntType = new StdType(STD_INT); stdIntType->referensible = false;
	stdFloatType = new StdType(STD_FLOAT); stdFloatType->referensible = false;
	stdCharType = new StdType(STD_CHAR); stdCharType->referensible = false;
	stdStringType = new StdType(STD_STRING); stdStringType->referensible = false;
	stdBoolLitType = new StdType(STD_BOOL, SUFFIX_LATCH);
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
	// build the in* types
	inIntType = new StdType(STD_INT, SUFFIX_LATCH); inIntType->instantiable = false;
	inFloatType = new StdType(STD_FLOAT, SUFFIX_LATCH); inFloatType->instantiable = false;
	inCharType = new StdType(STD_CHAR, SUFFIX_LATCH); inCharType->instantiable = false;
	inStringType = new StdType(STD_STRING, SUFFIX_LATCH); inStringType->instantiable = false;
	// build the stringerType
	StructorList instructorList;
	StructorList stringerOutstructorList;
	TypeList *stringOutstructorType = new TypeList(stdStringType); stringOutstructorType->referensible = false;
	stringerOutstructorList.add(stringOutstructorType);
	stringerType = new ObjectType(instructorList, stringerOutstructorList, SUFFIX_LIST, 1); stringerType->referensible = false; stringerType->instantiable = false;
	// prepare to build the standard operator types
	TypeList *filterOutstructorType;
	StructorList opOutstructorList;
	// build the boolUnOpType
	filterOutstructorType = new TypeList(new FilterType(stdBoolType, boolLatchType, SUFFIX_LATCH));
	opOutstructorList = stringerOutstructorList; opOutstructorList.add(filterOutstructorType);
	boolUnOpType = new ObjectType(instructorList, opOutstructorList, SUFFIX_LATCH); boolUnOpType->referensible = false;
	// build the intUnOpType
	filterOutstructorType = new TypeList(new FilterType(stdIntType, intLatchType, SUFFIX_LATCH));
	opOutstructorList = stringerOutstructorList; opOutstructorList.add(filterOutstructorType);
	intUnOpType = new ObjectType(instructorList, opOutstructorList, SUFFIX_LATCH); intUnOpType->referensible = false;
	// build the boolBinOpType
	filterOutstructorType = new TypeList(new FilterType(boolPairType, boolLatchType, SUFFIX_LATCH));
	opOutstructorList = stringerOutstructorList; opOutstructorList.add(filterOutstructorType);
	boolBinOpType = new ObjectType(instructorList, opOutstructorList, SUFFIX_LATCH); boolBinOpType->referensible = false;
	// build the intBinOpType
	filterOutstructorType = new TypeList(new FilterType(intPairType, intLatchType, SUFFIX_LATCH));
	opOutstructorList = stringerOutstructorList; opOutstructorList.add(filterOutstructorType);
	intBinOpType = new ObjectType(instructorList, opOutstructorList, SUFFIX_LATCH); intBinOpType->referensible = false;
	// build the floatBinOpType
	filterOutstructorType = new TypeList(new FilterType(floatPairType, floatLatchType, SUFFIX_LATCH));
	opOutstructorList = stringerOutstructorList; opOutstructorList.add(filterOutstructorType);
	floatBinOpType = new ObjectType(instructorList, opOutstructorList, SUFFIX_LATCH); floatBinOpType->referensible = false;
	// build the boolCompOpType
	filterOutstructorType = new TypeList(new FilterType(boolPairType, boolLatchType, SUFFIX_LATCH));
	opOutstructorList = stringerOutstructorList; opOutstructorList.add(filterOutstructorType);
	boolCompOpType = new ObjectType(instructorList, opOutstructorList, SUFFIX_LATCH); boolCompOpType->referensible = false;
	// build the intCompOpType
	filterOutstructorType = new TypeList(new FilterType(intPairType, boolLatchType, SUFFIX_LATCH));
	opOutstructorList = stringerOutstructorList; opOutstructorList.add(filterOutstructorType);
	intCompOpType = new ObjectType(instructorList, opOutstructorList, SUFFIX_LATCH); intCompOpType->referensible = false;
	// build the floatCompOpType
	filterOutstructorType = new TypeList(new FilterType(floatPairType, boolLatchType, SUFFIX_LATCH));
	opOutstructorList = stringerOutstructorList; opOutstructorList.add(filterOutstructorType);
	floatCompOpType = new ObjectType(instructorList, opOutstructorList, SUFFIX_LATCH); floatCompOpType->referensible = false;
	// build the charCompOpType
	filterOutstructorType = new TypeList(new FilterType(charPairType, boolLatchType, SUFFIX_LATCH));
	opOutstructorList = stringerOutstructorList; opOutstructorList.add(filterOutstructorType);
	charCompOpType = new ObjectType(instructorList, opOutstructorList, SUFFIX_LATCH); charCompOpType->referensible = false;
	// build the stringCompOpType
	filterOutstructorType = new TypeList(new FilterType(stringPairType, boolLatchType, SUFFIX_LATCH));
	opOutstructorList = stringerOutstructorList; opOutstructorList.add(filterOutstructorType);
	stringCompOpType = new ObjectType(instructorList, opOutstructorList, SUFFIX_LATCH); stringCompOpType->referensible = false;
	// build the standard library type
	stdLibType = new StdType(STD_STD, SUFFIX_LATCH); stdLibType->referensible = false; stdLibType->instantiable = false;
	// build the standard library node
	stdLib = new SymbolTree(KIND_STD, STANDARD_LIBRARY_STRING, stdLibType);
	// build the nop IRTree
	nopCode = new CodeTree(CATEGORY_NOP);
}

SymbolTree *genDefaultDefs() {
	// generate the root block node
	SymbolTree *stRoot = new SymbolTree(KIND_BLOCK, BLOCK_NODE_STRING);
	// concatenate in the standard types
	catStdNodes(stRoot);
	// concatenate in the standard library
	catStdLib(stRoot);
	// finally, return the generated default symtable
	return stRoot;
}

// recursively extracts the appropriate nodes from the given tree and appropriately populates the passed containers
void buildSt(Tree *tree, SymbolTree *st, vector<SymbolTree *> &importList) {
	// base case
	if (tree == NULL) {
		return;
	}
	// log the current symbol environment in the tree (this pointer will potentially be overridden by a SymbolTree() constructor)
	tree->env = st;
	// recursive cases
	if (*tree == TOKEN_Declaration) { // if it's a Declaration-style node
		Token defToken = tree->child->t; // ID, AT, or DAT
		if (defToken.tokenType != TOKEN_ID || (defToken.s != "null" && defToken.s != "true" && defToken.s != "false")) { // if this isn't a standard literal override, proceed normally
			Tree *dcn = tree->child->next;
			if (*dcn == TOKEN_EQUALS) { // standard static declaration
				// allocate the new declaration node
				SymbolTree *newDef = new SymbolTree(KIND_DECLARATION, tree->child->t.s, tree);
				// ... and link it in
				*st *= newDef;
				// recurse
				buildSt(tree->child, newDef, importList); // child of Declaration
				buildSt(tree->next, st, importList); // right
			} else if (*(tree->child) == TOKEN_AT) { // import-style declaration
				// allocate the new definition node
				Tree *importId = (*(tree->child->next) == TOKEN_ImportIdentifier) ? tree->child->next : tree->child->next->next; // ImportIdentifier
				SymbolTree *newDef = new SymbolTree((*(importId->child) != TOKEN_OpenIdentifier) ? KIND_CLOSED_IMPORT : KIND_OPEN_IMPORT, IMPORT_DECL_STRING, tree);
				// ... and link it in
				*st *= newDef;
				// also, since it's an import declaration, log it to the import list
				importList.push_back(newDef);
				// recurse
				buildSt(tree->child, newDef, importList); // child of Declaration
				buildSt(tree->next, st, importList); // right
			}
		} else { // else if this is a standard literal override, flag an error
			semmerError(defToken.fileIndex,defToken.row,defToken.col,"redefinition of standard literal '"<<defToken.s<<"'");
		}
	} else if (*tree == TOKEN_Block || *tree == TOKEN_Object) { // if it's a block-style node
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
			fakeId += (uintptr_t)tree;
		}
		SymbolTree *blockDef = new SymbolTree(kind, fakeId, tree);
		// latch the new node into the SymbolTree trunk
		*st *= blockDef;
		// recurse
		buildSt(tree->child, blockDef, importList); // child of Block or Object
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Filter || *tree == TOKEN_ExplicitFilter) { // if it's a filter-style node
		// allocate the new filter definition node
		// generate a fake identifier for the filter node from a hash of the Tree node
		string fakeId(FILTER_NODE_STRING);
		fakeId += (uintptr_t)tree;
		SymbolTree *filterDef = new SymbolTree(KIND_FILTER, fakeId, tree);
		// parse out the header's parameter declarations and add them to the st
		Tree *pl = (*(tree->child) == TOKEN_FilterHeader) ? tree->child->child->next : NULL; // RSQUARE, ParamList, RetList, or NULL
		if (pl != NULL && *pl == TOKEN_ParamList) { // if there is a parameter list to process
			for (Tree *param = pl->child; param != NULL; param = (param->next != NULL) ? param->next->next->child : NULL) { // per-param loop
				// allocate the new parameter definition node
				SymbolTree *paramDef = new SymbolTree(KIND_PARAMETER, param->child->next->t.s, param);
				// ... and link it into the filter definition node
				*filterDef *= paramDef;
			}
		} // if there is a parameter list to process
		// latch the new node into the SymbolTree trunk
		*st *= filterDef;
		// recurse
		buildSt(tree->child, filterDef, importList); // child of Filter
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Instructor || *tree == TOKEN_LastInstructor) { // if it's an instructor-style node
		// allocate the new instructor definition node
		// generate a fake identifier for the instructor node from a hash of the Tree node
		string fakeId(INSTRUCTOR_NODE_STRING);
		fakeId += (uintptr_t)tree;
		SymbolTree *consDef = new SymbolTree(KIND_INSTRUCTOR, fakeId, tree);
		// .. and link it in
		*st *= consDef;
		// link in the parameters of this instructor, if any
		Tree *conscn = tree->child->next; // NULL, SEMICOLON, LSQUARE, or NonRetFilterHeader
		if (conscn != NULL && *conscn == TOKEN_NonRetFilterHeader && *(conscn->child->next) == TOKEN_ParamList) { // if there is actually a parameter list on this instructor
			Tree *pl = conscn->child->next; // ParamList
			for (Tree *param = pl->child; param != NULL; param = (param->next != NULL) ? param->next->next->child : NULL) { // per-param loop
				// allocate the new parameter definition node
				SymbolTree *paramDef = new SymbolTree(KIND_PARAMETER, param->child->next->t.s, param);
				// ... and link it into the instructor definition node
				*consDef *= paramDef;
			}
		}
		// recurse
		buildSt(tree->child, consDef, importList); // child of Instructor
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Outstructor) { // if it's an outstructor-style node
		// allocate the new outstructor definition node
		// generate a fake identifier for the outstructor node from a hash of the Tree node
		string fakeId(OUTSTRUCTOR_NODE_STRING);
		fakeId += (uintptr_t)tree;
		SymbolTree *consDef = new SymbolTree(KIND_OUTSTRUCTOR, fakeId, tree);
		// .. and link it in
		*st *= consDef;
		// recurse
		buildSt(tree->child, consDef, importList); // child of Outstructor
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Instantiation && st->kind != KIND_DECLARATION) { // if it's a non-bound (inlined) instantiation-style node
		string fakeId(INSTANTIATION_NODE_STRING);
		fakeId += (uintptr_t)tree;
		// allocate the new instantiation node
		SymbolTree *newDef = new SymbolTree(KIND_INSTANTIATION, fakeId, tree);
		// ... and link it in
		*st *= newDef;
		// recurse
		buildSt(tree->child, newDef, importList); // child of Instantiation
		buildSt(tree->next, st, importList); // right
	} else { // else if it's any other kind of node
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
// second component is whether we passed through constantication for this binding
pair<SymbolTree *, bool> bindId(const string &s, SymbolTree *env, const TypeStatus &inStatus = TypeStatus()) {
	vector<string> id = chopId(s); // chop up the input identifier into its components
	SymbolTree *stRoot = NULL; // the latch point of the binding
	if (id[0] == "..") { // if the identifier begins with a recall
		Type *recallType = inStatus.type;
		if (recallType) { // if there's a recall binding passed in, use a fake SymbolTree node for it
			// generate a fake identifier for the recall binding node from a hash of the recall identifier's Type object
			string fakeId(FAKE_RECALL_NODE_PREFIX);
			fakeId += (unsigned int)inStatus;
			// check if a SymbolTree node with this identifier already exists -- if so, use it
			map<string, SymbolTree *>::const_iterator fakeFind = env->children.find(fakeId);
			if (fakeFind != env->children.end()) { // if we found a match, use it
				stRoot = (*fakeFind).second;
			} else { // else if we didn't find a match, create a new fake latch point to use
				SymbolTree *fakeStNode = new SymbolTree(KIND_FAKE, fakeId);
				// attach the new fake node to the main SymbolTree
				*env *= fakeStNode;
				// accept the new fake node as the latch point
				stRoot = fakeStNode;
			}
		} else { // else if there is no known recall binding, return an error
			return make_pair((SymbolTree *)NULL, false);
		}
	} else { // else if it's a regular identifier
		for (SymbolTree *stCur = env; stCur != NULL; stCur = stCur->parent) { // scan for a latch point for the beginning of the identifier
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
				map<string, SymbolTree *>::const_iterator latchFind = stCur->children.find(id[0]);
				if (latchFind != stCur->children.end()) { // if we've found a latch point in the children
					stRoot = (*latchFind).second;
					break;
				}
			}
		}
	}
	if (stRoot != NULL) { // if we managed to find a latch point, verify the rest of the binding
		bool needsConstantization = false; // whether this identifier needs to be constantized due to going though a constant reference in the chain
		SymbolTree *stCur = stRoot; // the basis under which we're hoping to bind the current sub-identifier (KIND_STD, KIND_DECLARATION, or KIND_PARAMETER)
		for (unsigned int i = 1; i < id.size(); i++) { // for each sub-identifier of the identifier we're trying to find the binding for
			bool success = false;
			Type *stCurType = errType;
			if (stCur->kind == KIND_STD) { // if it's a standard system-level binding, look in the list of children for a match to this sub-identifier
				map<string, SymbolTree *>::const_iterator childFind = stCur->children.find(id[i]);
				if (childFind != stCur->children.end()) { // if there's a match to this sub-identifier, proceed
					if (*(stCur->defSite->status.type) == STD_STD) { // if it's the root std node, just log the child as stCur and continue in the derivation
						stCur = (*childFind).second;
						success = true;
					} else { // else if it's not the root std node, use the subidentifier's type for derivation, as usual
						stCurType = (*childFind).second->defSite->status.type;
					}
				}
			} else if (stCur->kind == KIND_DECLARATION) { // else if it's a Declaration binding, carefully get its type
				Tree *discriminant = stCur->defSite->child->next->next; // TypedStaticTerm, BlankInstantiation, SEMICOLON, ImportIdentifier, or NULL
				if (discriminant != NULL && (*discriminant == TOKEN_TypedStaticTerm || *discriminant == TOKEN_BlankInstantiation)) { // if it's a Declaration with sub-identifiers, derive its type
					stCurType = getStatusDeclaration(stCur->defSite);
				}
			} else if (stCur->kind == KIND_PARAMETER) { // else if it's a Param binding, naively get its type
				stCurType = getStatusType(stCur->defSite->child); // Type
			} else if (stCur->kind == KIND_FAKE) { // else if it's a faked SymbolTree node, get its type from the fake Tree node we created for it
				stCurType = stCur->defSite->status.type;
			}
			if (*stCurType) { // if we managed to derive a type for this SymbolTree node
				// handle some special cases based on the suffix of the type we just derived
				if (stCurType->suffix == SUFFIX_LIST || stCurType->suffix == SUFFIX_STREAM) { // else if it's a list or a stream, flag an error, since we can't traverse down those
					Token curToken = stCur->defSite->t;
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"member access on unmembered identifier '"<<rebuildId(id, i)<<"'");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (identifier type is "<<stCurType<<")");
					stCurType = errType;
				} else if (stCurType->suffix == SUFFIX_ARRAY || stCurType->suffix == SUFFIX_POOL) { // else if it's an array or pool, ensure that we're accessing it using a subscript
					if (id[i] == "[]" || id[i] == "[:]") { // if we're accessing it via a subscript, accept it and proceed deeper into the binding
						// if it's an array type, flag the fact that it must be constantized
						if (stCurType->suffix == SUFFIX_ARRAY) {
							needsConstantization = true;
						}
						// we're about to fake a SymbolTree node for this subscript access
						// but first, check if a SymbolTree node has already been faked for this member
						map<string, SymbolTree *>::const_iterator fakeFind = stCur->children.find(id[i]);
						if (fakeFind != stCur->children.end()) { // if we've already faked a SymbolTree node for this member, accept it and proceed deeper into the binding
							stCur = (*fakeFind).second;
						} else { // else if we haven't yet faked a SymbolTree node for this member, do so now
							Type *mutableStCurType = stCurType;
							if (id[i] == "[]") { // if it's an expression access (as opposed to a range access), decrease the type's depth
								mutableStCurType = mutableStCurType->copy(); // make a mutable copy of the type
								mutableStCurType->decreaseDepth();
							}
							SymbolTree *fakeStNode = new SymbolTree(KIND_FAKE, id[i], mutableStCurType);
							// attach the new fake node to the main SymbolTree
							*stCur *= fakeStNode;
							// accept the new fake node and proceed deeper into the binding
							stCur = fakeStNode;
						}
						success = true; // all of the above branches lead to success
					} else {
						Token curToken = stCur->defSite->t;
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"non-subscript access on identifier '"<<rebuildId(id, i)<<"'");
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (identifier type is "<<stCurType<<")");
						stCurType = errType;
					}
				} else if (stCurType->category == CATEGORY_OBJECTTYPE) { // else if it's an Object constant or latch (the only other category that can have sub-identifiers)
					// if it's a constant type, flag the fact that this identifier will need to be constantized
					if (stCurType->suffix == SUFFIX_CONSTANT) {
						needsConstantization = true;
					}
					// proceed with binding the sub-identifier as normal by trying to find a match in the Object's members
					ObjectType *stCurTypeCast = ((ObjectType *)stCurType);
					MemberList::iterator findIter = stCurTypeCast->memberList.find(id[i]);
					if (findIter != stCurTypeCast->memberList.end()) { // if we managed to find a matching sub-identifier
						if ((*findIter).defSite() != NULL) { // if the member has a real definition site, accept it and proceed deeper into the binding
							stCur = (*findIter).defSite()->env;
						} else { // else if the member has no real definition site, we'll need to fake a SymbolTree node for it
							// but first, check if a SymbolTree node has already been faked for this member
							map<string, SymbolTree *>::const_iterator fakeFindIter = stCur->children.find(id[i]);
							if (fakeFindIter != stCur->children.end()) { // if we've already faked a SymbolTree node for this member, accept it and proceed deeper into the binding
								stCur = (*fakeFindIter).second;
								stCur = (*fakeFindIter).second;
							} else { // else if we haven't yet faked a SymbolTree node for this member, do so now
								SymbolTree *fakeStNode = new SymbolTree(KIND_FAKE, id[i], (*findIter));
								// attach the new fake node to the main SymbolTree
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
				return make_pair((SymbolTree *)NULL, false);
			} // else if we managed to find a binding for this sub-identifier, continue onto trying to bind the next one
		}
		// if we managed to bind all of the sub-identifiers, return the tail of the binding as well as whether we need to post-constantize it
		return make_pair(stCur, needsConstantization);
	} else { // else if we failed to find an initial latch point, return failure
		return make_pair((SymbolTree *)NULL, false);
	}
}

void subImportDecls(vector<SymbolTree *> importList) {
	bool stdExplicitlyImported = false;
	for(;;) { // per-change loop
		// per-import loop
		vector<SymbolTree *> redoList; // the list of imports that we couldn't handle this round and must redo in the next one
		for (vector<SymbolTree *>::const_iterator importIter = importList.begin(); importIter != importList.end(); importIter++) {
			// extract the import path out of the iterator
			Tree *importdcn = (*importIter)->defSite->child->next;
			bool copyImport = (*importdcn == TOKEN_LSQUARE); // whether this is a copy-import
			Tree *importId = copyImport ? importdcn->next : importdcn; // ImportIdentifier
			string importPath = *(importId->child); // NonArrayedIdentifier, ArrayedIdentifier, or OpenIdentifier
			SymbolTree *importParent = (*importIter)->parent;
			// try to find a binding for this import
			SymbolTree *binding = bindId(importPath, *importIter).first;
			if (binding != NULL) { // if we found a valid binding
				// check for the standard library import special case
				if (binding == stdLib) { // if the import binds to the standard library node
					if ((*importIter)->kind == KIND_CLOSED_IMPORT) { // if this is a closed-import of the standard library node
						if (copyImport) { // if it was a copy-import, log the copy-import site as the standard library node (this will flag an error later)
							(*importIter)->copyImportSite = stdLib;
						}
						if (!stdExplicitlyImported) { // if it's the first standard import, flag it as handled and let it slide
							(*importIter)->id = STANDARD_IMPORT_DECL_STRING;
							stdExplicitlyImported = true;
							continue;
						}
					} else /* if ((*importIter)->kind == KIND_OPEN_IMPORT) */ { // else if this is an open-import of the standard library node
						// add in the imported nodes, scanning for conflicts along the way
						bool firstInsert = true;
						for (map<string, SymbolTree *>::const_iterator childIter = binding->children.begin();
								childIter != binding->children.end();
								childIter++) {
							// check for naming conflicts
							map<string, SymbolTree *>::const_iterator conflictFind = importParent->children.find((*childIter).second->id);
							if (conflictFind == importParent->children.end()) { // if there were no member naming conflicts
								if (firstInsert) { // if this is the first insertion, copy in place of the import placeholder node
									if (copyImport) { // if this is a copy-import
										**importIter = SymbolTree(*((*childIter).second), importParent, (*childIter).second); // scope to the local environment
									} else { // else if this is not a copy-import
										**importIter = SymbolTree(*((*childIter).second), (*childIter).second->parent, NULL); // scope to the foreign environment
									}
									firstInsert = false;
								} else { // else if this is not the first insertion, latch in a copy of the child
									SymbolTree *baseChildCopy = new SymbolTree(*((*childIter).second), NULL, (copyImport) ? (*childIter).second : NULL); // build the copy, scoping to NULL for now
									*((*importIter)->parent) *= baseChildCopy; // latch in the copy
									// correct the scope based on whether this is a copy-import or not
									if (copyImport) { // if this is a copy-import
										baseChildCopy->parent = importParent; // scope to the local environment
									} else { // else if this is not a copy-import
										baseChildCopy->parent = (*childIter).second->parent; // scope to the foreign environment
									}
								}
							} // else if there is a member naming conflict, do nothing; the import is overridden by what's already there
						}
					}
				} else { // else if the import doesn't bind to the standard library node, proceed with normal import processing
					string importPathTip = binding->id; // must exist if binding succeeed
					if ((*importIter)->kind == KIND_CLOSED_IMPORT) { // if this is a closed-import
						// check to make sure that this import doesn't cause a binding conflict
						string importPathTip = binding->id; // must exist if binding succeeed
						map<string, SymbolTree *>::const_iterator conflictFind = importParent->children.find(importPathTip);
						if (conflictFind == importParent->children.end()) { // there was no conflict, so just copy the binding in place of the import placeholder node
							if (copyImport) { // if this is a copy-import
								**importIter = SymbolTree(*binding, importParent, binding); // scope to the local environment
							} else { // else if this is not a copy-import
								**importIter = SymbolTree(*binding, binding->parent, NULL); // scope to the foreign environment
							}
						} else { // else if there was a conflict, flag an error
							Token curDefToken = importId->child->t; // child of NonArrayedIdentifier or ArrayedIdentifier
							Token prevDefToken;
							if ((*conflictFind).second->defSite != NULL) { // if there is a definition site for the previous symbol
								prevDefToken = (*conflictFind).second->defSite->t;
							} else { // otherwise, it must be a standard definition, so make up the token as if it was
								prevDefToken.fileIndex = STANDARD_LIBRARY_FILE_INDEX;
								prevDefToken.row = 0;
								prevDefToken.col = 0;
							}
							semmerError(curDefToken.fileIndex,curDefToken.row,curDefToken.col,"name conflict in importing '"<<importPathTip<<"'");
							semmerError(prevDefToken.fileIndex,prevDefToken.row,prevDefToken.col,"-- (conflicting definition was here)");
						}
					} else /* if ((*importIter)->kind == KIND_OPEN_IMPORT) */ { // else if this is an open-import
						// verify that what's being open-imported is actually an object by finding an object-style child in the binding's children
						map<string, SymbolTree *>::const_iterator bindingChildIter;
						for (bindingChildIter = binding->children.begin(); bindingChildIter != binding->children.end(); bindingChildIter++) {
							if ((*bindingChildIter).second->kind == KIND_OBJECT) {
								break;
							}
						}
						if (bindingChildIter != binding->children.end()) { // if we found an object-style child in this open-import's children (it's a valid open-impoprt of an object)
							SymbolTree *bindingBase = (*bindingChildIter).second; // KIND_OBJECT; this node's children are the ones we're going to import in
							// add in the imported nodes, scanning for conflicts along the way
							bool firstInsert = true;
							for (map<string, SymbolTree *>::const_iterator bindingBaseIter = bindingBase->children.begin();
									bindingBaseIter != bindingBase->children.end();
									bindingBaseIter++) {
								// check for member naming conflicts (constructor type conflicts will be resolved later)
								map<string, SymbolTree *>::const_iterator conflictFind = importParent->children.find((*bindingBaseIter).second->id);
								if (conflictFind == importParent->children.end()) { // if there were no member naming conflicts
									if (firstInsert) { // if this is the first insertion, copy in place of the import placeholder node
										if (copyImport) { // if this is a copy-import
											**importIter = SymbolTree(*((*bindingBaseIter).second), importParent, (*bindingBaseIter).second); // scope to the local environment
										} else { // else if this is not a copy-import
											**importIter = SymbolTree(*((*bindingBaseIter).second), (*bindingBaseIter).second->parent, NULL); // scope to the foreign environment
										}
										firstInsert = false;
									} else { // else if this is not the first insertion, latch in a copy of the child
										SymbolTree *baseChildCopy = new SymbolTree(*((*bindingBaseIter).second), NULL, (copyImport) ? (*bindingBaseIter).second : NULL); // build the copy, scoping to NULL for now
										*((*importIter)->parent) *= baseChildCopy; // latch in the copy
										// correct the scope based on whether this is a copy-import or not
										if (copyImport) { // if this is a copy-import
											baseChildCopy->parent = importParent; // scope to the local environment
										} else { // else if this is not a copy-import
											baseChildCopy->parent = (*bindingBaseIter).second->parent; // scope to the foreign environment
										}
									}
								} // else if there is a member naming conflict, do nothing; the import is overridden by what's already there
							}
						} else { // else if we didn't find an object-style child in this open-import's children (it's an open-impoprt of a non-object), flag an error
							Token curDefToken = importId->child->t; // child of NonArrayedIdentifier or ArrayedIdentifier
							Token prevDefToken;
							if (binding->defSite != NULL) { // if there is a definition site for the previous symbol
								prevDefToken = binding->defSite->t;
							} else { // otherwise, it must be a standard definition, so make up the token as if it was
								prevDefToken.fileIndex = STANDARD_LIBRARY_FILE_INDEX;
								prevDefToken.row = 0;
								prevDefToken.col = 0;
							}
							semmerError(curDefToken.fileIndex,curDefToken.row,curDefToken.col,"open import on non-object '"<<importPathTip<<"'");
							semmerError(prevDefToken.fileIndex,prevDefToken.row,prevDefToken.col,"-- (importing from here)");
						}
					}
				}
			} else { // else if no binding could be found
				redoList.push_back(*importIter); // log the failed node for rebinding during the next round
			}
		} // per-import loop
		if (redoList.size() == importList.size()) { // if the import table has stabilized
			for (vector<SymbolTree *>::const_iterator importIter = redoList.begin(); importIter != redoList.end(); importIter++) {
				Token curToken = (*importIter)->defSite->t;
				Tree *importdcn = (*importIter)->defSite->child->next;
				Tree *importId = (*importdcn == TOKEN_NonArrayedIdentifier || *importdcn == TOKEN_ArrayedIdentifier) ?
					(*importIter)->defSite->child->next :
					(*importIter)->defSite->child->next->next; // NonArrayedIdentifier or ArrayedIdentifier
				string importPath = *importId; // NonArrayedIdentifier or ArrayedIdentifier
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"cannot resolve import '"<<importPath<<"'");
			}
			break;
		} else { // else if the import table hasn't stabilized yet, do another substitution round on the failed binding list
			importList = redoList;
		}
	} // per-change loop
}

// recursively derives the Type trees and offsets of all non-inlined semantic-impacting nodes in the passed-in SymbolTree
void semSt(SymbolTree *root, SymbolTree *parent = NULL) {
	if (root->kind == KIND_DECLARATION || root->kind == KIND_INSTRUCTOR || root->kind == KIND_OUTSTRUCTOR) { // if it's a non-inlined node, derive its type
		getStatusSymbolTree(root, parent);
	}
	// recurse on this node's children
	for (map<string, SymbolTree *>::const_iterator iter = root->children.begin(); iter != root->children.end(); iter++) {
		semSt((*iter).second, root);
	}
}

// reports errors; derives the status of this SymbolTree node, as well as deriving its subnode offset properties
TypeStatus getStatusSymbolTree(SymbolTree *root, SymbolTree *parent, const TypeStatus &inStatus) {
	GET_STATUS_SYMBOL_TREE_HEADER;
	if (root->kind == KIND_DECLARATION) { // if the symbol was defined as a Declaration-style node
		returnStatus(getStatusDeclaration(tree));
	} else if (root->kind == KIND_PARAMETER) { // else if the symbol was defined as a Param-style node
		returnStatus(getStatusType(tree->child, inStatus)); // Type
	} else if (root->kind == KIND_INSTRUCTOR) { // else if the symbol was defined as an instructor-style node
		returnStatus(getStatusInstructor(tree, inStatus)); // Instructor
	} else if (root->kind == KIND_OUTSTRUCTOR) { // else if the symbol was defined as an outstructor-style node
		returnStatus(getStatusOutstructor(tree, inStatus)); // OutStructor
	} else if (root->kind == KIND_INSTANTIATION) { // else if the symbol was defined as an instantiation-style node
		returnStatus(getStatusInstantiation(tree, inStatus)); // Instantiation
	} else if (root->kind == KIND_FAKE) { // else if the symbol was fake-defined as part of bindId()
		return (tree->status);
	}
	GET_STATUS_CODE;
	// ensure that this isn't a copy-import of a non-referensible node
	if (root->copyImportSite != NULL && !(root->defSite->status.type->referensible)) { // if it's a copy-import of a non-referensible type, flag an error
		Token curToken = root->defSite->t;
		Token sourceToken = root->copyImportSite->defSite->t;
		semmerError(curToken.fileIndex,curToken.row,curToken.col,"copy import of non-referensible identifier '"<<root->copyImportSite->id<<"'");
	}
	// generate the intermediate code tree
	if (root->kind == KIND_DECLARATION || root->kind == KIND_INSTRUCTOR || root->kind == KIND_OUTSTRUCTOR || root->kind == KIND_INSTANTIATION) {
		returnCode(tree->code());
	} else if (root->kind == KIND_PARAMETER || root->kind == KIND_FAKE) {
		returnCode(nopCode);
	}
	GET_STATUS_FOOTER;
}

// typing function definitions

// reports errors
TypeStatus getStatusIdentifier(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	string id = *tree; // string representation of this identifier
	pair<SymbolTree *, bool> binding = bindId(id, tree->env, inStatus);
	SymbolTree *st = binding.first;
	if (st != NULL) { // if we found a binding
		TypeStatus stStatus = getStatusSymbolTree(st, st->parent, inStatus);
		if (*stStatus) { // if we successfully extracted a type for this SymbolTree entry
			Type *mutableStType = stStatus;
			if (binding.second) { // do the upstream-mandated constantization if needed
				mutableStType = mutableStType->copy();
				mutableStType->constantize();
			}
			returnType(mutableStType);
		}
	} else { // else if we couldn't find a binding
		Token curToken = tree->t;
		semmerError(curToken.fileIndex,curToken.row,curToken.col,"cannot resolve '"<<id<<"'");
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
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"delatch of incompatible type");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (type is "<<subStatus<<")");
					mutableSubStatus.type->erase();
				}
			} else { // else if the derived type isn't a latch or stream (and thus can't be delatched), error
				Token curToken = pbc->t;
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"delatch of non-latch, non-stream '"<<subSI<<"'");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (type is "<<inStatus<<")");
			}
		}
	} else if (*pbc == TOKEN_Instantiation) {
		returnStatus(getStatusInstantiation(pbc, inStatus));
	} else if (*pbc == TOKEN_ExplicitFilter) {
		getStatusFilter(pbc, inStatus); // blindly generates a thunk; never fails
		returnStatus(verifyStatusFilter(pbc)); // return the status resulting from verifying the contents of this filter
	} else if (*pbc == TOKEN_Object) {
		getStatusObject(pbc, inStatus); // blindly generates a thunk; never fails
		returnStatus(verifyStatusObject(pbc)); // return the status resulting from verifying the contents of this object
	} else if (*pbc == TOKEN_PrimLiteral) {
		returnStatus(getStatusPrimLiteral(pbc, inStatus));
	} else if (*pbc == TOKEN_BracketedExp) {
		returnStatus(getStatusBracketedExp(pbc, inStatus));
	} else if (*pbc == TOKEN_PrimaryBase) { // postfix operator application
		TypeStatus baseStatus = getStatusPrimaryBase(pbc, inStatus); // derive the status of the base node
		if (*baseStatus) { // if we managed to derive the status of the base node
			if (*baseStatus >> *stdIntType) { // if the base can be converted into an int, return int
				returnType(new StdType(STD_INT, SUFFIX_LATCH));
			} else { // else if we couldn't apply the operator to the type of the subnode, flag an error
				Token curToken = pbc->next->child->t; // the actual operator token
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"postfix operation '"<<curToken.s<<"' on invalid type");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (type is "<<baseStatus<<")");
			}
		}
	}
	GET_STATUS_CODE;
	if (*pbc == TOKEN_NonArrayedIdentifier || *pbc == TOKEN_ArrayedIdentifier ||
			*pbc == TOKEN_Instantiation || *pbc == TOKEN_ExplicitFilter || *pbc == TOKEN_Object || *pbc == TOKEN_PrimLiteral || *pbc == TOKEN_BracketedExp) {
		returnCode(pbc->code());
	} else if (*pbc == TOKEN_SingleAccessor) { // if it's an accessed term
		// first, derive the subtype
		Tree *subSI = pbc->next; // NonArrayedIdentifier or ArrayedIdentifier
		TypeStatus subStatus = getStatusIdentifier(subSI, inStatus); // NonArrayedIdentifier or ArrayedIdentifier
		if (*subStatus) { // if we successfully derived a subtype
			if ((*subStatus).suffix == SUFFIX_LATCH) { // if we're delatching from a latch
				// LOL delatching-from-latch code
			} else if ((*subStatus).suffix == SUFFIX_STREAM) { // else if we're delatching from a stream
				// LOL delatching-from-stream code
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
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"prefix operation '"<<curToken.s<<"' on invalid type");
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (type is "<<subStatus<<")");
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

TypeStatus getStatusBracketedExp(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *becn = tree->child->next; // RBRACKET or ExpList
	if (*becn == TOKEN_RBRACKET) {
		returnTypeRet(nullType, inStatus.retType);
	} else /* if (*becn == TOKEN_ExpList) */ {
		Tree *exp = becn->child;
		if (exp->next == NULL) { // if it's just a single Exp
			returnStatus(getStatusExp(exp, inStatus));
		} else { // else if it's a true ExpList, we must derive the corresponding TypeList
			bool failed = false;
			vector<Type *> list;
			for (; exp != NULL; exp = (exp->next != NULL) ? exp->next->next->child : NULL) {
				Type *thisExpType = getStatusExp(exp, inStatus);
				list.push_back(thisExpType);
				if (!(*thisExpType)) {
					failed = true;
				}
			}
			if (!failed) {
				returnTypeRet(new TypeList(list), inStatus.retType);
			}
		}
	}
	GET_STATUS_CODE;
	if (*becn == TOKEN_RBRACKET) {
		returnCode(nopCode);
	} else /* if (*becn == TOKEN_ExpList) */ {
		Tree *exp = becn->child;
		if (exp->next == NULL) { // if it's just a single Exp
			returnCode(exp->code());
		} else { // else if it's a true ExpList
			vector<DataTree *> dataList;
			for (; exp != NULL; exp = (exp->next != NULL) ? exp->next->next->child : NULL) {
				dataList.push_back((DataTree *)(exp->code()));
			}
			returnCode(new CompoundTree(dataList));
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
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"left operand of expression is not a constant or latch");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (operand type is "<<left<<")");
			} else if (!(right->suffix == SUFFIX_CONSTANT || right->suffix == SUFFIX_LATCH)) {
				Token curToken = expRight->t; // Exp
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"right operand of expression is not a constant or latch");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (operand type is "<<right<<")");
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
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"infix operation '"<<curToken.s<<"' on invalid operands");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (left operand type is "<<left<<")");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (right operand type is "<<right<<")");
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
		if (*pipec == TOKEN_Declaration) { // if it's a declaration-style Pipe, ignore the returned retType (it's used for recursion detection instead)
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
	TypeStatus from;
	TypeStatus to;
	Tree *treeCur = tree->child->next; // ParamList or RetList
	bool failed = false;
	if (*treeCur == TOKEN_ParamList) {
		from = getStatusParamList(treeCur, inStatus);
		failed = !(*from);
		// advance to handle the possible RetList
		treeCur = treeCur->next; // RetList or RSQUARE
	} else {
		from = TypeStatus(nullType, inStatus);
	}
	if (*treeCur == TOKEN_RetList) { // if this is a potentially implicity defined to-list
		if (*(treeCur->child->next) != TOKEN_QUESTION) { // if this is an explicitly-defined to-list
			to = getStatusTypeList(treeCur->child->next, inStatus); // TypeList
			failed = (failed || !(*to));
		} else { // else if this is an implicitly defined to-list
			to.type = NULL;
		}
	} else if (*treeCur == TOKEN_ExplicitRetList) { // else if this is an explicitly defined to-list
		to = getStatusTypeList(treeCur->child->next, inStatus); // TypeList
		failed = (failed || !(*to));
	} else {
		to = TypeStatus(nullType, inStatus);
	}
	if (!failed) { // if we succeeded in deriving both the from- and to- statuses
		returnType(new FilterType(from.type, to.type, SUFFIX_LATCH));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors; assumes that the corresponding Filter thunk was generated successfully
TypeStatus verifyStatusFilter(Tree *tree) {
	FilterType *headerType = (FilterType *)(tree->status.type);
	if (*(headerType->from())) { // if the header from-type evaluates to a valid type
		Tree *block;
		TypeStatus startStatus; // the status that we're going to feed into the Block subnode derivation
		startStatus.retType = NULL; // make no initial presuppositions about what type the Block should return
		if (*(tree->child) == TOKEN_Block) { // if this is an implicitly block-defined filter
			block = tree->child; // Block
			// set the type to feed into the block derivation to be the one coming in to this filter
			startStatus = headerType->from();
		} else /* if (*(tree->child->next) == TOKEN_Block) */ { // else if this is an explicitly header-defined filter
			block = tree->child->next; // Block
			// nullify the type to feed into the block derivation, since we have an explicit parameter list
			startStatus = nullType;
		}
		TypeStatus blockStatus = getStatusBlock(block, startStatus); // derive the definition Block's Type
		if (*blockStatus) { // if we successfully verified the definition Block (meaning there were no internal return type inconsistencies)
			if (headerType->to() == NULL) { // if the header's to-type was implicit, update it to be whatever the block returned
				headerType->toInternal = ((FilterType *)(blockStatus.type))->to()->wrapTypeList();
				returnTypeRet(headerType, NULL);
			} else if ((*(((FilterType *)(blockStatus.type))->to()) == *nullType && *(headerType->to()) == *nullType) ||
					(*(((FilterType *)(blockStatus.type))->to()) >> *(headerType->to()))) { // if the header and Block return types are compatible
				returnTypeRet(headerType, NULL);
			} else { // else if the header and Block don't match
				Token curToken = block->child->t; // LCURLY
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"block returns unexpected type "<<((FilterType *)(blockStatus.type))->to());
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (expected type is "<<headerType->to()<<")");
			}
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// generates a thunk; does not actually generate any code
TypeStatus getStatusFilter(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// derive the declared type of the filter
	Tree *filterc = tree->child; // Block or FilterHeader
	if (*filterc == TOKEN_Block) { // if it's an implicit block-defined filter, return its type as a consumer of the input type
		returnTypeRet(new FilterType(inStatus.type, nullType, SUFFIX_LATCH), NULL);
	} else /* if (*filterc == TOKEN_FilterHeader) */ { // else if it's an explicit header-defined filter, return the filter header's definition site in the FilterType thunk
		returnTypeRet(new FilterType(filterc, inStatus.type, SUFFIX_LATCH), NULL);
	}
	GET_STATUS_CODE;
	// KOL need to derive the offset here
	GET_STATUS_FOOTER;
}

// generates a thunk; does not actually generate any code
TypeStatus getStatusInstructor(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *icn = tree->child->next; // NULL, SEMICOLON, Block, or NonRetFilterHeader
	if (icn == NULL || *icn == TOKEN_SEMICOLON || *icn == TOKEN_Block) {
		returnTypeRet(new FilterType(nullType, nullType, SUFFIX_LATCH), NULL);
	} else /* if (*icn == TOKEN_NonRetFilterHeader) */ {
		returnTypeRet(new FilterType(icn, nullType, SUFFIX_LATCH), NULL);
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// assumes that the corresponding Filter thunk was generated successfully
TypeStatus verifyStatusInstructor(Tree *tree) {
	FilterType *headerType = (FilterType *)(tree->status.type);
	if (*(headerType->from())) { // if the header evaluates to a valid type
		Tree *block;
		if (tree->child->next != NULL) {
			if (*(tree->child->next) == TOKEN_Block) {
				block = tree->child->next;
			} else {
				block = tree->child->next->next;
			}
		} else {
			block = NULL;
		}
		if (block != NULL) { // if there's actually an explicit definition block to verify
			TypeStatus startStatus(nullType, errType); // set retType = errType to make sure that the instructor doesn't return anything
			TypeStatus verifiedStatus = getStatusBlock(block, startStatus);
			if (*verifiedStatus) { // if we successfully verified this instructor (meaning there were no internal return type inconsistencies)
				returnTypeRet(headerType, NULL);
			}
		} else { // else if there is no explicit definition block to verify, simply return the previously derived status
			returnStatus(tree->status);
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// generates a thunk; does not actually generate any code
TypeStatus getStatusOutstructor(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	returnTypeRet(new FilterType(tree->child->next, nullType, SUFFIX_LATCH), NULL); // RetFilterHeader
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors; assumes that the corresponding Filter thunk was generated successfully
TypeStatus verifyStatusOutstructor(Tree *tree) {
	FilterType *headerType = (FilterType *)(tree->status.type);
	if (*(headerType->from())) { // if the header from-type evaluates to a valid type
		Tree *block = tree->child->next->next; // NULL, RSQUARE, or Block
		if (block != NULL && *block == TOKEN_Block) { // if there's an explicit block to verify for this Outstructor
			TypeStatus startStatus(nullType, NULL); // set retType = NULL to allow the oustructor to return any type (for now) 
			TypeStatus verifiedStatus = getStatusBlock(block, startStatus);
			if (*verifiedStatus) { // if we successfully verified this outstructor (meaning there were no internal return type inconsistencies)
				if (headerType->to() == NULL) { // if the header's to-type was implicit, update it to be whatever the block returned
					headerType->toInternal = ((FilterType *)(verifiedStatus.type))->to()->wrapTypeList();
					returnTypeRet(headerType, NULL);
				} else if (*(((FilterType *)(verifiedStatus.type))->to()) >> *(headerType->to())) { // else if the return types are compatible, log the header's to-type as the return status
					returnTypeRet(headerType, NULL);
				} else { // if the return types are not compatible, flag an error
					Token curToken = block->child->t; // LCURLY
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"outstructor returns unexpected type "<<((FilterType *)(verifiedStatus.type))->to());
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (expected type is "<<headerType->to()<<")");
				}
			}
		} else { // else if there is no explicit block to verify for this Outstructor (it's an implicitly null outstructor)
			returnStatus(tree->status);
		}
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// generates a thunk; does not actually generate any code
TypeStatus getStatusObject(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// log the definition sites of all intructors, outstructors, and members
	StructorList instructorList;
	StructorList outstructorList;
	MemberList memberList;
	SymbolTree *objectSt = tree->env;
	for (map<string, SymbolTree *>::const_iterator memberIter = objectSt->children.begin(); memberIter != objectSt->children.end(); memberIter++) {
		if ((*memberIter).second->kind == KIND_INSTRUCTOR) { // if it's an instructor-style node
			instructorList.add((*memberIter).second->defSite); // Instructor
		} else if ((*memberIter).second->kind == KIND_OUTSTRUCTOR) { // else if it's an outstructor-style node
			outstructorList.add((*memberIter).second->defSite); // Outstructor
		} else if ((*memberIter).second->kind == KIND_DECLARATION) { // else if it's a declaration-style node
			memberList.add((*memberIter).second->id, (*memberIter).second->defSite);
		} else if ((*memberIter).second->kind == KIND_STD) { // else if it's an imported standard node
			memberList.add((*memberIter).second->id, (*memberIter).second->defSite->status.type);
		}
	}
	// return a thunk representing this ObjectType
	returnTypeRet(new ObjectType(instructorList, outstructorList, memberList, SUFFIX_LATCH), NULL);
	GET_STATUS_CODE;
	// KOL need to derive the offset here
	GET_STATUS_FOOTER;
}

// reports errors; assumes that the corresponding Object thunk was generated successfully
TypeStatus verifyStatusObject(Tree *tree) {
	// overview:
	// derive types for instructors and outstructors from headers
	// verify the definitions of all instructors and outstructors, including filling in the return types of implicit outstructors
	// making sure that there are no type conflicts in instructors and outstructors
	// derive types for and verify all members
	// verify all remaining raw pipes in this Object definition
	ObjectType *objectType = (ObjectType *)(tree->status.type);
	// derive types for all instructors and outstructors from their headers and validate that there were no errors in doing so
	bool failed = !(objectType->instructorList.reify() && objectType->outstructorList.reify());
	// verify the definitions of all instructors and outstructors (including filling in the return types of implicit outstructors)
	for (StructorList::iterator iter = objectType->instructorList.begin(); iter != objectType->instructorList.end(); iter++) {
		TypeStatus verifiedStatus = verifyStatusInstructor((*iter).defSite()); // Instructor
		if (!(*verifiedStatus)) { // if the verification failed, flag this fact
			failed = true;
		}
	}
	for (StructorList::iterator iter = objectType->outstructorList.begin(); iter != objectType->outstructorList.end(); iter++) {
		TypeStatus verifiedStatus = verifyStatusOutstructor((*iter).defSite()); // Outstructor
		if (!(*verifiedStatus)) { // if the verification failed, flag this fact
			failed = true;
		}
	}
	// make sure that there are no type conflicts in instuctors and outstructors
	for (StructorList::iterator iter1 = objectType->instructorList.begin(); iter1 != objectType->instructorList.end(); iter1++) {
		Type *insType1 = *iter1;
		if (*insType1) { // if we managed to derive a type for this instructor, check that it doesn't conflict with any other instructor
			for (StructorList::iterator iter2 = objectType->instructorList.begin(); iter2 != objectType->instructorList.end(); iter2++) {
				if (iter2 != iter1) { // if this isn't the same instructor as in the outer loop
					Type *insType2 = *iter2;
					if (*insType2 && *insType2 == *insType1) {
						Token curDefToken = (*iter1).defSite()->t; // Instructor
						Token prevDefToken = (*iter2).defSite()->t; // Instructor
						semmerError(curDefToken.fileIndex,curDefToken.row,curDefToken.col,"duplicate instructor of type "<<insType1);
						semmerError(prevDefToken.fileIndex,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
						failed = true;
						break;
					}
				}
			}
		} else { // else if we failed to derive a type for this instructor
			failed = true;
		}
	}
	for (StructorList::iterator iter1 = objectType->outstructorList.begin(); iter1 != objectType->outstructorList.end(); iter1++) {
		Type *outsType1 = *iter1;
		if (*outsType1) { // if we managed to derive a type for this outstructor, check that it doesn't conflict with any other outstructor
			for (StructorList::iterator iter2 = objectType->outstructorList.begin(); iter2 != objectType->outstructorList.end(); iter2++) {
				if (iter2 != iter1) { // if this isn't the same outstructor as in the outer loop
					Type *outsType2 = *iter2;
					if (*outsType2 && *outsType2 == *outsType1) {
						Token curDefToken = (*iter1).defSite()->t; // Outstructor
						Token prevDefToken = (*iter2).defSite()->t; // Outstructor
						semmerError(curDefToken.fileIndex,curDefToken.row,curDefToken.col,"duplicate outstructor of type "<<outsType1);
						semmerError(prevDefToken.fileIndex,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
						failed = true;
						break;
					}
				}
			}
		} else { // else if we failed to derive a type for this outstructor
			failed = true;
		}
	}
	// derive the types and verify the definitions of all members
	for (MemberList::iterator iter = objectType->memberList.begin(); iter != objectType->memberList.end(); iter++) {
		Type *memberType = *iter;
		if (!(*memberType)) { // if we failed to derive the type of this member, flag this fact
			failed = true;
		}
	}
	// verify all remaining regular pipes in this Object definition
	for (Tree *pipe = tree->child->next->next->next->child; pipe != NULL; pipe = (pipe->next != NULL) ? pipe->next->child : NULL) { // pipe is child of InstructedObjectPipes or ObjectPipes
		if (*pipe == TOKEN_Pipe || *pipe == TOKEN_LastPipe) {
			Tree *pipec = pipe->child;
			if (*pipec != TOKEN_Declaration) { // if it's not a declaration-style Pipe, derive and validate its status
				TypeStatus pipeStatus = getStatusPipe(pipe);
				if (!(*pipeStatus)) { // if we failed to derive a type for this Pipe, flag an error
					failed = true;
				}
			}
		}
	}
	// finally, do the verified return
	if (!failed) { // if we successfully verified everything, return the originally derived status
		returnStatus(tree->status);
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

TypeStatus getStatusType(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *typeSuffix = tree->child->next; // TypeSuffix
	// derive the suffix and depth first, since we'll need to know them in order to construct the Type object
	bool failed = false;
	int suffixVal;
	int depthVal = 0;
	Tree *offsetExp = NULL;
	if (typeSuffix == NULL) {
		suffixVal = SUFFIX_CONSTANT;
	} else if (*(typeSuffix->child) == TOKEN_SLASH) {
		suffixVal = SUFFIX_LATCH;
	} else if (*(typeSuffix->child) == TOKEN_ListTypeSuffix) {
		suffixVal = SUFFIX_LIST;
		for (Tree *lts = typeSuffix->child; lts != NULL; lts = (lts->child->next->next != NULL) ? lts->child->next->next : NULL) { // ListTypeSuffix
			depthVal++;
		}
	} else if (*(typeSuffix->child) == TOKEN_StreamTypeSuffix) {
		suffixVal = SUFFIX_STREAM;
		for (Tree *sts = typeSuffix->child; sts != NULL; sts = (sts->child->next != NULL) ? sts->child->next : NULL) { // StreamTypeSuffix
			depthVal++;
		}
	} else if (*(typeSuffix->child) == TOKEN_ArrayTypeSuffix) {
		suffixVal = SUFFIX_ARRAY;
		for (Tree *exp = typeSuffix->child->child->next; exp != NULL; exp = (exp->next->next != NULL) ? exp->next->next->child->next : NULL) { // Exp
			depthVal++;
			// validate that this suffix expression is valid
			TypeStatus expStatus = getStatusExp(exp, inStatus); // Exp
			if (!(*expStatus >> *stdIntType)) { // if the expression is incompatible with an integer, flag a bad expression error
				Token curToken = exp->t; // Exp
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"array subscript is invalid");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (subscript type is "<<expStatus<<")");
				failed = true;
			}
		}
	} else /* if (*(typeSuffix->child) == TOKEN_PoolTypeSuffix) */ {
		suffixVal = SUFFIX_POOL;
		offsetExp = typeSuffix->child->child->next; // LSQUARE
		for(Tree *exp = offsetExp->next; exp != NULL; exp = (exp->next->next != NULL) ? exp->next->next->child->next : NULL) { // Exp
			depthVal++;
			// validate that this suffix expression is valid
			TypeStatus expStatus = getStatusExp(exp, inStatus); // Exp
			if (!(*expStatus >> *stdIntType)) { // if the expression is incompatible with an integer, flag a bad expression error
				Token curToken = exp->t; // Exp
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"pool subscript is invalid");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (subscript type is "<<expStatus<<")");
				failed = true;
			}
		}
	}
	if (!failed) { // if all of the suffixes were valid
		Tree *typec = tree->child; // NonArrayedIdentifier, FilterType, or ObjectType
		if (*typec == TOKEN_NonArrayedIdentifier || *typec == TOKEN_ArrayedIdentifier) { // if it's an identifier-defined type
			TypeStatus idStatus = getStatusIdentifier(typec, inStatus); // NonArrayedIdentifier
			if (*idStatus) { // if we managed to derive a type for the instantiation identifier
				if (idStatus.type != stdBoolLitType) { // if the type isn't defined by a standard literal
					idStatus.type = idStatus.type->copy(); // make a copy of the identifier's type, so that the below mutation doesn't propagate to it
					idStatus->suffix = suffixVal;
					idStatus->depth = depthVal;
					idStatus->offsetExp = offsetExp;
					returnStatus(idStatus);
				} else { // else if the type is defined by a standard literal, flag an error
					Token curToken = typec->child->t; // guaranteed to be ID, since only NonArrayedIdentifier or ArrayedIdentifier nodes generate inoperable types
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"standard literal '"<<typec<<"' is not a type");
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
			if (*sub == TOKEN_ExplicitRetList) { // if there is a to-list
				to = getStatusTypeList(sub->child->next, inStatus); // TypeList
			}
			returnType(new FilterType(from, to, suffixVal, depthVal, offsetExp));
		} else if (*typec == TOKEN_ObjectType) { // else if it's an in-place-defined object type
			Tree *otcn = typec->child->next; // RCURLY or ObjectTypeList
			if (*otcn == TOKEN_RCURLY) { // if it's a blank object type
				returnType(new ObjectType(suffixVal, depthVal, offsetExp));
			} else /* if (*otcn == TOKEN_ObjectTypeList) */ { // else if it's a custom-defined object type
				StructorList instructorList;
				vector<Token> instructorTokens;
				StructorList outstructorList;
				vector<Token> outstructorTokens;
				MemberList memberList;
				vector<Token> memberTokens;
				bool failed = false;
				for(Tree *cur = otcn->child; cur != NULL; cur = (cur->next != NULL) ? cur->next->next->child : NULL) { // invariant: cur is a child of ObjectTypeList
					if (*cur == TOKEN_InstructorType) { // if it's an instructor type
						TypeStatus insStatus;
						if (cur->child->next == NULL) { // if it's an implicitly null instructor, log it as such
							insStatus = TypeStatus(new TypeList(), inStatus);
						} else { // else if it's an explicitly described instructor, get its type from the subnode
							insStatus = getStatusTypeList(cur->child->next->next, inStatus); // TypeList
						}
						if (*insStatus) { // if we successfully derived a type for this instructor
							// check if there's already a instructor of this type
							StructorList::iterator iter1;
							vector<Token>::const_iterator iter2;
							for (iter1 = instructorList.begin(), iter2 = instructorTokens.begin(); iter1 != instructorList.end(); iter1++, iter2++) {
								if (**iter1 == *insStatus) {
									break;
								}
							}
							if (iter1 == instructorList.end()) { // if there were no conflicts, add the instructor's type to the list
								instructorList.add((TypeList *)(insStatus.type));
								instructorTokens.push_back(cur->child->t); // EQUALS
							} else { // otherwise, flag the conflict as an error
								Token curDefToken = cur->child->t; // EQUALS
								Token prevDefToken = *iter2;
								semmerError(curDefToken.fileIndex,curDefToken.row,curDefToken.col,"duplicate instructor of type "<<insStatus);
								semmerError(prevDefToken.fileIndex,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
								failed = true;
							}
						} else { // otherwise, if we failed to derive a type for this instructor
							failed = true;
						}
					} else if (*cur == TOKEN_OutstructorType) { // if it's an outstructor type
						TypeStatus outsStatus = getStatusTypeList(cur->child->next->next->next, inStatus); // TypeList
						if (*outsStatus) { // if we successfully derived a type for this outstructor
							// check if there's already a outstructor of this type
							StructorList::iterator iter1;
							vector<Token>::const_iterator iter2;
							for (iter1 = outstructorList.begin(), iter2 = outstructorTokens.begin(); iter1 != outstructorList.end(); iter1++, iter2++) {
								if (**iter1 == *outsStatus) {
									break;
								}
							}
							if (iter1 == outstructorList.end()) { // if there were no conflicts, add the outstructor's type to the list
								outstructorList.add((TypeList *)(outsStatus.type));
								outstructorTokens.push_back(cur->child->t); // EQUALS
							} else { // otherwise, flag the conflict as an error
								Token curDefToken = cur->child->t; // EQUALS
								Token prevDefToken = *iter2;
								semmerError(curDefToken.fileIndex,curDefToken.row,curDefToken.col,"duplicate outstructor of type "<<outsStatus);
								semmerError(prevDefToken.fileIndex,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
								failed = true;
							}
						} else { // otherwise, if we failed to derive a type for this outstructor
							failed = true;
						}
					} else if (*cur == TOKEN_MemberType) { // else if it's a member type
						// check for naming conflicts with this member
						string &stringToAdd = cur->child->t.s; // ID
						MemberList::iterator iter1;
						vector<Token>::const_iterator iter2;
						for (iter1 = memberList.begin(), iter2 = memberTokens.begin(); iter1 != memberList.end(); iter1++, iter2++) {
							if ((string)(*iter1) == stringToAdd) {
								break;
							}
						}
						if (iter1 == memberList.end()) { // if there were no naming conflicts with this member
							TypeStatus memberStatus = getStatusType(cur->child->next->next, inStatus); // Type
							if (*memberStatus) { // if we successfully derived a type for this Declaration
								memberList.add(stringToAdd, memberStatus.type);
								memberTokens.push_back(cur->child->t); // ID
							} else { // else if we failed to derive a type
								failed = true;
							}
						} else { // else if there was a naming conflict with this member
							Token curDefToken = cur->child->t;
							Token prevDefToken = *iter2;
							semmerError(curDefToken.fileIndex,curDefToken.row,curDefToken.col,"duplicate declaration of object type member '"<<stringToAdd<<"'");
							semmerError(prevDefToken.fileIndex,prevDefToken.row,prevDefToken.col,"-- (previous declaration was here)");
							failed = true;
						}
					}
				}
				if (!failed) {
					returnType(new ObjectType(instructorList, outstructorList, memberList, suffixVal, depthVal, offsetExp));
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
	for (Tree *cur = tree->child; cur != NULL; cur = (cur->next != NULL) ? cur->next->next->child : NULL) { // invariant: cur is a Type
		TypeStatus curTypeStatus = getStatusType(cur, inStatus);
		if (*curTypeStatus) { // if we successfully derived a type for this node
			if (curTypeStatus.type->instantiable) { // if the derived type is instantiable
				list.push_back(curTypeStatus.type); // commit the type to the list
			} else { // else if the derived type is not instantiable, flag an error
				Token curToken = cur->t; // Type
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"parameterized non-instantiable node '"<<cur->child<<"'"); // NonArrayedIdentifier
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (parameter type is "<<curTypeStatus<<")");
				failed = true;
			}
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

TypeStatus getStatusParamList(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	vector<Type *> list;
	bool failed = false;
	for (Tree *cur = tree->child; cur != NULL; cur = (cur->next != NULL) ? cur->next->next->child : NULL) { // invariant: cur is a Param
		if (*(cur->child) == TOKEN_Type) { // if it's a regularly typed parameter
			TypeStatus paramStatus = getStatusType(cur->child, inStatus); // Type
			list.push_back(paramStatus.type->copy()); // commit the type to the list
			if (*paramStatus) { // if we successfully derived a type for this node
				if (!(paramStatus.type->instantiable)) { // if the derived type is not instantiable, flag an error
					Token curToken = cur->t; // Param
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"parameterized non-instantiable node '"<<cur->child->child<<"'"); // NonArrayedIdentifier
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (parameter type is "<<paramStatus<<")");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (parameter identifier is '"<<cur->child->next->t.s<<"')");
					failed = true;
				}
			} else { // else if we failed to derive a type for this node
				failed = true;
			}
		} else /* if (*(cur->child) == TOKEN_QUESTION) */ { // else if it's an automatically typed parameter
			if (inStatus->category == CATEGORY_TYPELIST) { // if the incoming type is a proper type list
				if (((TypeList *)(inStatus.type))->list.size() > list.size()) { // if the incoming type list is long enough to contain a type for this parameter, allow the derivation
					list.push_back((((TypeList *)(inStatus.type))->list[list.size()])->copy());
				} else { // else if the incoming type list is too short, flag an error
					list.push_back(errType);
					Token curToken = cur->t; // Param
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"auto-typed parameter in list with not enough incoming values"); // NonArrayedIdentifier
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (incoming type is "<<inStatus<<")");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (parameter identifier is '"<<cur->child->next->t.s<<"')");
					failed = true;
				}
			} else if (*inStatus != *nullType) { // else if there is a valid single non-null incoming type
				if (list.size() == 0) { // if this is the first parameter in the list, allow the derivation
					list.push_back(inStatus.type->copy());
				} else { // else if this a subsequent parameter in the list, there are too many parameters
					list.push_back(errType);
					Token curToken = cur->t; // Param
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"auto-typed parameter in list with single incoming value"); // NonArrayedIdentifier
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (incoming type is "<<inStatus<<")");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (parameter identifier is '"<<cur->child->next->t.s<<"')");
					failed = true;
				}
			} else { // else if the incoming type is null, flag an error
				list.push_back(errType);
				Token curToken = cur->t; // Param
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"auto-typed parameter with no incoming value"); // NonArrayedIdentifier
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (incoming type is "<<inStatus<<")");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (parameter identifier is '"<<cur->child->next->t.s<<"')");
				failed = true;
				failed = true;
			}
		}
		// log the parameter's type in the Param tree node
		cur->status.type = list.back();
	}
	if (!failed) { // if we managed to derive types for all of the parameters, return a TypeList containing them
		returnType(new TypeList(list));
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusInstantiationSource(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	if (*tree == TOKEN_BlankInstantiationSource) { // if it's a blank slot instantiation 
		returnStatus(getStatusType(tree, inStatus)); // BlankInstantiationSource (compatible in this form as a Type)
	} else if (*tree == TOKEN_SingleInitInstantiationSource) { // else if it's a regular single-initialized instantiation
		TypeStatus mutableIdStatus = getStatusType(tree, inStatus); // SingleInitInstantiationSource (compatible in this form as a Type)
		mutableIdStatus.type = mutableIdStatus.type->copy();
		mutableIdStatus.type->latchize();
		returnStatus(mutableIdStatus);
	} else if (*tree == TOKEN_MultiInitInstantiationSource) { // else if it's a regular multi-initialized instantiation
		TypeStatus mutableIdStatus = getStatusType(tree, inStatus); // MultiInitInstantiationSource (compatible in this form as a Type)
		mutableIdStatus.type = mutableIdStatus.type->copy();
		mutableIdStatus.type->poolize(tree->child->next->child->child); // LSQUARE
		returnStatus(mutableIdStatus);
	} else if (*tree == TOKEN_SingleFlowInitInstantiationSource) { // else if it's a single flow-style instantiation
		TypeStatus mutableIdStatus = getStatusType(tree->child->next, inStatus); // SingleInitInstantiationSource (compatible in this form as a Type)
		mutableIdStatus.type = mutableIdStatus.type->copy();
		mutableIdStatus.type->latchize();
		returnStatus(mutableIdStatus);
	} else if (*tree == TOKEN_MultiFlowInitInstantiationSource) { // else if it's a multi flow-style instantiation
		TypeStatus mutableIdStatus = getStatusType(tree->child->next, inStatus); // MultiInitInstantiationSource (compatible in this form as a Type)
		mutableIdStatus.type = mutableIdStatus.type->copy();
		mutableIdStatus.type->poolize(tree->child->next->child->next->child->child); // LSQUARE
		returnStatus(mutableIdStatus);
	} else if (*tree == TOKEN_CopyInstantiationSource) { // else if it's a copy-style instantiation
		TypeStatus mutableIdStatus = getStatusIdentifier(tree->child->next, inStatus); // NonArrayedIdentifier or ArrayedIdentifier
		mutableIdStatus.type = mutableIdStatus.type->copy();
		mutableIdStatus.type->copyDelatch(tree->child->next); // NonArrayedIdentifier or ArrayedIdentifier
		returnStatus(mutableIdStatus);
	} else if (*tree == TOKEN_CloneInstantiationSource) { // else if it's a clone-style instantiation
		TypeStatus mutableIdStatus = inStatus; // NonArrayedIdentifier or ArrayedIdentifier
		mutableIdStatus.type = mutableIdStatus.type->copy();
		mutableIdStatus.type->copyDelatch(tree); // CloneInstantiationSource
		returnStatus(mutableIdStatus);
	}
	GET_STATUS_CODE;
	GET_STATUS_FOOTER;
}

unsigned int decodeInitializerList(Tree *tree, deque<unsigned int> *depthList, const TypeStatus &instantiationStatus, const TypeStatus &inStatus, bool last = true) { // tree is a CurlyBracketedExp
	if (*(tree->child->next) == TOKEN_ExpList) { // if this is an ExpList base case, verify that the raw initializers are compatible
		bool failed = false;
		unsigned int initializerCount = 0;
		for (Tree *initializer = tree->child->next->child; initializer != NULL; initializer = (initializer->next != NULL) ? initializer->next->next->child : NULL) { // Exp
			initializerCount++;
			TypeStatus initializerStatus = getStatusExp(initializer, inStatus);
			if (*initializerStatus) { //  if we successfully derived a type for the initializer
				// try the ObjectType outstructor special case for instantiation
				bool objectOutstructorFound = false;
				if (initializerStatus->category == CATEGORY_OBJECTTYPE) { // else if the initializer is an object, see if one of its outstructors is acceptable
					for (StructorList::iterator outsIter = ((ObjectType *)(initializerStatus.type))->outstructorList.begin();
							outsIter != ((ObjectType *)(initializerStatus.type))->outstructorList.end();
							outsIter++) {
						if (**outsIter >> *instantiationStatus) {
							objectOutstructorFound = true;
							break;
						}
					}
				}
				if (!objectOutstructorFound) { // if the special case failed, try a direct compatibility
					if (!(*initializerStatus >> *instantiationStatus)) { // if the initializer is incompatible, throw an error
						Token curToken = initializer->t; // Exp
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"incompatible initializer in list");
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (instantiation type is "<<instantiationStatus<<")");
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (initializer type is "<<initializerStatus<<")");
						failed = true;
					}
				}
			}
		}
		if (!failed) { // if we didn't find any incompatible initializers
			if (last) { // if we're the last call among our siblings, log the initializer count in the depthList
				depthList->push_front(initializerCount);
			}
			return initializerCount; // return the count of how many initializers there were
		} else { // else if we found incompatible initializers, return error code 0
			return 0;
		}
	} else /* if (*(tree->child->next) == TOKEN_CurlyBracketedExpList) */ { // else if this is the recursive CurlyBracketedExpList case
		Tree *cbel = tree->child->next; // CurlyBracketedExpList
		unsigned int firstCbeBreadth = decodeInitializerList(cbel->child, depthList, instantiationStatus, inStatus, (last && (cbel->child->next == NULL)));
		bool failed = (firstCbeBreadth == 0);
		unsigned int breadthCount = 1;
		for (Tree *cbe = (cbel->child->next != NULL) ? cbel->child->next->next->child : NULL;
				cbe != NULL;
				cbe = (cbe->next != NULL) ? cbe->next->next->child : NULL) { // CurlyBracketedExp
			breadthCount++;
			unsigned int thisCbeBreadth = decodeInitializerList(cbe, depthList, instantiationStatus, inStatus, (last && (cbe->next == NULL)));
			if (thisCbeBreadth != firstCbeBreadth) { // if a this breadth analysis differs from the first one
				if (firstCbeBreadth != 0 && thisCbeBreadth != 0) { // if neither breadth analysis was a failure, flag a breadth mismatch error
					Token curToken = cbe->t; // CurlyBracketedExp
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"initializer list is jagged");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (found "<<thisCbeBreadth<<" elements)");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (expected "<<firstCbeBreadth<<" elements)");
					failed = true;
				} else if (firstCbeBreadth == 0 /* && thisCbeBreadth != 0 */) { // else if our first breadth analysis failed but this one didn't, log this one as the new first one
					firstCbeBreadth = thisCbeBreadth;
				} else /* if (thisCbeBreadth == 0 && firstCbeBreadth != 0) */ { // else if this breadth analysis failed but the first one didn't, flag the failure but don't report an error
					failed = true;
				}
			}
		}
		if (!failed) { // if all of the breadth analyses at this level were consistent
			if (last) { // if we're the last call among our siblings, log this level's breadth count in the depthList
				depthList->push_front(breadthCount);
			}
			return breadthCount; // return the breadth count for this call
		} else { // else if the breadth analyses at this level were inconsistent, return error code 0
			return 0;
		}
	}
}

// reports errors
TypeStatus getStatusInstantiation(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *is = tree->child->next; // InstantiationSource
	TypeStatus instantiationStatus = getStatusInstantiationSource(is, inStatus); // BlankInstantiationSource or CopyInstantiationSource
	if (*instantiationStatus) { // if we successfully derived a type for the instantiation
		if (!(instantiationStatus.type->instantiable)) { // if we are instantiating an uninstantiable node, flag an error
			Token curToken = is->t; // InstantiationSource
			Tree *identifier = (*(is->child) == TOKEN_NonArrayedIdentifier) ? is->child: is->child->next;// NonArrayedIdentifier or ArrayedIdentifier
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"instantiation of non-instantiable node '"<<identifier<<"'"); 
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (node type is "<<instantiationStatus<<")");
			returnTypeRet(errType, NULL);
		}
		if ((is->next->next == NULL || *(is->next->next->child->next) == TOKEN_RBRACKET) && *(is->child) != TOKEN_RARROW) { // if we're doing default initialization
			if (*is != TOKEN_BlankInstantiationSource && instantiationStatus.type->category == CATEGORY_OBJECTTYPE) { // if we're default-instantiating an object, ensure it's null-instantiable
				if (((ObjectType *)(instantiationStatus.type))->isNullInstantiable()) {
					returnStatus(instantiationStatus);
				} else {
					Token curToken = is->t; // InstantiationSource
					Tree *identifier = (*(is->child) == TOKEN_NonArrayedIdentifier) ? is->child: is->child->next;// NonArrayedIdentifier or ArrayedIdentifier
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"null instantiation of non-null-instantiable node '"<<identifier<<"'"); 
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (node type is "<<instantiationStatus<<")");
					returnTypeRet(errType, NULL);
				}
			} else { // else if we're default instantiating a non-object, just return the type of the instantiation
				returnStatus(instantiationStatus);
			}
		} else if (*(is->child) == TOKEN_RARROW) { // else if we're doing flow-based initialization, verify that it's valid
			if (*(is->child->next) != TOKEN_SingleAccessor) { // if it's a regular flow-based initialization
				// derive the temporary instantiation type to use for comparison, based on whether this is a single (latch) or multi (pool) initialization
				Type *mutableInstantiationType = instantiationStatus.type->copy();
				if (*(is->child->next) == TOKEN_MultiInitInstantiationSource) { // if it's a multi initialization, decrease the pool's depth to get at the initializable base type
					mutableInstantiationType->decreaseDepth();
				}
				// try the ObjectType outstructor special case for instantiation
				if (inStatus->category == CATEGORY_OBJECTTYPE) { // if the initializer is an object, see if one of its outstructors is acceptable
					for (StructorList::iterator outsIter = ((ObjectType *)(inStatus.type))->outstructorList.begin();
							outsIter != ((ObjectType *)(inStatus.type))->outstructorList.end();
							outsIter++) {
						if (**outsIter >> *mutableInstantiationType) {
							mutableInstantiationType->erase(); // delete the temporary instantiation comparison type
							returnStatus(instantiationStatus);
						}
					}
				}
				// if the special case failed, try a direct compatibility
				if (*inStatus >> *mutableInstantiationType) { // if the initializer is directly compatible, allow it
					mutableInstantiationType->erase(); // delete the temporary instantiation comparison type
					returnStatus(instantiationStatus);
				} else { // else if the initializer is incompatible, throw an error
					Token curToken = is->child->next->t; // SingleInitInstantiationSource or MultiInitInstantiationSource
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"incompatible initialization of flow instantiation");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (instantiation type is "<<mutableInstantiationType<<")");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (incoming type is "<<inStatus<<")");
					mutableInstantiationType->erase(); // delete the temporary instantiation comparison type
				}
			} else /* if (*(is->child->next) == TOKEN_SingleAccessor) */ { // else if it's a clone-based initialization
				returnStatus(instantiationStatus);
			}
		} else if (*(is->next->next) == TOKEN_BracketedExp) { // else if there is a regular initializer, make sure that its type is compatible
			Tree *initializer = is->next->next; // BracketedExp
			TypeStatus initializerStatus = getStatusBracketedExp(initializer, inStatus);
			if (*initializerStatus) { //  if we successfully derived a type for the initializer
				// derive the temporary instantiation type to use for comparison, based on whether this is a single (latch) or multi (pool) initialization
				Type *mutableInstantiationType = instantiationStatus.type->copy();
				if (*is == TOKEN_MultiInitInstantiationSource) { // if it's a multi initialization, decrease the pool's depth to get at the initializable base type
					mutableInstantiationType->decreaseDepth();
				}
				// try the ObjectType outstructor special case for instantiation
				 if (initializerStatus->category == CATEGORY_OBJECTTYPE) { // if the initializer is an object, see if one of its outstructors is acceptable
					for (StructorList::iterator outsIter = ((ObjectType *)(initializerStatus.type))->outstructorList.begin();
							outsIter != ((ObjectType *)(initializerStatus.type))->outstructorList.end();
							outsIter++) {
						if (**outsIter >> *mutableInstantiationType) {
							mutableInstantiationType->erase(); // delete the temporary instantiation comparison type
							returnStatus(instantiationStatus);
						}
					}
				}
				// if the special case failed, try a direct compatibility
				if (*initializerStatus >> *mutableInstantiationType) { // if the initializer is directly compatible, allow it
					mutableInstantiationType->erase(); // delete the temporary instantiation comparison type
					returnStatus(instantiationStatus);
				} else { // else if the initializer is incompatible, throw an error
					Token curToken = initializer->t; // BracketedExp
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"incompatible initializer");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (instantiation type is "<<mutableInstantiationType<<")");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (initializer type is "<<initializerStatus<<")");
					mutableInstantiationType->erase(); // delete the temporary instantiation comparison type
				}
			}
		} else /* if (*(is->next->next) == TOKEN_CurlyBracketedExp) */ { // else if there is an initializer list, make sure that all of the initializers are compatiable
			deque<unsigned int> *depthList = new deque<unsigned int>();
			if (decodeInitializerList(is->next->next, depthList, instantiationStatus, inStatus) > 0) { // if we succeeded in decoding the initializer list depths
				TypeStatus mutableInstantiationStatus = instantiationStatus;
				mutableInstantiationStatus.type = mutableInstantiationStatus.type->copy();
				mutableInstantiationStatus.type->depth = depthList->size();
				mutableInstantiationStatus.type->poolize(new Tree(depthList)); // use the depthList as the pool size expression
				returnStatus(mutableInstantiationStatus);
			} else { // else if we failed to decode the initializer list depths, delete the allocated depthList
				delete depthList;
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
		getStatusFilter(nodec, inStatus); // blindly generates a thunk; never fails
		returnStatus(verifyStatusFilter(nodec)); // return the status resulting from verifying the contents of this filter
	} else if (*nodec == TOKEN_Object) {
		getStatusObject(nodec, inStatus); // blindly generates a thunk; never fails
		returnStatus(verifyStatusObject(nodec)); // return the status resulting from verifying the contents of this object
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
			if (nodeStatus->referensible) { // if the node is referensible on its own
				Tree *tstcc = tstc->child;
				if ((*tstcc == TOKEN_NonArrayedIdentifier || *tstcc == TOKEN_ArrayedIdentifier) &&
						!(nodeStatus->category == CATEGORY_FILTERTYPE && nodeStatus->suffix == SUFFIX_LATCH) && nodeStatus.type != stdBoolLitType) { // if the Node needs to be constantized
					// first, copy the Type so that our mutations don't propagate to the StaticTerm
					TypeStatus mutableNodeStatus = nodeStatus;
					mutableNodeStatus.type = mutableNodeStatus.type->copy();
					mutableNodeStatus->constantize();
					returnStatus(mutableNodeStatus);
				} else { // else if the node doesn't need to be constantized, just return the nodeStatus
					returnStatus(nodeStatus);
				}
			} else { // else if it's a standard node that we can't use an access operator on, flag an error
				Token curToken = tstc->child->child->t; // guaranteed to be ID, since only NonArrayedIdentifier or ArrayedIdentifier nodes generate inoperable types
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"reference to non-referensible node '"<<tstc->child<<"'");
			}
		}
	} else if (*tstc == TOKEN_BracketedExp) { // else if it's an expression
		TypeStatus expStatus = getStatusBracketedExp(tstc, inStatus);
		if (*expStatus) { // if we managed to derive a type for the expression node
			if (expStatus.type->suffix != SUFFIX_LIST && expStatus.type->suffix != SUFFIX_STREAM) { // if the type isn't inherently dynamic, return the status normally
				returnStatus(expStatus);
			} else { // else if the type is inherently dynamic, flag an error
				Token curToken = tstc->child->t; // LBRACKET
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"expression returns dynamic type");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (expression type is "<<expStatus<<")");
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
		if (!(nodeStatus->referensible)) { // else if it's a standard node that we can't use an access operator on, flag an error
			Token curToken = tree->child->child->t; // SLASH, SSLASH, ASLASH, DSLASH, DSSLASH, or DASLASH
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"access of immutable node '"<<tree->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (node type is "<<nodeStatus<<")");
		} else if (nodeStatus.type == stdBoolLitType) { // else if it's an access of a standard literal, flag an error
			Token curToken = tree->child->child->t; // SLASH, SSLASH, ASLASH, DSLASH, DSSLASH, or DASLASH
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"access of immutable literal '"<<tree->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
		} else if (nodeStatus.type->category == CATEGORY_STDTYPE && !(((StdType *)(nodeStatus.type))->isComparable())) { // else if it's an access of an incomparable StdType, flag an error
			Token curToken = tree->child->child->t; // SLASH, SSLASH, ASLASH, DSLASH, DSSLASH, or DASLASH
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"access of immutable standard node");
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (node type is "<<nodeStatus<<")");
		} else { // else if it's an otherwise acceptable access, attempt it
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
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"delatch of incompatible type");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			} else if (*accessorc == TOKEN_DSLASH) {
				if (mutableNodeStatus.type->destream()) {
					returnStatus(mutableNodeStatus);
				} else {
					Token curToken = accessorc->t; // SLASH, DSLASH, or LSQUARE
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"destream of incompatible type");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			} else /* if (*accessorc == TOKEN_LSQUARE) */ {
				if (mutableNodeStatus.type->delist()) {
					returnStatus(mutableNodeStatus);
				} else {
					Token curToken = accessorc->t; // SLASH, DSLASH, or LSQUARE
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"delist of incompatible type");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			}
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
	} else /* if (*stc == TOKEN_SingleAccess || *stc == TOKEN_MultiAccess) */ {
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
	} else if (*dtc == TOKEN_Pack) {
		TypeStatus packedStatus = inStatus;
		packedStatus.type = packedStatus.type->copy();
		if (packedStatus->pack()) { // if we managed to pack the type, proceed normally
			returnStatus(packedStatus);
		} else { // else if we failed to pack the type, erase the copied type and flag an error
			Token curToken = dtc->child->t; // RFLAG
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"incoming type cannot be packed");
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (type is "<<inStatus<<")");
			packedStatus->erase();
		}
	} else if (*dtc == TOKEN_Unpack) {
		TypeStatus unpackedStatus = inStatus;
		unpackedStatus.type = unpackedStatus.type->copy();
		if (unpackedStatus->unpack()) { // if we managed to unpack the type, proceed normally
			returnStatus(unpackedStatus);
		} else { // else if we failed to unpack the type, erase the copied type and flag an error
			Token curToken = dtc->child->t; // LFLAG
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"incoming type cannot be unpacked");
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (type is "<<inStatus<<")");
			unpackedStatus->erase();
		}
	} else if (*dtc == TOKEN_Link) {
		TypeStatus linkStatus = getStatusStaticTerm(dtc->child->next);
		if (*linkStatus) { // if we managed to derive a type for the Link subnode
			Type *linkType = inStatus->link(*linkStatus);
			if (*linkType) { // if the types are link-compatible, return the resulting type
				returnTypeRet(linkType, inStatus.retType);
			} else {
				Token curToken = dtc->child->t; // DCOLON
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"link with incompatible type");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (incoming type is "<<inStatus<<")");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (link type is "<<linkStatus<<")");
			}
		}
	} else if (*dtc == TOKEN_Loopback) {
		SymbolTree *enclosingEnv = dtc->env;
		if (enclosingEnv->kind == KIND_BLOCK && enclosingEnv->parent != NULL) {
			SymbolTree *enclosingParent = enclosingEnv->parent;
			if (enclosingParent->kind == KIND_FILTER) {
				FilterType *enclosingType = (FilterType *)(enclosingParent->defSite->status.type);
				if ((*inStatus == *nullType && *(enclosingType->from()) == *nullType) || (*inStatus >> *(enclosingType->from()))) {
					if (enclosingType->to() == NULL) {
						Token curToken = dtc->child->t; // LARROW
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"irresolvable implicit loopback return type");
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (incoming type is "<<enclosingType->from()<<")");
					} else if (enclosingType->to()->list.size() == 1) {
						returnTypeRet(enclosingType->to()->list[0], inStatus.retType);
					} else {
						returnTypeRet(enclosingType->to(), inStatus.retType);
					}
				} else {
					Token curToken = dtc->child->t; // LARROW
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"loopback of unexpected type "<<inStatus);
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (expected type is "<<enclosingType->from()<<")");
				}
			} else if (enclosingParent->kind == KIND_INSTRUCTOR) {
				TypeList *enclosingType = (TypeList *)(enclosingParent->defSite->status.type);
				if (*inStatus >> *enclosingType) {
					returnTypeRet(nullType, inStatus.retType);
				} else {
					Token curToken = dtc->child->t; // LARROW
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"loopback of unexpected type "<<inStatus);
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (expected type is "<<enclosingType<<")");
				}
			} else if (enclosingParent->kind == KIND_OUTSTRUCTOR) {
				TypeList *enclosingType = (TypeList *)(enclosingParent->defSite->status.type);
				if (*inStatus == *nullType) {
					if (enclosingType->list.size() == 1) {
						returnTypeRet(enclosingType->list[0], inStatus.retType);
					} else {
						returnTypeRet(enclosingType, inStatus.retType);
					}
				} else {
					Token curToken = dtc->child->t; // LARROW
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"loopback of unexpected type "<<inStatus);
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (expected type is "<<enclosingType<<")");
				}
			}
		} else {
			Token curToken = dtc->child->t; // LARROW
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"loopback outside of a filter block");
		}
	} else if (*dtc == TOKEN_Send) {
		TypeStatus nodeStatus = getStatusNode(dtc->child->next, inStatus);
		if (*nodeStatus) { // if we managed to derive a type for the send destination
			if ((nodeStatus->referensible || nodeStatus.type == stringerType) &&
					(nodeStatus.type != stdBoolLitType)) { // if the destination allows sends
				if (*inStatus >> *nodeStatus) { // if the Send is valid, proceed normally
					returnType(nullType);
				} else { // else if the Send is invalid, flag an error
					Token curToken = dtc->child->t; // RARROW
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"send to incompatible type");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (source type is "<<inStatus<<")");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (destination type is "<<nodeStatus<<")");
				}
			} else if (!(nodeStatus->referensible)) { // else if it's a standard node that we can't use an access operator on, flag an error
				Token curToken = dtc->child->t; // RARROW
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"send to immutable node '"<<dtc->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (node type is "<<nodeStatus<<")");
			} else /* if (nodeStatus.type == stdBoolLitType) */ { // else if it's an access of a standard literal, flag an error
				Token curToken = dtc->child->t; // RARROW
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"send to immutable literal '"<<dtc->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
			}
		}
	} else if (*dtc == TOKEN_Swap) {
		TypeStatus nodeStatus = getStatusNode(dtc->child->next, inStatus);
		if (*nodeStatus) { // if we managed to derive a type for the swap destination
			if ((nodeStatus->referensible || nodeStatus.type == stringerType) &&
					(nodeStatus.type != stdBoolLitType)) { // if the destination allows swaps
				if (*inStatus >> *nodeStatus) { // if the Swap is valid, proceed normally
					returnType(nodeStatus.type);
				} else { // else if the Send is invalid, flag an error
					Token curToken = dtc->child->t; // RARROW
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"swap with incompatible type");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (source type is "<<inStatus<<")");
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (destination type is "<<nodeStatus<<")");
				}
			} else if (!(nodeStatus->referensible)) { // else if it's a standard node that we can't use an access operator on, flag an error
				Token curToken = dtc->child->t; // LRARROW
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"swap with immutable node '"<<dtc->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (node type is "<<nodeStatus<<")");
			} else /* if (nodeStatus.type == stdBoolLitType) */ { // else if it's an access of a standard literal, flag an error
				Token curToken = dtc->child->t; // LRARROW
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"swap with immutable literal '"<<dtc->child->next->child<<"'"); // NonArrayedIdentifier or ArrayedIdentifier
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
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"return of unexpected type "<<thisRetType);
				if (*knownRetType == *errType) {
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (not expecting a return here)");
				} else {
					semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (expected type is "<<knownRetType<<")");
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
		if (*ltc == TOKEN_TypedStaticTerm) {
			// derive the label's type
			TypeStatus label = getStatusTypedStaticTerm(ltc, inStatus);
			if (!(*label >> *inStatus)) { // if the type doesn't match, throw an error
				Token curToken = ltc->t;
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"incompatible switch label");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (label type is "<<label<<")");
				semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
			}
		}
		// derive the to-type of this label
		Tree *toTree = (*ltc == TOKEN_TypedStaticTerm) ? ltc->next->next : ltc->next; // SimpleTerm
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
			semmerError(curToken1.fileIndex,curToken1.row,curToken1.col,"inconsistent switch destination type");
			semmerError(curToken1.fileIndex,curToken1.row,curToken1.col,"-- (this type is "<<thisToStatus<<")");
			semmerError(curToken2.fileIndex,curToken2.row,curToken2.col,"-- (first type is "<<firstToStatus<<")");
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
		semmerError(curToken.fileIndex,curToken.row,curToken.col,"non-boolean input to conditional");
		semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
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
				semmerError(curToken1.fileIndex,curToken1.row,curToken1.col,"type mismatch in conditional branches");
				semmerError(curToken2.fileIndex,curToken2.row,curToken2.col,"-- (true branch type is "<<trueStatus<<")");
				semmerError(curToken3.fileIndex,curToken3.row,curToken3.col,"-- (false branch type is "<<falseStatus<<")");
			}
		}
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileIndex,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
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
					*(curTerm->child->child->child->child->child) == TOKEN_Node &&
					*(curTerm->child->child->child->child->child->child) != TOKEN_Instantiation) { // if it's a flow-through Term
				pair<Type *, bool> stdFlowResult(errType, false);
				if (nextTermStatus->category == CATEGORY_STDTYPE) { // if this Term's type is a STDTYPE, try to derive a three-term exceptional type for it
					stdFlowResult = ((StdType *)(nextTermStatus.type))->stdFlowDerivation(curStatus, curTerm->next->child);
				}
				if (*(stdFlowResult.first)) { // if we managed to derive a three-term exceptional type for this term
					curStatus = TypeStatus(stdFlowResult.first, nextTermStatus); // log the three-term exceptional type as the current status
					if (stdFlowResult.second) { // if we used a third term for the derivation, advance curTerm past it
						curTerm = curTerm->next->child;
					}
				} else if (*curStatus == *nullType &&
						nextTermStatus.type->category == CATEGORY_FILTERTYPE && *(((FilterType *)(nextTermStatus.type))->from()) == *nullType) { // else if this is a generator invocation
					// use the generator's result type as the current status
					curStatus = TypeStatus(((FilterType *)(nextTermStatus.type))->to(), nextTermStatus);
				} else { // else if this is not a recognized exceptional case
					// derive a type for the flow of the current type into the next term in the sequence
					Type *flowResult = (*curStatus , *nextTermStatus);
					if (flowResult == NULL) {
						Token curToken = curTerm->t; // Term
						Token prevToken = prevTerm->t; // Term
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"irresolvable implicit filter return type");
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (term's type is "<<nextTermStatus<<")");
						semmerError(prevToken.fileIndex,prevToken.row,prevToken.col,"-- (incoming type is "<<curStatus<<")");
						// short-circuit the derivation of this NonEmptyTerms
						curStatus = errType;
						break;
					} else if (*flowResult) { // if the type flow is valid, log it as the current status
						curStatus = TypeStatus(flowResult, nextTermStatus);
					} else if (*curStatus == *nullType) { // else if the flow is not valid, but the incoming type is null, log the next term's status as the current one
						curStatus = nextTermStatus;
					} else { // else if the type flow is not valid and the incoming type is not null, flag an error
						Token curToken = curTerm->t; // Term
						Token prevToken = prevTerm->t; // Term
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"term does not accept incoming type");
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (term's type is "<<nextTermStatus<<")");
						semmerError(prevToken.fileIndex,prevToken.row,prevToken.col,"-- (incoming type is "<<curStatus<<")");
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
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"cannot resolve term's output type");
			semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (input type is "<<curStatus<<")");
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
TypeStatus getStatusDeclaration(Tree *tree) {
	GET_STATUS_HEADER;
	// check if this is a recursive invocation
	Type *&fakeRetType = tree->status.retType;
	if (fakeRetType != NULL) { // if we previously logged a recursion alert here (and we don't have a memoized type to return), flag an ill-formed recursion error
		Token curToken = tree->child->t;
		semmerError(curToken.fileIndex,curToken.row,curToken.col,"irresolvable recursive definition of '"<<curToken.s<<"'");
	} else { // else if there is no recursion alert for this Declaration
		// flag a recursion alert for this Declaration and proceed normally
		fakeRetType = errType;
		// check if this is a Filter or Object derivation special case
		Tree *declarationSub = tree->child->next->next; // TypedStaticTerm, NonEmptyTerms, or NULL
		if (declarationSub != NULL && *declarationSub == TOKEN_TypedStaticTerm && *(declarationSub->child) == TOKEN_Node) {
			if (*(declarationSub->child->child) == TOKEN_Filter) {
				TypeStatus derivedStatus = getStatusFilter(declarationSub->child->child); // blindly generates a thunk; never fails
				// log the derived type as the return value of this declaration
				Type *&fakeType = tree->status.type;
				fakeType = derivedStatus.type;
				// verify the internal contents of the filter
				TypeStatus verifiedStatus = verifyStatusFilter(declarationSub->child->child);
				if (*verifiedStatus) { // if the verification was successful, return the derived status
					returnTypeRet(verifiedStatus, NULL);
				} else { // else if the verification failed
					returnTypeRet(errType, NULL);
				}
			} else if (*(declarationSub->child->child) == TOKEN_Object) {
				TypeStatus derivedStatus = getStatusObject(declarationSub->child->child); // blindly generates a thunk; never fails
				// log the derived type as the return value of this declaration
				Type *&fakeType = tree->status.type;
				fakeType = derivedStatus.type;
				// verify the internal contents of the object
				TypeStatus verifiedStatus = verifyStatusObject(declarationSub->child->child);
				if (*verifiedStatus) { // if the verification was successful, return the derived status
					returnTypeRet(verifiedStatus, NULL);
				} else { // else if the verification failed
					returnTypeRet(errType, NULL);
				}
			}
		}
		// otherwise, if this is not a Filter or Object special case
		if (*(tree->child) != TOKEN_AT) { // if it's a non-import declaration
			// attempt to derive the type of this Declaration
			if (*declarationSub == TOKEN_TypedStaticTerm) { // if it's a standard declaration
				TypeStatus derivedStatus = getStatusTypedStaticTerm(declarationSub);
				if (*derivedStatus) { // if we managed to derive a type for this node
					if (derivedStatus.type->category != CATEGORY_TYPELIST) { // if the derived type is not a TypeList, simply return it 
						returnTypeRet(derivedStatus, NULL);
					} else { // else if the derived type is a TypeList, flag an error
						Token curToken = tree->t; // Declaration
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"declaration of compound-typed identifier '"<<tree->child->t.s<<"'");
						semmerError(curToken.fileIndex,curToken.row,curToken.col,"-- (identifier type is "<<derivedStatus<<")");
					}
				}
			} else if (*declarationSub == TOKEN_BlankInstantiation) { // else if it's a blank instantiation declaration
				returnTypeRet(getStatusInstantiation(declarationSub), NULL);
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
	Tree *pipec = tree->child; // Declaration or NonEmptyTerms
	if (*pipec == TOKEN_Declaration) { // if it's a Declaration-style pipe
		returnStatus(getStatusDeclaration(pipec));
	} else if (*pipec == TOKEN_NonEmptyTerms) { // else if it's a raw NonEmptyTerms pipe
		returnStatus(getStatusNonEmptyTerms(pipec, inStatus));
	}
	GET_STATUS_CODE;
	vector<CodeTree *> seqList;
	
	// LOL need to actually fill seqList with the CodeTree nodes to execute in this pipe
	
	returnCode(new SeqTree(seqList));
	GET_STATUS_FOOTER;
}

void semPipes(Tree *treeRoot) {
	TypeStatus rootStatus(nullType, stdIntType);
	for (Tree *programCur = treeRoot; programCur != NULL; programCur = programCur->next) {
		for (Tree *pipeCur = programCur->child->child; pipeCur != NULL; pipeCur = (pipeCur->next != NULL) ? pipeCur->next->child : NULL) {
			getStatusPipe(pipeCur, rootStatus);
		}
	}
}

// creates the top-level SchedTree containing all of the LabelTrees that should be initially scheduled
SchedTree *genCodeRoot(Tree *treeRoot) {
	// build the list of labels that should be initially scheduled
	vector<LabelTree *> labelList;
	for (Tree *programCur = treeRoot; programCur != NULL; programCur = programCur->next) {
		for (Tree *pipeCur = programCur->child->child; pipeCur != NULL; pipeCur = (pipeCur->next != NULL) ? pipeCur->next->child : NULL) {
			labelList.push_back(new LabelTree((SeqTree *)(pipeCur->status.code)));
		}
	}
	// finally, return the resulting SchedTree
	return (new SchedTree(labelList));
}

// main semming function; makes no assumptions about stRoot and codeRoot's values; they're just return parameters
int sem(Tree *treeRoot, SymbolTree *&stRoot, SchedTree *&codeRoot) {

	// initialize local error code
	semmerErrorCode = 0;

	VERBOSE( printNotice("building symbol tree..."); )

	// initialize the standard types and nodes
	initSemmerGlobals();
	
	// build the symbol tree
	stRoot = genDefaultDefs(); // initialize the symbol tree root with the default definitions
	vector<SymbolTree *> importList; // list of import Declaration nodes; will be populated in the next step
	buildSt(treeRoot, stRoot, importList); // get user definitions and populate the import list
	subImportDecls(importList); // resolve and substitute import declarations into the symbol tree

	VERBOSE( printNotice("tracing data flow..."); )

	// perform semantic analysis (derivation of Type trees and offsets) on the entire SymbolTree
	semSt(stRoot);
	// perform semantic analysis (derivation of Type and IR trees) on the remaining pipes
	semPipes(treeRoot);
	
	// build the root-level IRTree node at which assembly dumping will start
	codeRoot = genCodeRoot(treeRoot);
	
	VERBOSE( cout << stRoot; )

	// if there were no errors, delete the error type node
	if (!semmerErrorCode) {
		delete errType;
	}

	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
