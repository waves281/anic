#include "semmer.h"

#include "customOperators.h"

// semmer-global variables

int semmerErrorCode;

Type *nullType = new StdType(STD_NULL);
Type *errType = new ErrorType();

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
SymbolTable::SymbolTable(const SymbolTable &st) {*this = st;}
SymbolTable::~SymbolTable() {
	// delete all of the child nodes
	for (vector<SymbolTable *>::iterator childIter = children.begin(); childIter != children.end(); childIter++) {
		delete *childIter;
	}
}

// deep-copy assignment operator
SymbolTable &SymbolTable::operator =(const SymbolTable &st) {
	kind = st.kind;
	id = st.id;
	defSite = st.defSite;
	parent = st.parent;
	for (vector<SymbolTable *>::const_iterator childIter = st.children.begin(); childIter != st.children.end(); childIter++) {
		// copy the child node
		SymbolTable *child = new SymbolTable(**childIter);
		// fix the child's parent pointer to point up to this node
		child->parent = this;
		// finally, log the child in the copied child into the children list
		children.push_back(child);
	}
	return *this;
}

// concatenators
SymbolTable &SymbolTable::operator *=(SymbolTable *st) {
	// first, check for conflicting bindings
	if (st != NULL && (st->kind == KIND_STD || st->kind == KIND_DECLARATION || st->kind == KIND_PARAMETER)) { // if this is a conflictable (non-special system-level binding)
		// per-symbol loop
		for (vector<SymbolTable *>::const_iterator childIter = children.begin(); childIter != children.end(); childIter++) {
			if ((*childIter)->id == st->id) { // if we've found a conflict
				Token curDefToken;
				if (st->defSite != NULL) { // if there is a definition site for the current symbol
					curDefToken = st->defSite->t;
				} else { // otherwise, it must be a standard definition, so make up the token as if it was
					curDefToken.fileName = STANDARD_LIBRARY_STRING;
					curDefToken.row = 0;
					curDefToken.col = 0;
				}
				Token prevDefToken;
				if ((*childIter)->defSite != NULL) { // if there is a definition site for the previous symbol
					prevDefToken = (*childIter)->defSite->t;
				} else { // otherwise, it must be a standard definition, so make up the token as if it was
					prevDefToken.fileName = STANDARD_LIBRARY_STRING;
					prevDefToken.row = 0;
					prevDefToken.col = 0;
				}
				semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"redefinition of '"<<st->id<<"'");
				semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
				delete st;
				return *this;
			} // if there's a conflict
		} // for per-symbol loop
	} // if this is not a special system-level binding
	// binding is now known to be conflict-free, so log it normally
	children.push_back(st);
	if (st != NULL) {
		st->parent = this;
		return *st;
	} else {
		return *this;
	}
}

// Main semantic analysis functions

void catStdNodes(SymbolTable *&stRoot) {
	*stRoot *= new SymbolTable(KIND_STD, "int", new StdType(STD_INT));
	*stRoot *= new SymbolTable(KIND_STD, "float", new StdType(STD_FLOAT));
	*stRoot *= new SymbolTable(KIND_STD, "bool", new StdType(STD_BOOL));
	*stRoot *= new SymbolTable(KIND_STD, "char", new StdType(STD_CHAR));
	*stRoot *= new SymbolTable(KIND_STD, "string", new StdType(STD_STRING));
	*stRoot *= new SymbolTable(KIND_STD, "null", new StdType(STD_NULL));
}

void catStdLib(SymbolTable *&stRoot) {
	// standard root
	SymbolTable *stdLib = new SymbolTable(KIND_STD, STANDARD_LIBRARY_STRING, new StdType(STD_STD));
	// system nodes
	// streams
	*stdLib *= new SymbolTable(KIND_STD, "inInt", new StdType(STD_INT, SUFFIX_STREAM, 1));
	*stdLib *= new SymbolTable(KIND_STD, "inFloat", new StdType(STD_FLOAT, SUFFIX_STREAM, 1));
	*stdLib *= new SymbolTable(KIND_STD, "inChar", new StdType(STD_CHAR, SUFFIX_STREAM, 1));
	*stdLib *= new SymbolTable(KIND_STD, "inString", new StdType(STD_STRING, SUFFIX_STREAM, 1));
	// create the stringer type that the outer type uses
	vector<TypeList *> constructorTypes;
	vector<string> memberNames;
	memberNames.push_back("toString");
	vector<Type *> memberTypes;
	memberTypes.push_back(new FilterType(nullType, new StdType(STD_STRING, SUFFIX_LATCH), SUFFIX_LATCH));
	vector<Tree *> memberDefSites;
	memberDefSites.push_back(NULL);
	Type *stringer = new ObjectType(constructorTypes, memberNames, memberTypes, memberDefSites, SUFFIX_LATCH);
	// create the outer type that the output streams use
	constructorTypes.push_back(new TypeList(new StdType(STD_INT)));
	constructorTypes.push_back(new TypeList(new StdType(STD_FLOAT)));
	constructorTypes.push_back(new TypeList(new StdType(STD_BOOL)));
	constructorTypes.push_back(new TypeList(new StdType(STD_CHAR)));
	constructorTypes.push_back(new TypeList(new StdType(STD_STRING)));
	constructorTypes.push_back(new TypeList(stringer));
	Type *outer = new ObjectType(constructorTypes, SUFFIX_LATCH);
	*stdLib *= new SymbolTable(KIND_STD, "out", outer);
	*stdLib *= new SymbolTable(KIND_STD, "err", outer);
	// control nodes
	*stdLib *= new SymbolTable(KIND_STD, "randInt", new StdType(STD_INT, SUFFIX_STREAM, 1));
	*stdLib *= new SymbolTable(KIND_STD, "delay", new FilterType(new StdType(STD_INT), nullType, SUFFIX_LATCH));
	// standard library
	// generators
	*stdLib *= new SymbolTable(KIND_STD, "gen", new FilterType(new StdType(STD_INT), new StdType(STD_INT, SUFFIX_STREAM, 1), SUFFIX_LATCH));
	// concatenate the library to the root
	*stRoot *= stdLib;
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
		SymbolTable *blockDef = new SymbolTable(KIND_OBJECT, (*tree == TOKEN_Block) ? BLOCK_NODE_STRING : OBJECT_NODE_STRING, tree);
		// recurse
		buildSt(tree->child, blockDef, importList); // child of Block or Object
		if (blockDef->children.size() > 0) { // if there are any subnodes, link the block node into the main trunk
			*st *= blockDef;
		} else { // else if there are no subnodes, don't bother linking the block node into the main trunk
			delete blockDef;
		}
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Filter) { // if it's a filter node
		// allocate the new filter definition node
		SymbolTable *filterDef = new SymbolTable(KIND_FILTER, FILTER_NODE_STRING, tree);
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
		// recurse
		buildSt(tree->child, filterDef, importList); // child of Filter
		if (filterDef->children.size() > 0) { // if there are any subnodes, link the filter node into the main trunk
			*st *= filterDef;
		} else { // else if there are no subnodes, don't bother linking the filter node into the main trunk
			delete filterDef;
		}
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Constructor || *tree == TOKEN_LastConstructor) { // if it's a Constructor-style node
		// allocate the new constructor definition node
		SymbolTable *consDef = new SymbolTable(KIND_CONSTRUCTOR, CONSTRUCTOR_NODE_STRING, tree);
		// .. and link it in
		*st *= consDef;
		// link in the parameters of this constructor, if any
		Tree *conscn = tree->child->next; // NULL, SEMICOLON, LSQUARE, or NonRetFilterHeader
		if (conscn != NULL && *conscn == TOKEN_NonRetFilterHeader && *(conscn->child->next) == TOKEN_ParamList) { // if there is actually a parameter list on this constructor
			Tree *pl = conscn->child->next; // ParamList
			for (Tree *param = pl->child; param != NULL; param = (param->next != NULL) ? param->next->next->child : NULL) { // per-param loop
				// allocate the new parameter definition node
				SymbolTable *paramDef = new SymbolTable(KIND_PARAMETER, param->child->next->t.s, param);
				// ... and link it into the constructor definition node
				*consDef *= paramDef;
			}
		}
		// recurse
		buildSt(tree->child, consDef, importList); // child of Constructor
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Declaration || *tree == TOKEN_LastDeclaration) { // if it's a Declaration-style node
		Tree *bnc = tree->child->next;
		if (*bnc == TOKEN_EQUALS) { // standard static declaration
			// allocate the new declaration node
			SymbolTable *newDef = new SymbolTable(KIND_DECLARATION, tree->child->t.s, tree);
			// ... and link it in
			*st *= newDef;
			// recurse
			buildSt(tree->child, newDef, importList); // child of Declaration
			buildSt(tree->next, st, importList); // right
		} else if (*bnc == TOKEN_ERARROW) { // flow-through declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_DECLARATION, tree->child->t.s, tree);
			// ... and link it in
			*st *= newDef;
			// recurse
			buildSt(tree->child, newDef, importList); // child of Declaration
			buildSt(tree->next, st, importList); // right
		} else if (*bnc == TOKEN_SuffixedIdentifier) { // import declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_IMPORT, IMPORT_DECL_STRING, tree);
			// ... and link it in
			*st *= newDef;
			// also, since it's an import declaration, log it to the import list
			importList.push_back(newDef);
			// recurse
			buildSt(tree->child, newDef, importList); // child of Declaration
			buildSt(tree->next, st, importList); // right
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

SymbolTable *bindId(const string &s, SymbolTable *env, const TypeStatus &inStatus = TypeStatus()) {
	vector<string> id = chopId(s); // chop up the input identifier into its components
	if (id[0] == "..") { // if the identifier begins with a recall
		return NULL; // LOL need to implement recall identifier binding
	} else { // else if it's a regular identifier
		SymbolTable *stRoot = NULL;
		for (SymbolTable *stCur = env; stCur != NULL; stCur = stCur->parent) { // scan for a latch point for the beginning of the identifier
			if ((stCur->kind == KIND_STD ||
					stCur->kind == KIND_DECLARATION ||
					stCur->kind == KIND_PARAMETER) &&
					stCur->id == id[0]) { // if this is a valid latch point, log it and break
				stRoot = stCur;
				break;
			} else if (stCur->kind == KIND_BLOCK ||
					stCur->kind == KIND_OBJECT ||
					stCur->kind == KIND_CONSTRUCTOR ||
					stCur->kind == KIND_FILTER) { // else if this is a valid basis block, scan its children for a latch point
				for (vector<SymbolTable *>::const_iterator iter = stCur->children.begin(); iter != stCur->children.end(); iter++) {
					if (((*iter)->kind == KIND_STD ||
							(*iter)->kind == KIND_DECLARATION ||
							(*iter)->kind == KIND_PARAMETER) &&
							(*iter)->id == id[0]) { // if this is a valid latch point, log it and break
						stRoot = (*iter);
						break;
					}
				}
			}
		}
		if (stRoot != NULL) { // if we managed to find an initial latch point, verify the rest of the binding
			// KOL need to take into account the referensability of bindings (e.g. cannot reference directly through a stream)
			SymbolTable *stCur = stRoot; // the basis under which we're hoping to bind the current sub-identifier (KIND_STD, KIND_DECLARATION, or KIND_PARAMETER)
			for (unsigned int i = 1; i < id.size(); i++) { // for each sub-identifier of the identifier we're trying to find the binding for
				bool success = false;
				if (stCur->kind == KIND_STD) { // if it's a standard system-level binding, look in the list of children for a match to this sub-identifier
					for (vector<SymbolTable *>::const_iterator iter = stCur->children.begin(); iter != stCur->children.end(); iter++) {
						if ((*iter)->id == id[i]) { // if this child matches the sub-identifier, accept it and proceed deeper into the binding
							stCur = (*iter);
							success = true;
							break;
						}
					}
				} else { // else if it's not a standard system-level binding, we must get at the sub-identifier binding based on this SymbolTable node's type
					Type *stCurType = errType;
					if (stCur->kind == KIND_DECLARATION) { // else if it's a Declaration binding, carefully get its type (we can't derive flow-through types so naively, but these cannot be sub-identified anyway)
						Tree *discriminant = stCur->defSite->child->next->next; // TypedStaticTerm, NonEmptyTerms, or NULL
						if (*discriminant == TOKEN_TypedStaticTerm) { // if it's a Declaration that could possibly have sub-identifiers, derive its type
							stCurType = getStatusDeclaration(stCur->defSite);
						}
					} else { // else if it's a Param binding, naively get its type
						stCurType = getStatusParam(stCur->defSite);
					}
					if (*stCurType) { // if we managed to derive a type for this SymbolTable node
						if (stCurType->category == CATEGORY_OBJECTTYPE) { // if it's an Object Declaration (the only category that can have sub-identifiers), try to find a match in the Object's members
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
									success = true;
									break;
								} else { // else if the member has no real definition site, we'll need to fake one
									// KOL need to fake the definition site
								}
							}
						}
					}
				}
				if (!success) { // if we didn't find a binding for this sub-identifier, return failure
					return NULL;
				} // else if we managed to find a binding for this sub-identifier, continue onto trying to bind the next one
			}
			// if we managed to bind all of the sub-identifiers, return the root of the binding
			return stRoot;
		} else { // else if we failed to find an initial latch point, return failure
			return NULL;
		}
	}
}

void subImportDecls(vector<SymbolTable *> importList) {
	bool stdExplicitlyImported = false;
	for(;;) { // per-change loop
		// per-import loop
		vector<SymbolTable *> newImportList;
		for (vector<SymbolTable *>::iterator importIter = importList.begin(); importIter != importList.end(); importIter++) {
			// extract the import path out of the iterator
			string importPath = *((*importIter)->defSite->child->next);
			// standard import special-casing
			if (importPath == "std") { // if it's the standard import
				if (!stdExplicitlyImported) { // if it's the first standard import, flag it as handled and let it slide
					(*importIter)->id = STANDARD_IMPORT_DECL_STRING;
					stdExplicitlyImported = true;
					continue;
				}
			}
			// otherwise, try to find a non-std binding for this import
			SymbolTable *binding = bindId(importPath, *importIter);
			if (binding != NULL && importPath.empty()) { // if we found a complete static binding
				// check to make sure that this import doesn't cause a binding conflict
				string importPathTip = binding->id; // must exist if binding succeeed
				// per-parent's children loop (parent must exist, since the root is a block st node)
				vector<SymbolTable *>::iterator childIter = (*importIter)->parent->children.begin();
				while (childIter != (*importIter)->parent->children.end()) {
					// LOL need to check for type conflicts in constructors
					if (((*childIter)->kind == KIND_STD ||
							(*childIter)->kind == KIND_DECLARATION ||
							(*childIter)->kind == KIND_PARAMETER) && (*childIter)->id == importPathTip) { // if there's a conflict
						Token curDefToken = (*importIter)->defSite->child->next->child->t; // child of Identifier
						Token prevDefToken;
						if ((*childIter)->defSite != NULL) { // if there is a definition site for the previous symbol
							prevDefToken = (*childIter)->defSite->t;
						} else { // otherwise, it must be a standard definition, so make up the token as if it was
							prevDefToken.fileName = STANDARD_LIBRARY_STRING;
							prevDefToken.row = 0;
							prevDefToken.col = 0;
						}
						semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"name conflict in importing '"<<importPathTip<<"'");
						semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (conflicting definition was here)");
						goto nextImport;
					}
					// advance
					childIter++;
				}
				// there was no conflict, so just deep-copy the binding in place of the import placeholder node
				**importIter = *binding;
			} else { // else if no binding could be found
				newImportList.push_back(*importIter); // log the failed node for rebinding during the next round
			}
			nextImport: ;
		} // per-import loop
		if (newImportList.size() == importList.size()) { // if the import table has stabilized
			for (vector<SymbolTable *>::iterator importIter = newImportList.begin(); importIter != newImportList.end(); importIter++) {
				Token curToken = (*importIter)->defSite->t;
				string importPath = *((*importIter)->defSite->child->next);
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
	if (root->kind == KIND_DECLARATION || root->kind == KIND_PARAMETER || root->kind == KIND_CONSTRUCTOR) { // if it's a named node, derive its type
		getStatusSymbolTable(root);
	}
	// recurse on this node's children
	for (vector<SymbolTable *>::const_iterator iter = root->children.begin(); iter != root->children.end(); iter++) {
		typeSt(*iter);
	}
}

TypeStatus getStatusSymbolTable(SymbolTable *st, const TypeStatus &inStatus) {
	Tree *tree = st->defSite; // set up the tree varaible that the header expects
	GET_STATUS_HEADER;
	if (st->kind == KIND_DECLARATION) { // if the symbol was defined as a Declaration-style node
		status = getStatusDeclaration(tree, inStatus);
	} else if (st->kind == KIND_PARAMETER) { // else if the symbol was defined as a Param-style node
		status = getStatusParam(tree, inStatus); // Param
	} else if (st->kind == KIND_CONSTRUCTOR) { // else if the symbol was defined as a Constructor-style node
		status = getStatusConstructor(tree, inStatus); // Constructor
	}
	GET_STATUS_FOOTER;
}

// typing function definitions

// reports errors
TypeStatus getStatusSuffixedIdentifier(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	string id = *tree; // string representation of this identifier
	SymbolTable *st = bindId(id, tree->env, inStatus);
	if (st != NULL) { // if we found a binding
		TypeStatus stStatus = getStatusSymbolTable(st, inStatus);
		if (*stStatus) { // if we successfully extracted a type for this SymbolTable entry
			status = TypeStatus(stStatus, tree);
		}
	} else { // else if we couldn't find a binding
		Token curToken = tree->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve '"<<id<<"'");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPrefixOrMultiOp(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *pomocc = tree->child->child;
	TypeStatus subStatus = getStatusPrimary(tree->next, inStatus);
	if (*pomocc == TOKEN_NOT) {
		if (*subStatus == STD_BOOL) {
			status = subStatus;
		}
	} else if (*pomocc == TOKEN_COMPLEMENT) {
		if (*subStatus == STD_INT) {
			status = subStatus;
		}
	} else if (*pomocc == TOKEN_DPLUS) {
		if (*subStatus == STD_INT) {
			status = subStatus;
		}
	} else if (*pomocc == TOKEN_DMINUS) {
		if (*subStatus == STD_INT) {
			status = subStatus;
		}
	} else if (*pomocc == TOKEN_PLUS) {
		if (*subStatus == STD_INT || *subStatus == STD_FLOAT) {
			status = subStatus;
		}
	} else if (*pomocc == TOKEN_MINUS) {
		if (*subStatus == STD_INT || *subStatus == STD_FLOAT) {
			status = subStatus;
		}
	}
	GET_STATUS_FOOTER;
}

// reports errors for TOKEN_SLASH case
TypeStatus getStatusPrimary(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *primaryc = tree->child;
	if (*primaryc == TOKEN_SuffixedIdentifier) {
		status = getStatusSuffixedIdentifier(primaryc, inStatus);
	} else if (*primaryc == TOKEN_SingleAccessor) { // if it's an accessed term
		// first, derive the subtype
		Tree *subSI = primaryc->next; // SuffixedIdentifier
		TypeStatus subStatus = getStatusSuffixedIdentifier(subSI, inStatus); // SuffixedIdentifier
		if (*subStatus) { // if we successfully derived a subtype
			if ((*subStatus).suffix != SUFFIX_CONSTANT) { // if the derived type is a latch or a stream
				// copy the Type so that our mutations don't propagate to the SuffixedIdentifier
				TypeStatus mutableSubStatus = subStatus;
				mutableSubStatus.type = mutableSubStatus.type->copy();
				// next, make sure the subtype is compatible with the accessor
				Tree *accessorc = primaryc->child; // SLASH, SSLASH, or ASLASH
				if (*accessorc == TOKEN_SLASH) {
					if (mutableSubStatus.type->delatch()) {
						status = mutableSubStatus;
					} else {
						Token curToken = accessorc->t;
						semmerError(curToken.fileName,curToken.row,curToken.col,"delatch of incompatible type");
						semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<subStatus<<")");
						mutableSubStatus.type->erase();
					}
				} else if (*accessorc == TOKEN_SSLASH) {
					if (mutableSubStatus.type->copyDelatch()) {
						status = mutableSubStatus;
					} else {
						Token curToken = accessorc->t;
						semmerError(curToken.fileName,curToken.row,curToken.col,"copy delatch of incompatible type");
						semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<subStatus<<")");
						mutableSubStatus.type->erase();
					}
				} else if (*accessorc == TOKEN_ASLASH) {
					if (mutableSubStatus.type->constantDelatch()) {
						status = mutableSubStatus;
					} else {
						Token curToken = accessorc->t;
						semmerError(curToken.fileName,curToken.row,curToken.col,"constant delatch of incompatible type");
						semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<subStatus<<")");
						mutableSubStatus.type->erase();
					}
				}
			} else { // else if the derived type isn't a latch or stream (and thus can't be delatched), error
				Token curToken = primaryc->t;
				semmerError(curToken.fileName,curToken.row,curToken.col,"delatching non-latch, non-stream '"<<subSI<<"'");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<inStatus<<")");
			}
		}
	} else if (*primaryc == TOKEN_PrimLiteral) {
		status = getStatusPrimLiteral(primaryc, inStatus);
	} else if (*primaryc == TOKEN_PrefixOrMultiOp) {
		status = getStatusPrefixOrMultiOp(primaryc, inStatus);
	} else if (*primaryc == TOKEN_LBRACKET) {
		status = getStatusExp(primaryc->next, inStatus);
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusBracketedExp(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *exp = tree->child->next; // Exp
	return getStatusExp(exp, inStatus);
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusExp(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// derive the correct type of this expression
	Tree *expc = tree->child;
	if (*expc == TOKEN_Primary) {
		status = getStatusPrimary(expc, inStatus);
	} else if (*expc == TOKEN_Exp) {
		Tree *expLeft = expc;
		Tree *op = expLeft->next;
		Tree *expRight = op->next;
		TypeStatus left = getStatusExp(expLeft, inStatus);
		TypeStatus right = getStatusExp(expRight, inStatus);
		switch (op->t.tokenType) {
			case TOKEN_DOR:
			case TOKEN_DAND:
				if (*left == STD_BOOL && *right == STD_BOOL) {
					status = new StdType(STD_BOOL, SUFFIX_LATCH);
				}
				break;
			case TOKEN_OR:
			case TOKEN_XOR:
			case TOKEN_AND:
				if (*left == STD_INT && *right == STD_INT) {
					status = new StdType(STD_INT, SUFFIX_LATCH);
				}
				break;
			case TOKEN_DEQUALS:
			case TOKEN_NEQUALS:
			case TOKEN_LT:
			case TOKEN_GT:
			case TOKEN_LE:
			case TOKEN_GE:
				if (left->isComparable(*right)) {
					status = new StdType(STD_BOOL, SUFFIX_LATCH);
				}
				break;
			case TOKEN_LS:
			case TOKEN_RS:
				if (*left == STD_INT && *right == STD_INT) {
					status = new StdType(STD_INT, SUFFIX_LATCH);
				}
				break;
			case TOKEN_TIMES:
			case TOKEN_DIVIDE:
			case TOKEN_MOD:
			case TOKEN_PLUS:
			case TOKEN_MINUS:
				if ((*left == STD_INT || *left == STD_FLOAT) && (*right == STD_INT || *right == STD_FLOAT)) {
					if (*left != STD_FLOAT && *right != STD_FLOAT) {
						status = new StdType(STD_INT, SUFFIX_LATCH);
					} else {
						status = new StdType(STD_FLOAT, SUFFIX_LATCH);
					}
				}
				break;
			default: // can't happen; the above should cover all cases
				break;
		} // switch
	} // if
	if (!status) { // if we couldn't resolve a type for this expression
		Token curToken = tree->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve expression's type");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPrimOpNode(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *ponc = tree->child->child; // the operator token itself
	// generate the type based on the specific operator it is
	switch (ponc->t.tokenType) {
		case TOKEN_NOT:
			status = new StdType(STD_NOT, SUFFIX_LATCH);
			break;
		case TOKEN_COMPLEMENT:
			status = new StdType(STD_COMPLEMENT, SUFFIX_LATCH);
			break;
		case TOKEN_DPLUS:
			status = new StdType(STD_DPLUS, SUFFIX_LATCH);
			break;
		case TOKEN_DMINUS:
			status = new StdType(STD_DMINUS, SUFFIX_LATCH);
			break;
		case TOKEN_DOR:
			status = new StdType(STD_DOR, SUFFIX_LATCH);
			break;
		case TOKEN_DAND:
			status = new StdType(STD_DAND, SUFFIX_LATCH);
			break;
		case TOKEN_OR:
			status = new StdType(STD_OR, SUFFIX_LATCH);
			break;
		case TOKEN_XOR:
			status = new StdType(STD_XOR, SUFFIX_LATCH);
			break;
		case TOKEN_AND:
			status = new StdType(STD_AND, SUFFIX_LATCH);
			break;
		case TOKEN_DEQUALS:
			status = new StdType(STD_DEQUALS, SUFFIX_LATCH);
			break;
		case TOKEN_NEQUALS:
			status = new StdType(STD_NEQUALS, SUFFIX_LATCH);
			break;
		case TOKEN_LT:
			status = new StdType(STD_LT, SUFFIX_LATCH);
			break;
		case TOKEN_GT:
			status = new StdType(STD_GT, SUFFIX_LATCH);
			break;
		case TOKEN_LE:
			status = new StdType(STD_LE, SUFFIX_LATCH);
			break;
		case TOKEN_GE:
			status = new StdType(STD_GE, SUFFIX_LATCH);
			break;
		case TOKEN_LS:
			status = new StdType(STD_LS, SUFFIX_LATCH);
			break;
		case TOKEN_RS:
			status = new StdType(STD_RS, SUFFIX_LATCH);
			break;
		case TOKEN_TIMES:
			status = new StdType(STD_TIMES, SUFFIX_LATCH);
			break;
		case TOKEN_DIVIDE:
			status = new StdType(STD_DIVIDE, SUFFIX_LATCH);
			break;
		case TOKEN_MOD:
			status = new StdType(STD_MOD, SUFFIX_LATCH);
			break;
		case TOKEN_PLUS:
			status = new StdType(STD_PLUS, SUFFIX_LATCH);
			break;
		case TOKEN_MINUS:
			status = new StdType(STD_MINUS, SUFFIX_LATCH);
			break;
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPrimLiteral(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *plc = tree->child;
	if (*plc == TOKEN_INUM) {
		status = new StdType(STD_INT, SUFFIX_LATCH);
	} else if (*plc == TOKEN_FNUM) {
		status = new StdType(STD_FLOAT, SUFFIX_LATCH);
	} else if (*plc == TOKEN_CQUOTE) {
		status = new StdType(STD_CHAR, SUFFIX_LATCH);
	} else if (*plc == TOKEN_SQUOTE) {
		status = new StdType(STD_STRING, SUFFIX_LATCH);
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusBlock(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	bool pipeTypesValid = true;
	TypeStatus curStatus = inStatus;
	for (Tree *pipe = tree->child->next->child; pipe != NULL; pipe = (pipe->next != NULL) ? pipe->next->child : NULL) { // Pipe or LastPipe
		// try to get a type for this pipe
		TypeStatus thisPipeStatus = getStatusPipe(pipe, curStatus);
		if (*thisPipeStatus) { // if we successfully derived a type for this Pipe, log its return type into the current status
			curStatus.retType = thisPipeStatus.retType;
		} else { // else if we failed to derive a type for this Pipe, flag this fact
			pipeTypesValid = false;
		}
	}
	if (pipeTypesValid) { // if we managed to derive a type for all of the enclosed pipes, set the return status to be the appropriate filter type
		if (curStatus.retType == NULL) { // if there were no returns in this block, set this block as returning the null type
			curStatus.retType = nullType;
		}
		status = new FilterType(inStatus, curStatus.retType, SUFFIX_LATCH);
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusFilterHeader(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	TypeStatus from = TypeStatus(nullType, inStatus);
	TypeStatus to = TypeStatus(nullType, inStatus);
	Tree *treeCur = tree->child->next; // ParamList, RetList, or RSQUARE
	if (*treeCur == TOKEN_ParamList) {
		from = getStatusParamList(treeCur, inStatus);
		// advance to handle the possible RetList
		treeCur = treeCur->next; // RetList or RSQUARE
	}
	if (*treeCur == TOKEN_RetList) {
		to = getStatusTypeList(treeCur->child->next, inStatus); // TypeList
	}
	if (*from && *to) { // if we succeeded in deriving both the from- and to- statuses
		status = new FilterType(from, to, SUFFIX_LATCH);
		status = tree;
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusFilter(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// fake a type for this node in order to allow for recursion
	Type *&fakeType = tree->status.type;
	fakeType = new FilterType(nullType, nullType, SUFFIX_LATCH);
	TypeStatus startStatus = inStatus; // the status that we're going to feed into the Block subnode derivation
	startStatus.retType = NULL; // make no initial presuppositions about what type the Block should return
	// derive the declared type of the filter
	Tree *filterCur = tree->child; // Block or FilterHeader
	if (*filterCur == TOKEN_Block) { // if it's an implicit block-defined filter, its type is a consumer of the input type
		((FilterType *)fakeType)->from = (inStatus.type->category == CATEGORY_TYPELIST) ? ((TypeList *)inStatus.type) : new TypeList(inStatus.type);
	} else if (*filterCur == TOKEN_FilterHeader) { // else if it's an explicit header-defined filter, its type is the type of the header
		TypeStatus tempStatus = getStatusFilterHeader(filterCur, inStatus); // derive the (possibly recursive) type of the filter header
		if (*tempStatus) { // if we successfully derived a type for the header
			// log the derived type into the fake type that we previously created
			((FilterType *)fakeType)->from = ((FilterType *)(tempStatus.type))->from;
			((FilterType *)fakeType)->to = ((FilterType *)(tempStatus.type))->to;
			// nullify the incoming type, since we have an explicit parameter list
			startStatus = nullType;
			// advance to the Block definition node
			filterCur = filterCur->next;
		} else { // else if we failed to derive a type for the header, delete the fake type and set it to be erroneous
			delete fakeType; // delete the fake type
			fakeType = errType;
		}
	}
	if (*fakeType) { // if we successfully derived a type for the header, verify the filter definition Block
		TypeStatus blockStatus = getStatusBlock(filterCur, startStatus); // derive the definition Block's Type
		if (*blockStatus) { // if we successfully derived a type for the definition Block (meaning there were no return type violations)
			if (*( ((FilterType *)(blockStatus.type))->to ) == *( ((FilterType *)fakeType)->to )) { // if the header and Block return types match
				// log the header type as the return status
				status = fakeType;
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
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusConstructor(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *conscn = tree->child->next; // NULL, SEMICOLON, LSQUARE, or NonRetFilterHeader
	if (conscn == NULL || *conscn == TOKEN_SEMICOLON || *conscn == TOKEN_LSQUARE) {
		status = TypeStatus(new FilterType(nullType, nullType, SUFFIX_LATCH), inStatus);
	} else if (*conscn == TOKEN_NonRetFilterHeader) {
		TypeStatus headerStatus = getStatusFilterHeader(conscn, inStatus);
		if (*headerStatus) { // if we managed to derive a type for the header
			// fake a type for this node in order to allow for recursion in the upcoming Block
			Type *&fakeType = tree->status.type;
			fakeType = headerStatus.type;
			// verify the constructor definition Block
			Tree *block = conscn->next; // Block
			TypeStatus startStatus = inStatus;
			startStatus.retType = errType; // ensure that the Block does not return anything
			TypeStatus blockStatus = getStatusBlock(block, startStatus); // derive the definition Block's Type
			if (*blockStatus) { // if we successfully derived a type for the definition Block, log the header as the return status
				status = headerStatus;
			} else { // else if we failed to derive a type for the Block
				delete fakeType; // delete the fake type
				fakeType = errType;
			}
		} else { // else if we failed to derive a type for the header, flag an error
			Token curToken = conscn->child->t; // LSQUARE
			semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve constructor's header type");
		}
	}
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
	Tree *conss = tree->child->next; // Constructors
	Tree *pipes = conss->next; // NonEmptyPipes or RCURLY
	for (Tree *pipe = (*pipes == TOKEN_NonEmptyPipes) ? pipes->child : NULL; pipe != NULL; pipe = (pipe->next != NULL) ? pipe->next->child : NULL) { // Pipe or LastPipe
		Tree *pipec = pipe->child; // Declaration, NonEmptyTerms, or LastDeclaration
		if (*pipec == TOKEN_Declaration || *pipec == TOKEN_LastDeclaration) { // if it's a member declaration
			// check for naming conflicts with this member
			string &stringToAdd = pipec->child->t.s; // ID
			vector<string>::iterator iter1;
			vector<Token>::iterator iter2;
			for (iter1 = memberNames.begin(), iter2 = memberTokens.begin(); iter1 != memberNames.end(); iter1++, iter2++) {
				if (*iter1 == stringToAdd) {
					break;
				}
			}
			if (iter1 == memberNames.end()) { // if there were no naming conflicts with this member
				memberNames.push_back(stringToAdd);
				memberTypes.push_back(NULL);
				memberDefSites.push_back(pipec);
				memberTokens.push_back(pipec->child->t); // ID
			} else { // else if there was a naming conflict with this member
				Token curDefToken = pipec->child->t;
				Token prevDefToken = *iter2;
				semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate definition of object member '"<<stringToAdd<<"'");
				semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
				failed = true;
			}
		}
	}
	// log the fake member names and types, as well as their definition sites
	((ObjectType *)fakeType)->memberNames = memberNames;
	((ObjectType *)fakeType)->memberTypes = memberTypes;
	((ObjectType *)fakeType)->memberDefSites = memberDefSites;
	// derive types for all of the contructors
	vector<TypeList *> constructorTypes;
	vector<Token> constructorTokens;
	for (Tree *cons = conss->child; cons != NULL; cons = (cons->next != NULL) ? cons->next->child : NULL) {
		TypeStatus consStatus = getStatusConstructor(cons, inStatus); // Constructor
		if (*consStatus) { // if we successfully derived a type for this constructor
			// check if there's already a constructor of this type
			vector<TypeList *>::iterator iter1;
			vector<Token>::iterator iter2;
			for (iter1 = constructorTypes.begin(), iter2 = constructorTokens.begin(); iter1 != constructorTypes.end(); iter1++, iter2++) {
				if (**iter1 == *consStatus) {
					break;
				}
			}
			if (iter1 == constructorTypes.end()) { // if there were no conflicts, add the constructor's type to the list
				constructorTypes.push_back((TypeList *)(consStatus.type));
				constructorTokens.push_back(cons->child->t); // EQUALS
			} else { // otherwise, flag the conflict as an error
				Token curDefToken = cons->child->t; // EQUALS
				Token prevDefToken = *iter2;
				semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate object constructor of type "<<consStatus);
				semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
				failed = true;
			}
		} else { // otherwise, if we failed to derive a type for this constructor
			failed = true;
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
		// log the derived list into the fake type that we previously created
		((ObjectType *)fakeType)->constructorTypes = constructorTypes;
		// propagate the change to all copies
		((ObjectType *)fakeType)->propagateToCopies();
		// finally, log the completed type as the return status
		status = fakeType;
	} else { // else if we failed to derive the lists, delete the fake type
		delete fakeType;
	}
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
			StdType stdIntType(STD_INT); // temporary integer type for comparison
			if (!(*(*expStatus >> stdIntType))) { // if the expression is incompatible with an integer, flag a bad expression error
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
			StdType stdIntType(STD_INT); // temporary integer type for comparison
			if (!(*(*expStatus >> stdIntType))) { // if the expression is incompatible with an integer, flag a bad expression error
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
		Tree *typec = tree->child; // NonArraySuffixedIdentifier, FilterType, or ObjectType
		if (*typec == TOKEN_NonArraySuffixedIdentifier) { // if it's an identifier-defined type
			TypeStatus idStatus = getStatusSuffixedIdentifier(typec, inStatus); // NonArraySuffixedIdentifier
			if (*idStatus) {
				if (idStatus->suffix == SUFFIX_CONSTANT || idStatus->suffix == SUFFIX_LATCH) { // if the suffix is valid for instantiating a node out of
					idStatus->suffix = suffixVal;
					idStatus->depth = depthVal;
					status = idStatus;
				} else { // else if the suffix isn't valid for instantiating a node out of
					Token curToken = typec->child->t; // ID
					semmerError(curToken.fileName,curToken.row,curToken.col,"instantiation from a non-constant, non-latch node");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (instantiation type is "<<idStatus<<")");
				}
			}
		} else if (*typec == TOKEN_FilterType) { // else if it's an in-place-defined filter type
			TypeStatus from = inStatus;
			TypeStatus to = inStatus;
			Tree *sub = typec->child->next; // TypeList, RetList, or RSQUARE
			if (*sub == TOKEN_TypeList) { // if there was a from-list
				from = getStatusTypeList(sub, inStatus); // TypeList
				// advance (in order to properly handle the possible trailing RetList)
				sub = sub->next; // RetList or RSQUARE
			} else if (*sub == TOKEN_RetList) { // else if there was no from-list, but there's a RetList
				from = new TypeList();
			} else { // else if there was no from-list or RetList (it's a null filter)
				from = new TypeList();
				sub = NULL;
			}
			if (sub != NULL && *sub == TOKEN_RetList) { // if there was a RetList
				to = getStatusTypeList(sub->child->next, inStatus); // TypeList
			} else { // else if there was no RetList
				to = new TypeList();
			}
			status = new FilterType(from, to, suffixVal, depthVal);
		} else if (*typec == TOKEN_ObjectType) { // else if it's an in-place-defined object type
			Tree *otcn = typec->child->next; // RCURLY or ObjectTypeList
			if (*otcn == TOKEN_RCURLY) { // if it's a blank object type
				status = new ObjectType(suffixVal, depthVal);
			} else if (*otcn == TOKEN_ObjectTypeList) { // else if it's a custom-defined object type


				// KOL make this work with memberDefSites, as done in getStatusObject
				vector<Tree *> memberDefSites;


				vector<TypeList *> constructorTypes;
				vector<Token> constructorTokens;
				bool failed = false;
				Tree *cur;
				for(cur = otcn->child; cur != NULL && *cur == TOKEN_ConstructorType; cur = (cur->next != NULL) ? cur->next->next->child : NULL) { // invariant: cur is a ConstructorType
					TypeStatus consStatus;
					if (cur->child->next == NULL || cur->child->next->next == NULL) { // if it's an implicitly null constructor, log it as such
						consStatus = TypeStatus(new TypeList(), inStatus);
					} else { // else if it's an explicitly described constructor, get its type from the subnode
						consStatus = getStatusTypeList(cur->child->next->next, inStatus); // TypeList
					}
					if (*consStatus) { // if we successfully derived a type for this constructor
						// check if there's already a constructor of this type
						vector<TypeList *>::iterator iter1;
						vector<Token>::iterator iter2;
						for (iter1 = constructorTypes.begin(), iter2 = constructorTokens.begin(); iter1 != constructorTypes.end(); iter1++, iter2++) {
							if (**iter1 == *consStatus) {
								break;
							}
						}
						if (iter1 == constructorTypes.end()) { // if there were no conflicts, add the constructor's type to the list
							constructorTypes.push_back((TypeList *)(consStatus.type));
							constructorTokens.push_back(cur->child->t); // EQUALS
						} else { // otherwise, flag the conflict as an error
							Token curDefToken = cur->child->t; // EQUALS
							Token prevDefToken = *iter2;
							semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate object constructor of type "<<consStatus);
							semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
							failed = true;
						}
					} else { // otherwise, if we failed to derive a type for this constructor
						failed = true;
					}
				}
				// cur is now a MemberList or NULL
				vector<string> memberNames;
				vector<Type *> memberTypes;
				vector<Token> memberTokens;
				for(cur = (cur != NULL) ? cur->child : NULL; cur != NULL; cur = (cur->next != NULL) ? cur->next->next->child : NULL) { // invariant: cur is a MemberType
					// check for naming conflicts with this member
					string &stringToAdd = cur->child->t.s; // ID
					vector<string>::iterator iter1;
					vector<Token>::iterator iter2;
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
							memberTokens.push_back(cur->child->t); // ID
						} else { // else if we failed to derive a type
							failed = true;
						}
						
					} else { // else if there was a naming conflict with this member
						Token curDefToken = cur->child->t;
						Token prevDefToken = *iter2;
						semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate definition of object member '"<<stringToAdd<<"'");
						semmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)");
						failed = true;
					}
				}
				if (!failed) {
					status = new ObjectType(constructorTypes, memberNames, memberTypes, memberDefSites, suffixVal, depthVal);
				}
			}
		}
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusTypeList(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	vector<Type *> list;
	Tree *treeCur = tree;
	bool failed = false;
	for(;;) { // invariant: treeCur is a TypeList
		Tree *type = treeCur->child; // Type
		TypeStatus curTypeStatus = getStatusType(type, inStatus);
		if (*curTypeStatus) { // if we successfully derived a type for this node
			// commit the type to the list
			list.push_back(curTypeStatus.type);
		} else { // else if we failed to derive a type for this node
			failed = true;
		}
		// advance
		if (treeCur->child->next != NULL) {
			treeCur = treeCur->child->next->next;
		} else {
			break;
		}
	}
	if (!failed) {
		status = new TypeList(list);
		status = tree;
	}
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
		status = getStatusType(tree->child, inStatus); // Type
	}
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
		status = new TypeList(list);
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusNodeInstantiation(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *it = tree->child->next; // InstantiableType
	TypeStatus instantiation = getStatusType(it, inStatus); // InstantiableType (compatible as a Type)
	if (*instantiation) { // if we successfully derived a type for the instantiation
		if (it->next->next != NULL) { // if there's an initializer, we need to make sure that the types are compatible
			Tree *st = it->next->next->next; // StaticTerm
			TypeStatus initializer = getStatusStaticTerm(st, inStatus);
			if (*initializer) { //  if we successfully derived a type for the initializer
				// pipe the types into the status
				Type *result = (*initializer >> *instantiation);
				if (*result) {
					status = TypeStatus(instantiation, instantiation);
				} else { // if the types are incompatible, throw an error
					Token curToken = st->t;
					semmerError(curToken.fileName,curToken.row,curToken.col,"incompatible initializer");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (instantiation type is "<<instantiation<<")");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (initializer type is "<<initializer<<")");
				}
			}
		} else { // else if there is no initializer, simply set the status to be the type
			status = TypeStatus(instantiation, instantiation);
		}
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusNode(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *nodec = tree->child;
	if (*nodec == TOKEN_SuffixedIdentifier) {
		status = getStatusSuffixedIdentifier(nodec, inStatus); // erroneous recursion handled by Declaration derivation
	} else if (*nodec == TOKEN_NodeInstantiation) {
		status = getStatusNodeInstantiation(nodec, inStatus); // erroneous recursion handled by Declaration derivation
	} else if (*nodec == TOKEN_Filter) {
		status = getStatusFilter(nodec, inStatus); // allows for recursive definitions
	} else if (*nodec == TOKEN_Object) {
		status = getStatusObject(nodec, inStatus); // allows for recursive definitions
	} else if (*nodec == TOKEN_PrimOpNode) {
		status = getStatusPrimOpNode(nodec, inStatus); // can't possibly be recursive
	} else if (*nodec == TOKEN_PrimLiteral) {
		status = getStatusPrimLiteral(nodec, inStatus); // can't possibly be recursive
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusTypedStaticTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *tstc = tree->child;
	if (*tstc == TOKEN_Node) {
		status = getStatusNode(tstc, inStatus);
	} else if (*tstc == TOKEN_LBRACKET) { // it's an expression
		status = getStatusExp(tstc->next, inStatus); // move past the bracket to the actual Exp node
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusStaticTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *stc = tree->child;
	if (*stc == TOKEN_TypedStaticTerm) {
		TypeStatus tstStatus = getStatusTypedStaticTerm(stc, inStatus);
		if (*tstStatus) { // if we managed to derive a type for the subnode, log it as the return status
			status = tstStatus;
		}
	} else if (*stc == TOKEN_Access) {
		// first, derive the Type of the Node that we're acting upon
		TypeStatus nodeStatus = getStatusNode(stc->child->next, inStatus); // Node
		if (*nodeStatus) { // if we managed to derive a type for the subnode
			// copy the Type so that our mutations don't propagate to the Node
			TypeStatus mutableNodeStatus = nodeStatus;
			mutableNodeStatus.type = mutableNodeStatus.type->copy();
			// finally, do the mutation and see check it it worked
			Tree *accessorc = stc->child->child; // SLASH, SSLASH, ASLASH, DSLASH, DSSLASH, or DASLASH
			if (*accessorc == TOKEN_SLASH) {
				if (mutableNodeStatus.type->delatch()) {
					status = mutableNodeStatus;
				} else {
					Token curToken = accessorc->t;
					semmerError(curToken.fileName,curToken.row,curToken.col,"delatch of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			} else if (*accessorc == TOKEN_SSLASH) {
				if (mutableNodeStatus.type->copyDelatch()) {
					status = mutableNodeStatus;
				} else {
					Token curToken = accessorc->t;
					semmerError(curToken.fileName,curToken.row,curToken.col,"copy delatch of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			} else if (*accessorc == TOKEN_ASLASH) {
				if (mutableNodeStatus.type->constantDelatch()) {
					status = mutableNodeStatus;
				} else {
					Token curToken = accessorc->t;
					semmerError(curToken.fileName,curToken.row,curToken.col,"constant delatch of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			} else if (*accessorc == TOKEN_DSLASH) {
				if (mutableNodeStatus.type->destream()) {
					status = mutableNodeStatus;
				} else {
					Token curToken = accessorc->t;
					semmerError(curToken.fileName,curToken.row,curToken.col,"destream of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			} else if (*accessorc == TOKEN_DSSLASH) {
				if (mutableNodeStatus.type->copyDestream()) {
					status = mutableNodeStatus;
				} else {
					Token curToken = accessorc->t;
					semmerError(curToken.fileName,curToken.row,curToken.col,"copy destream of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			} else if (*accessorc == TOKEN_DASLASH) {
				if (mutableNodeStatus.type->constantDestream()) {
					status = mutableNodeStatus;
				} else {
					Token curToken = accessorc->t;
					semmerError(curToken.fileName,curToken.row,curToken.col,"constant destream of incompatible type");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<nodeStatus<<")");
					mutableNodeStatus.type->erase();
				}
			}
		}
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusDynamicTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *dtc = tree->child;
	if (*dtc == TOKEN_StaticTerm) {
		status = getStatusStaticTerm(dtc, inStatus);
	} else if (*dtc == TOKEN_Compound) {
		TypeStatus compoundStatus = getStatusStaticTerm(dtc->child->next, inStatus);
		if (*compoundStatus) { // if we successfully derived to compounding term's type
			Type *curType = inStatus;
			TypeList *curTypeList;
			if (curType->category == CATEGORY_TYPELIST) { // if the current type is already a TypeList, simply make a mutable copy of it
				curTypeList = (TypeList *)(curType->copy()); // create a mutable copy of the incoming Type
			} else { // else if the current type is not a TypeList, we must wrap it in one
				curTypeList = new TypeList(curType);
			}
			// finally, add the compounding term to the ongoing TypeList
			curTypeList->list.push_back(compoundStatus.type);
			status = curTypeList;
		}
	} else if (*dtc == TOKEN_Link) {
		TypeStatus linkStatus = getStatusStaticTerm(dtc->child->next);
		if (*linkStatus) { // if we managed to derive a type for the Link subnode
			Type *result = (*linkStatus >> *inStatus);
			if (*result) { // if the result is a successful link, log the success
				status = inStatus;
			} else {
				Token curToken = dtc->child->t; // DCOLON
				semmerError(curToken.fileName,curToken.row,curToken.col,"link with incompatible type");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (incoming type is "<<inStatus<<")");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (link type is "<<linkStatus<<")");
			}
		}
	} else if (*dtc == TOKEN_Send) {
		TypeStatus nodeStatus = getStatusNode(dtc->child->next, inStatus);
		Type *resultType = (*inStatus >> *nodeStatus);
		if (*resultType) { // if the Send is valid, proceed normally
			status = resultType;
		} else { // else if the Send is invalid, flag an error
			Token curToken = dtc->child->t; // RARROW
			semmerError(curToken.fileName,curToken.row,curToken.col,"send to incompatible type");
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (sent type is "<<inStatus<<")");
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (destination type is "<<nodeStatus<<")");
		}
	} else if (*dtc == TOKEN_Swap) {
		TypeStatus nodeStatus = getStatusNode(dtc->child->next, inStatus);
		Type *resultType = (*inStatus >> *nodeStatus);
		if (*resultType) { // if the Swap is valid, proceed normally
			status = nodeStatus.type; // inherit the type of the destination Node
		} else { // else if the Send is invalid, flag an error
			Token curToken = dtc->child->t; // LRARROW
			semmerError(curToken.fileName,curToken.row,curToken.col,"swap with incompatible type");
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (sent type is "<<inStatus<<")");
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (destination type is "<<nodeStatus<<")");
		}
	} else if (*dtc == TOKEN_Return) {
		Type *thisRetType = inStatus; // the type that we're returning, inferred from the incoming status
		Type *curRetType = inStatus.retType; // the current return type (i.e. the one we're expecting, or otherwise NULL)
		if (curRetType != NULL) { // if there's already a return type logged, make sure it matches this one
			if (*curRetType == *thisRetType) { // if the logged return type matches this one, proceed normally
				status.retType = curRetType;
				status = nullType;
			} else { // else if this return's type conflicts with a previous one
				Token curToken = dtc->child->t; // DRARROW
				semmerError(curToken.fileName,curToken.row,curToken.col,"return of unexpected type "<<thisRetType);
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (expected type is "<<curRetType<<")");
			}
		} else { // else if there is no return type logged, log this one and proceed normally
			status.retType = thisRetType;
			status = nullType;
		}
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusSwitchTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	vector<TypeStatus> toStatus; // vector for logging the destination statuses of each branch
	vector<Tree *> toTrees; // vector for logging the tree nodes of each branch
	 // LabeledPipes
	for (Tree *lpCur = tree->child->next->next;
			lpCur != NULL;
			lpCur = (lpCur->child->next->next != NULL && lpCur->child->next->next->next != NULL) ? lpCur->child->next->next->next : NULL) { // invariant: lpCur is a LabeledPipes
		Tree *lpc = lpCur->child; // StaticTerm or COLON
		// if there is a non-default label on this pipe, check its validity
		if (*lpc == TOKEN_StaticTerm) {
			// derive the label's type
			TypeStatus label = getStatusStaticTerm(lpc, inStatus);
			if (*inStatus != *label) { // if the type doesn't match, throw an error
				Token curToken = lpc->t;
				semmerError(curToken.fileName,curToken.row,curToken.col,"switch label type doesn't match input type");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (label type is "<<label<<")");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
			}
		}
		// derive the to-type of this label
		Tree *toTree = (*lpc == TOKEN_StaticTerm) ? lpc->next->next : lpc->next; // SimpleTerm
		TypeStatus to = getStatusSimpleTerm(toTree, inStatus);
		// log the to-type and to-tree of this label
		toStatus.push_back(to);
		toTrees.push_back(toTree);
	} // per-labeled pipe loop
	// verify that all of the to-types are the same
	TypeStatus firstToStatus = toStatus[0];
	Tree *firstToTree = toTrees[0];
	for (unsigned int i=1; i < toStatus.size(); i++) { // for each to-type
		TypeStatus to = toStatus[i];
		if (*to != *firstToStatus) { // if the types don't match, throw an error
			Tree *toTree = toTrees[i];
			Token curToken1 = toTree->t;
			Token curToken2 = firstToTree->t;
			semmerError(curToken1.fileName,curToken1.row,curToken1.col,"switch destination types are inconsistent");
			semmerError(curToken1.fileName,curToken1.row,curToken1.col,"-- (this type is "<<to<<")");
			semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (first type is "<<firstToStatus<<")");
		}
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusSimpleTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *stc = tree->child;
	if (*stc == TOKEN_DynamicTerm) {
		status = getStatusDynamicTerm(stc, inStatus);
	} else if (*stc == TOKEN_SwitchTerm) {
		status = getStatusSwitchTerm(stc, inStatus);
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusSimpleCondTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		status = getStatusTerm(tree->child->next, inStatus.recall->status);
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusClosedTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *ctc = tree->child;
	if (*ctc == TOKEN_SimpleTerm) {
		status = getStatusSimpleTerm(ctc, inStatus);
	} else if (*ctc == TOKEN_ClosedCondTerm) {
		status = getStatusClosedCondTerm(ctc, inStatus);
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusOpenTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *otc = tree->child;
	if (*otc == TOKEN_SimpleCondTerm) {
		status = getStatusSimpleCondTerm(otc, inStatus);
	} else if (*otc == TOKEN_OpenCondTerm) {
		status = getStatusOpenCondTerm(otc, inStatus);
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusOpenCondTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		Tree *trueBranch = tree->child->next;
		Tree *falseBranch = trueBranch->next->next;
		TypeStatus trueStatus = getStatusClosedTerm(trueBranch, inStatus.recall->status);
		TypeStatus falseStatus = getStatusOpenTerm(falseBranch, inStatus.recall->status);
		if (*trueStatus == *falseStatus) { // if the two branches match in type
			status = trueStatus;
		} else { // else if the two branches don't match in type
			Token curToken1 = tree->child->t; // QUESTION
			Token curToken2 = trueBranch->t; // ClosedTerm
			Token curToken3 = falseBranch->t; // OpenTerm
			semmerError(curToken1.fileName,curToken1.row,curToken1.col,"type mismatch in conditional operator branches");
			semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (true branch type is "<<trueStatus<<")");
			semmerError(curToken3.fileName,curToken3.row,curToken3.col,"-- (false branch type is "<<falseStatus<<")");
		}
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusClosedCondTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		Tree *trueBranch = tree->child->next;
		Tree *falseBranch = trueBranch->next->next;
		TypeStatus trueStatus = getStatusClosedTerm(trueBranch, inStatus.recall->status);
		TypeStatus falseStatus = getStatusClosedTerm(falseBranch, inStatus.recall->status);
		if (*trueStatus == *falseStatus) { // if the two branches match in type
			status = trueStatus;
		} else { // else if the two branches don't match in type
			Token curToken1 = tree->child->t; // QUESTION
			Token curToken2 = trueBranch->t; // ClosedTerm
			Token curToken3 = falseBranch->t; // ClosedTerm
			semmerError(curToken1.fileName,curToken1.row,curToken1.col,"type mismatch in conditional operator branches");
			semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (true branch type is "<<trueStatus<<")");
			semmerError(curToken3.fileName,curToken3.row,curToken3.col,"-- (false branch type is "<<falseStatus<<")");
		}
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusTerm(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *tcc = tree->child->child;
	if (*tcc == TOKEN_SimpleCondTerm) {
		status = getStatusSimpleCondTerm(tcc, inStatus);
	} else if (*tcc == TOKEN_OpenCondTerm) {
		status = getStatusOpenCondTerm(tcc, inStatus);
	} else if (*tcc == TOKEN_SimpleTerm) {
		status = getStatusSimpleTerm(tcc, inStatus);
	} else if (*tcc == TOKEN_ClosedCondTerm) {
		status = getStatusClosedCondTerm(tcc, inStatus);
	}
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
					*(curTerm->child->child->child) == TOKEN_DynamicTerm &&
					*(curTerm->child->child->child->child->child) == TOKEN_TypedStaticTerm &&
					*(curTerm->child->child->child->child->child->child) == TOKEN_Node) { // if it's a flow-through Term
				// derive a type for the flow of the current type into the next term in the sequence
				Type *flowResult = (*curStatus , *nextTermStatus);
				if (*flowResult) { // if the type flow is valid, log it as the current status
					curStatus = TypeStatus(flowResult, nextTermStatus);
				} else if (*curStatus == *nullType) { // else if the flow is not valid, but the incoming type is null, log the next term's status as the current one
					// but first, check if the Term needs to be constantized
					if (*(curTerm->child->child) == TOKEN_SimpleTerm &&
							*(curTerm->child->child->child) == TOKEN_DynamicTerm &&
							*(curTerm->child->child->child->child->child) == TOKEN_TypedStaticTerm &&
							*(curTerm->child->child->child->child->child->child->child) == TOKEN_SuffixedIdentifier) { // if the Term needs to be constantized
						// copy the Type so that our mutations don't propagate to the Term
						TypeStatus mutableNextTermStatus = nextTermStatus;
						mutableNextTermStatus.type = mutableNextTermStatus.type->copy();
						if (mutableNextTermStatus.type->constantize()) { // if the SuffixedIdentifier can be constantized, log it as the resulting status
							curStatus = mutableNextTermStatus;
						} else { // else if the SuffixedIdentifier cannot be constantized, flag an error
							Token curToken = curTerm->t; // Term
							semmerError(curToken.fileName,curToken.row,curToken.col,"constant reference to dynamic term");
							semmerError(curToken.fileName,curToken.row,curToken.col,"-- (term type is "<<nextTermStatus<<")");
							mutableNextTermStatus.type->erase();
						}
					} else { // else if the Term doesn't need to be constantized
						curStatus = nextTermStatus;
					}
				} else { // else if the type flow is not valid and the incoming type is not null, flag an error
					Token curToken = curTerm->t; // Term
					Token prevToken = prevTerm->t; // Term
					semmerError(curToken.fileName,curToken.row,curToken.col,"term does not accept incoming type");
					semmerError(prevToken.fileName,prevToken.row,prevToken.col,"-- (incoming type is "<<curStatus<<")");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (term's type is "<<nextTermStatus<<")");
					// short-circuit the derivation of this NonEmptyTerms
					curStatus = errType;
					break;
				}
			} else { // else if it's not a flow-through Term, log the next term's status as the current one
				curStatus  = nextTermStatus;
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
		status = curStatus;
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusDeclaration(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	// check if this is a recursive invocation
	Type *&fakeRetType = tree->status.retType;
	if (fakeRetType != NULL) { // if we previously logged a recursion alert here (and we don't have a memoized type to return), flag an ill-formed recursion error
		Token curToken = tree->child->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"irresolvable recursive definition of '"<<curToken.s<<"'");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<inStatus<<")");
	} else { // else if there is no recursion alert for this Declaration, continue
		// if the sub-node is not recursion-safe, institute a recursion warning for this Declaration
		Tree *declarationSub = tree->child->next->next; // TypedStaticTerm, NonEmptyTerms, or NULL
		if (!(declarationSub != NULL && *declarationSub == TOKEN_TypedStaticTerm && *(declarationSub->child) == TOKEN_Node &&
				(*(declarationSub->child->child) == TOKEN_Object || *(declarationSub->child->child) == TOKEN_Filter))) { // only Objects and Filters are exempt
			fakeRetType = errType; // log a recursion alert
		}
		// proceed with the normal derivation
		if (declarationSub != NULL) { // if it's a non-import declaration
			// attempt to derive the type of this Declaration
			if (*declarationSub == TOKEN_TypedStaticTerm) { // if it's a regular declaration
				Tree *tstc = declarationSub->child; // Node or LBRACKET
				if (*tstc == TOKEN_Node) { // if it's a node declaration, log its status as the return status
					TypeStatus nodeStatus = getStatusNode(tstc);
					// but first, check if the Node needs to be constantized
					if (*(tstc->child) == TOKEN_SuffixedIdentifier) { // if the Node needs to be constantized
						// copy the Type so that our mutations don't propagate to the Node
						TypeStatus mutableNodeStatus = nodeStatus;
						mutableNodeStatus.type = mutableNodeStatus.type->copy();
						if (mutableNodeStatus.type->constantize()) { // if the SuffixedIdentifier can be constantized, log it as the resulting status
							status = mutableNodeStatus;
						} else { // else if the SuffixedIdentifier cannot be constantized, flag an error
							Token curToken = tstc->t; // Node
							semmerError(curToken.fileName,curToken.row,curToken.col,"constant reference to dynamic term");
							semmerError(curToken.fileName,curToken.row,curToken.col,"-- (term type is "<<nodeStatus<<")");
							mutableNodeStatus.type->erase();
						}
					} else { // else if the Term doesn't need to be constantized
						status = nodeStatus;
					}
				} else if (*tstc == TOKEN_BracketedExp) { // else if it's a non-recursive expression declaration
					status = getStatusBracketedExp(tstc, inStatus);
				}
			} else if (*declarationSub == TOKEN_NonEmptyTerms) { // else if it's a regular flow-through declaration
				// first, set the identifier's type to the type of the NonEmptyTerms stream (an inputType consumer) in order to allow for recursion
				tree->status = new FilterType(inStatus, nullType, SUFFIX_LATCH);
				// then, verify types for the declaration sub-block
				status = getStatusNonEmptyTerms(declarationSub, inStatus);
				// delete the temporary filter type
				delete (tree->status.type);
			}
		} else { // otherwise, if it's an import declaration, do nothing; typing of the import will be handled at the definition site
			status = nullType;
		}
	}
	fakeRetType = NULL; // fix up the retType to serve its original purpose
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPipe(Tree *tree, const TypeStatus &inStatus) {
	GET_STATUS_HEADER;
	Tree *pipec = tree->child; // Declaration, NonEmptyTerms, or LastDeclaration
	if (*pipec == TOKEN_Declaration || *pipec == TOKEN_LastDeclaration) { // if it's a Declaration-style pipe
		status = getStatusDeclaration(pipec, inStatus);
	} else if (*pipec == TOKEN_NonEmptyTerms) { // else if it's a raw NonEmptyTerms pipe
		status = getStatusNonEmptyTerms(pipec, inStatus);
	}
	GET_STATUS_FOOTER;
}

void traceTypes(vector<Tree *> *parseme) {
	// iterate through all Pipe nodes
	vector<Tree *> &pipeList = parseme[TOKEN_Pipe];
	for (unsigned int i=0; i < pipeList.size(); i++) {
		Tree *pipeCur = pipeList[i];
		if (!(pipeCur->status)) { // if we haven't derived a type for this pipe yet
			getStatusPipe(pipeCur);
		}
	}
	// ... and all LastPipe nodes
	vector<Tree *> &lastPipeList = parseme[TOKEN_LastPipe];
	for (unsigned int i=0; i < lastPipeList.size(); i++) {
		Tree *lastPipeCur = lastPipeList[i];
		if (!(lastPipeCur->status)) { // if we haven't derived a type for this pipe yet
			getStatusPipe(lastPipeCur);
		}
	}
}

// main semming function; makes no assumptions about stRoot's value; it's just a return parameter
int sem(Tree *treeRoot, vector<Tree *> *parseme, SymbolTable *&stRoot) {

	// initialize error code
	semmerErrorCode = 0;

	VERBOSE( printNotice("Collecting tree nodes..."); )

	// initialize the symbol table root with the default definitions
	stRoot = genDefaultDefs();

	// populate the symbol table with definitions from the user parseme, and log the used imports/id instances
	vector<SymbolTable *> importList; // import Declaration nodes
	buildSt(treeRoot, stRoot, importList); // get user definitions/imports
	subImportDecls(importList); // resolve and substitute import declarations into the symbol table

	VERBOSE( printNotice("Tracing type flow..."); )

	// derive types of all identifiers in the SymbolTable
	typeSt(stRoot);
	// derive types for the remaining pipes
	traceTypes(parseme);
	
	VERBOSE( cout << stRoot; )

	// if there were no errors, free the error type node
	if (!semmerErrorCode) {
		delete errType;
	}

	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
