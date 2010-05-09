#include "semmer.h"

#include "customOperators.h"

// semmer-global variables

int semmerErrorCode;
bool semmerEventuallyGiveUp;

Type *nullType = new StdType(STD_NULL);
Type *errType = new ErrorType();

// SymbolTable functions

// allocators/deallocators
SymbolTable::SymbolTable(int kind, string id, Tree *defSite) : kind(kind), id(id), defSite(defSite), parent(NULL) {}

SymbolTable::SymbolTable(SymbolTable &st) {
	*this = st;
}

SymbolTable::~SymbolTable() {
	// delete all of the child nodes
	for (vector<SymbolTable *>::iterator childIter = children.begin(); childIter != children.end(); childIter++) {
		delete *childIter;
	}
}

// deep-copy assignment operator
SymbolTable &SymbolTable::operator=(SymbolTable &st) {
	kind = st.kind;
	id = st.id;
	defSite = st.defSite;
	parent = st.parent;
	for (vector<SymbolTable *>::iterator childIter = st.children.begin(); childIter != st.children.end(); childIter++) {
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
SymbolTable &SymbolTable::operator*=(SymbolTable *st) {
	// first, check for conflicting bindings
	if (st != NULL && st->id[0] != '_' && st->id[0] != '=') { // if this is not a special system-level binding
		// per-symbol loop
		for (vector<SymbolTable *>::iterator childIter = children.begin(); childIter != children.end(); childIter++) {
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
	*stRoot *= new SymbolTable(KIND_STD, "node");
	*stRoot *= new SymbolTable(KIND_STD, "int");
	*stRoot *= new SymbolTable(KIND_STD, "float");
	*stRoot *= new SymbolTable(KIND_STD, "bool");
	*stRoot *= new SymbolTable(KIND_STD, "char");
	*stRoot *= new SymbolTable(KIND_STD, "string");
}

void catStdOps(SymbolTable *&stRoot) {
	// prefix
	*stRoot *= new SymbolTable(KIND_STD, "!");
	*stRoot *= new SymbolTable(KIND_STD, "~");
	*stRoot *= new SymbolTable(KIND_STD, "++");
	*stRoot *= new SymbolTable(KIND_STD, "--");
	// infix
	*stRoot *= new SymbolTable(KIND_STD, "||");
	*stRoot *= new SymbolTable(KIND_STD, "&&");
	*stRoot *= new SymbolTable(KIND_STD, "|");
	*stRoot *= new SymbolTable(KIND_STD, "^");
	*stRoot *= new SymbolTable(KIND_STD, "&");
	*stRoot *= new SymbolTable(KIND_STD, "==");
	*stRoot *= new SymbolTable(KIND_STD, "!=");
	*stRoot *= new SymbolTable(KIND_STD, "<");
	*stRoot *= new SymbolTable(KIND_STD, ">");
	*stRoot *= new SymbolTable(KIND_STD, "<=");
	*stRoot *= new SymbolTable(KIND_STD, ">=");
	*stRoot *= new SymbolTable(KIND_STD, "<<");
	*stRoot *= new SymbolTable(KIND_STD, ">>");
	*stRoot *= new SymbolTable(KIND_STD, "*");
	*stRoot *= new SymbolTable(KIND_STD, "/");
	*stRoot *= new SymbolTable(KIND_STD, "%");
	// multi
	*stRoot *= new SymbolTable(KIND_STD, "+");
	*stRoot *= new SymbolTable(KIND_STD, "-");
}

void catStdLib(SymbolTable *&stRoot) {
	// standard root
	SymbolTable *stdLib = new SymbolTable(KIND_STD, STANDARD_LIBRARY_STRING);

	// system nodes
	// streams
	*stdLib *= new SymbolTable(KIND_STD, "in");
	*stdLib *= new SymbolTable(KIND_STD, "out");
	*stdLib *= new SymbolTable(KIND_STD, "err");
	// control nodes
	*stdLib *= new SymbolTable(KIND_STD, "rand");
	*stdLib *= new SymbolTable(KIND_STD, "delay");

	// standard library
	// containers
	*stdLib *= new SymbolTable(KIND_STD, "stack");
	*stdLib *= new SymbolTable(KIND_STD, "map");
	// filters
	*stdLib *= new SymbolTable(KIND_STD, "filter");
	*stdLib *= new SymbolTable(KIND_STD, "sort");
	// generators
	*stdLib *= new SymbolTable(KIND_STD, "gen");

	// concatenate the library to the root
	*stRoot *= stdLib;
}

SymbolTable *genDefaultDefs() {
	// generate the root block node
	SymbolTable *stRoot = new SymbolTable(KIND_BLOCK, BLOCK_NODE_STRING);
	// concatenate in the standard types
	catStdNodes(stRoot);
	// concatenate in the standard operators
	catStdOps(stRoot);
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
	// log the current symbol environment in the tree
	tree->env = st;
	// recursive cases
	if (*tree == TOKEN_Block || *tree == TOKEN_Object) { // if it's a block-style node
		// allocate the new block definition node
		SymbolTable *blockDef = new SymbolTable(KIND_BLOCK, BLOCK_NODE_STRING, tree);
		// finally, link the block node into the main trunk
		*st *= blockDef;
		// recurse
		buildSt(tree->child, blockDef, importList); // child of Block
	} else if (*tree == TOKEN_FilterHeader || *tree == TOKEN_NonRetFilterHeader) { // if it's a filter header node
		// locate the corresponding block and create an st node for it
		Tree *block = tree->next; // Block
		// allocate the new block definition node
		SymbolTable *blockDef = new SymbolTable(KIND_BLOCK, BLOCK_NODE_STRING, block);
		// finally, link the block node into the main trunk
		*st *= blockDef;
		// parse out the header's parameter declarations and add them to the st
		Tree *pl = tree->child->next; // ParamList or RetList
		if (*pl == TOKEN_ParamList) { // if there is a parameter list to process
			Tree *param = pl->child; // Param
			for (;;) { // per-param loop
				// allocate the new parameter definition node
				SymbolTable *paramDef = new SymbolTable(KIND_PARAM, param->child->next->t.s, param);
				// ... and link it into the block node
				*blockDef *= paramDef;
				// advance
				if (param->next != NULL) {
					param = param->next->next->child; // Param
				} else {
					break;
				}
			} // per-param loop
		} // if there is a parameter list to process
		// recurse
		buildSt(block->child, blockDef, importList); // child of Block
	} else if (*tree == TOKEN_Constructor) { // if it's a constructor node
		// allocate the new definition node
		SymbolTable *newDef = new SymbolTable(KIND_STATIC_DECL, tree->child->t.s, tree);
		// ... and link it in
		*st *= newDef;
		// recurse
		buildSt(tree->child, newDef, importList); // child of Constructor
		buildSt(tree->next, st, importList); // right
	} else if (*tree == TOKEN_Declaration) { // if it's a declaration node
		Tree *bnc = tree->child->next;
		if (*bnc == TOKEN_EQUALS) { // standard static declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_STATIC_DECL, tree->child->t.s, tree);
			// ... and link it in
			*st *= newDef;
			// recurse
			buildSt(tree->child, newDef, importList); // child of Declaration
		} else if (*bnc == TOKEN_ERARROW) { // flow-through declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_THROUGH_DECL, tree->child->t.s, tree);
			// ... and link it in
			*st *= newDef;
			// recurse
			buildSt(tree->child, newDef, importList); // child of Declaration
		} else if (*bnc == TOKEN_SuffixedIdentifier) { // import declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_IMPORT, IMPORT_DECL_STRING, tree);
			// ... and link it in
			*st *= newDef;
			// also, since it's an import declaration, log it to the import list
			importList.push_back(newDef);
			// recurse
			buildSt(tree->child, newDef, importList); // child of Declaration
		}
	} else { // else if it's not a declaration node
		// recurse normally
		buildSt(tree->child, st, importList); // down
		buildSt(tree->next, st, importList); // right
	}
}

// binds qualified identifiers in the given symtable environment; returns the tail of the binding
// also, updates id to contain the portion of the identifier that could not be bound
// returns NULL if no binding whatsoever can be found
SymbolTable *bindId(string &id, SymbolTable *env, TypeStatus inStatus = TypeStatus()) {
	// base case
	if (env == NULL) {
		return NULL;
	}
	// recall identifier case
	if (id[0] == '.' && id[1] == '.') {

		if (inStatus) { // if there is an input type
			// update id to be the rest of the recall identifier
			string idRest = "";
			if (id.length() >= 3) { // if this recall identifier has a rest-part
				idRest = id.substr(3, id.length()-3); // get the rest of the recall identifier string
			}
			id = idRest;

			// return the binding of the current input type, since that's what the recall identifier implicitly binds to
			if (*inStatus == *nullType) { // if the incoming type is null, we can't bind to it, so return an error
				return NULL;
			} else { // else if it's any other type, bind to it using its string representation
// LOL
				string recString = *inStatus; // statically get the string representation
				return bindId(recString, env); // statically bind to the specific standard node
			}
		} else { // else if there is no input type
			return NULL;
		}
	}
	// recursive case
	string tip = idHead(id);
	string idCur = idTail(id);
	SymbolTable *stCur = NULL;
	// scan the current environment's children for a latch point
	for (vector<SymbolTable *>::iterator latchIter = env->children.begin(); latchIter != env->children.end(); latchIter++) {
		if ((*latchIter)->id == tip) { // if we've found a latch point

			// verify that the latching holds for the rest of the identifier
			stCur = *latchIter;
			while (!idCur.empty()) { // while there's still more of the identifier to match

				// find a static match in the current st node's children
				SymbolTable *match = NULL;
				for (vector<SymbolTable *>::iterator stcIter = stCur->children.begin(); stcIter != stCur->children.end(); stcIter++) {
					string idCurHead = idHead(idCur);
					if ((*stcIter)->id == idCurHead) { // if the identifiers are the same, we have a match
						match = *stcIter;
						goto matchOK;

					// as a special case, look one block level deeper, since nested defs must be block-delimited
					} else if (stCur->kind != KIND_BLOCK && (*stcIter)->kind == KIND_BLOCK) {
						for (vector<SymbolTable *>::iterator blockIter = (*stcIter)->children.begin(); blockIter != (*stcIter)->children.end(); blockIter++) {
							if ((*blockIter)->id[0] != '_' && (*blockIter)->id == idCurHead) { // if the identifiers are the same, we have a match
								match = *blockIter;
								goto matchOK;
							}
						}
					}

				} matchOK: ; // match verification loop

				if (match != NULL) { // if we have a match for this part of the rest
					// advance to the matched st node
					stCur = match;
					// advance to the next token in the id
					idCur = idTail(idCur);
				} else { // else if we don't have a match for this part of the rest
					break;
				}

			}
			// no need to look through the rest of the children; we've already found the correctly named one on this level
			break;
		}
	} // per-latch point loop

	// if we've verified the entire identifier, return the tail of the latch point
	if (stCur != NULL && idCur.empty()) {
		id.clear(); // clear id to signify that the latching is complete
		return stCur; // return the tail of the latch point
	}

	// otherwise, compare the current binding with the alternate higher-level one
	// but first, scan up to the next block level, since jumping to the enclosing identifier is a waste of time
	SymbolTable *recurseSt = env->parent;
	while (recurseSt != NULL && recurseSt->kind != KIND_BLOCK) {
		recurseSt = recurseSt->parent;
	}
	string altId = id;
	if (stCur != NULL) { // if there was some sort of binding, update id to be the rest of it
		id = idCur;
	}

	// do the recursion
	SymbolTable *altResult = bindId(altId, recurseSt, inStatus);

	// check if the recursive solution is better
	if (altResult != NULL  && ((altId.length() < id.length()) || stCur == NULL)) { // if the other binding succeeded and is better, return it
		id = altId;
		return altResult;
	} else { // otherwise, return the local binding
		return stCur;
	}
}

void subImportDecls(vector<SymbolTable *> importList) {
	bool stdExplicitlyImported = false;

	for(;;) { // per-change loop
		// per-import loop
		vector<SymbolTable *> newImportList;
		for (vector<SymbolTable *>::iterator importIter = importList.begin(); importIter != importList.end(); importIter++) {
			// extract the import path out of the iterator
			string importPath = sid2String((*importIter)->defSite->child->next);
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
					if ((*childIter)->id[0] != '_' && (*childIter)->id == importPathTip) { // if there's a conflict
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
				string importPath = sid2String((*importIter)->defSite->child->next);
				semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve import '"<<importPath<<"'");
			}
			break;
		} else { // else if the import table hasn't stabilized yet, do another substitution round on the failed binding list
			importList = newImportList;
		}
	} // per-change loop
}

Type *getStType(SymbolTable *st) {
	if (st->defSite != NULL && st->defSite->status) { // if there is already a type logged for this st node, simply return it
		return st->defSite->status;
	} else { // else if we need to derive a type ourselves
// LOL
		return errType;
	}
}

// typing function definitions

// reports errors
TypeStatus getStatusSuffixedIdentifier(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	string id = sid2String(tree); // string representation of this identifier
	string idCur = id; // a destructible copy for the recursion
	SymbolTable *st = bindId(idCur, tree->env, inStatus);
	if (st != NULL) { // if we found a binding
		status = TypeStatus(getStType(st), tree);
	} else { // else if we couldn't find a binding
		Token curToken = tree->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve '"<<id<<"'");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPrefixOrMultiOp(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *pomocc = tree->child->child;
	TypeStatus sub = getStatusPrimary(tree->next, inStatus);
	if (*pomocc == TOKEN_NOT) {
		if (*sub == STD_BOOL) {
			status = sub;
		}
	} else if (*pomocc == TOKEN_COMPLEMENT) {
		if (*sub == STD_INT) {
			status = sub;
		}
	} else if (*pomocc == TOKEN_DPLUS) {
		if (*sub == STD_INT) {
			status = sub;
		}
	} else if (*pomocc == TOKEN_DMINUS) {
		if (*sub == STD_INT) {
			status = sub;
		}
	} else if (*pomocc == TOKEN_PLUS) {
		if (*sub == STD_INT || *sub == STD_FLOAT) {
			status = sub;
		}
	} else if (*pomocc == TOKEN_MINUS) {
		if (*sub == STD_INT || *sub == STD_FLOAT) {
			status = sub;
		}
	}
	GET_STATUS_FOOTER;
}

// reports errors for TOKEN_SLASH case
TypeStatus getStatusPrimary(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *primaryc = tree->child;
	if (*primaryc == TOKEN_SuffixedIdentifier) {
		status = getStatusSuffixedIdentifier(primaryc, inStatus);
	} else if (*primaryc == TOKEN_SLASH) { // if it's a delatched term
		Tree *subSI = primaryc->next; // SuffixedIdentifier
		TypeStatus sub = getStatusSuffixedIdentifier(subSI, inStatus); // SuffixedIdentifier
		if (*sub) { // if we successfully derived a subtype
			if ((*sub).suffix != SUFFIX_CONSTANT) { // if the derived type is a latch or a stream
				// copy the subtype
				status = new Type(*sub);
				// down-level the type
				status->delatch();
			} else { // else if the derived type isn't a latch or stream (and thus can't be delatched), error
				Token curToken = primaryc->t;
				semmerError(curToken.fileName,curToken.row,curToken.col,"delatching non-latch, non-stream '"<<sid2String(subSI)<<"'");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (type is "<<*inStatus<<")");
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

TypeStatus getStatusBracketedExp(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *exp = tree->child->next; // Exp
	return getStatusExp(exp, inStatus);
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusExp(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
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
					status = left.type;
				}
				break;
			case TOKEN_OR:
			case TOKEN_XOR:
			case TOKEN_AND:
				if (*left == STD_INT && *right == STD_INT) {
					status = left.type;
				}
				break;
			case TOKEN_DEQUALS:
			case TOKEN_NEQUALS:
			case TOKEN_LT:
			case TOKEN_GT:
			case TOKEN_LE:
			case TOKEN_GE:
				if (left->isComparable(*right)) {
					status = new Type(STD_BOOL);
				}
				break;
			case TOKEN_LS:
			case TOKEN_RS:
				if (*left == STD_INT && *right == STD_INT) {
					status = left.type;
				}
				break;
			case TOKEN_TIMES:
			case TOKEN_DIVIDE:
			case TOKEN_MOD:
			case TOKEN_PLUS:
			case TOKEN_MINUS:
				if ( (*left == STD_INT || *left == STD_FLOAT) &&
						(*right == STD_INT || *right == STD_FLOAT) ) {
					if (*right != STD_FLOAT) {
						status = left.type;
					} else {
						status = right.type;
					}
				}
				break;
			default: // can't happen
				left.type = NULL;
				right.type = NULL;
				break;
		} // switch
	} // if
	// if we couldn't resolve a type
	if (!status) {
		Token curToken = tree->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve expression's type");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPrimOpNode(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *ponc = tree->child->child; // the operator token itself
	// generate the type based on the specific operator it is
	switch (ponc->t.tokenType) {
		case TOKEN_NOT:
			status = new Type(STD_NOT);
			break;
		case TOKEN_COMPLEMENT:
			status = new Type(STD_COMPLEMENT);
			break;
		case TOKEN_DPLUS:
			status = new Type(STD_DPLUS);
			break;
		case TOKEN_DMINUS:
			status = new Type(STD_DMINUS);
			break;
		case TOKEN_DOR:
			status = new Type(STD_DOR);
			break;
		case TOKEN_DAND:
			status = new Type(STD_DAND);
			break;
		case TOKEN_OR:
			status = new Type(STD_OR);
			break;
		case TOKEN_XOR:
			status = new Type(STD_XOR);
			break;
		case TOKEN_AND:
			status = new Type(STD_AND);
			break;
		case TOKEN_DEQUALS:
			status = new Type(STD_DEQUALS);
			break;
		case TOKEN_NEQUALS:
			status = new Type(STD_NEQUALS);
			break;
		case TOKEN_LT:
			status = new Type(STD_LT);
			break;
		case TOKEN_GT:
			status = new Type(STD_GT);
			break;
		case TOKEN_LE:
			status = new Type(STD_LE);
			break;
		case TOKEN_GE:
			status = new Type(STD_GE);
			break;
		case TOKEN_LS:
			status = new Type(STD_LS);
			break;
		case TOKEN_RS:
			status = new Type(STD_RS);
			break;
		case TOKEN_TIMES:
			status = new Type(STD_TIMES);
			break;
		case TOKEN_DIVIDE:
			status = new Type(STD_DIVIDE);
			break;
		case TOKEN_MOD:
			status = new Type(STD_MOD);
			break;
		case TOKEN_PLUS:
			status = new Type(STD_PLUS);
			break;
		case TOKEN_MINUS:
			status = new Type(STD_MINUS);
			break;
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPrimLiteral(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *plc = tree->child;
	if (*plc == TOKEN_INUM) {
		status = new Type(STD_INT);
	} else if (*plc == TOKEN_FNUM) {
		status = new Type(STD_FLOAT);
	} else if (*plc == TOKEN_CQUOTE) {
		status = new Type(STD_CHAR);
	} else if (*plc == TOKEN_SQUOTE) {
		status = new Type(STD_STRING);
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusBlock(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *pipeCur = tree->child->next->child; // Pipe
	bool pipeTypesValid = true;
	while(pipeCur != NULL) {
		// try to get a type for this pipe
		TypeStatus result = getStatusPipe(pipeCur, inStatus);
		// if we failed to find a type, flag this fact
		if (!*result) {
			pipeTypesValid = false;
		}
		// advance
		if (pipeCur->next != NULL) {
			pipeCur = pipeCur->next->next->child; // Pipe
		} else {
			pipeCur = NULL;
		}
	}
	// if we managed to derive a type for all of the enclosed pipes
	if (pipeTypesValid) {
		// set the result type to the input type being mapped to the null type
		status = new FilterType(inStatus, nullType);
	}
	GET_STATUS_FOOTER;
}

// typeToUse is the reference to place the new FilterType object into; if it's NULL, use a new instantiation
TypeStatus getStatusFilterHeader(Tree *tree, FilterType *typeToUse, TypeStatus inStatus) {
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
		if (typeToUse != NULL) {
			typeToUse->from = (TypeList *)(from.type);
			typeToUse->to = (TypeList *)(to.type);
			status = typeToUse;
		} else {
			status = new FilterType(from, to);
		}
		status = tree;
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusFilterHeader(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	status = getStatusFilterHeader(tree, NULL, inStatus);
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusFilter(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *tc = tree->child; // FilterHeader or Block
	TypeStatus header = TypeStatus(nullType, inStatus);
	if (*tc == TOKEN_FilterHeader) { // if there is a header, derive its type
		header = getStatusFilterHeader(tc, inStatus);
	}
	if (*header) { // if we end up with a non-erroneous type for the header
// LOL
	} else { // else if we derived an erroneous type for the header
		Token curToken = tc->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve node header type");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusConstructor(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
// LOL
	GET_STATUS_FOOTER;
}

// blindly derives types from constructor headers: does not verify sub-blocks or add members to the ObjectType
void getStatusObjectSoft(Tree *tree, Type *&softType, TypeStatus inStatus) {
	// fake a type for the declaration base
	softType = new ObjectType();	
	// derive types for all of the contructors
	vector<TypeList *> constructorTypes;
	bool failed = false;
	Tree *conss = tree->child->next; // Constructors
	for (Tree *cons = conss->child; cons != NULL; cons = (cons->next->next != NULL) ? cons->next->next->child : NULL) {
		TypeStatus consStatus = getStatusConstructor(cons, inStatus);
		if (*consStatus) { // if we successfully derived a type for this constructor
			// check if there's already a constructor of this type
			vector<TypeList *>::iterator iter = constructorTypes.begin();
			while (iter != constructorTypes.end()) {
				if (**iter == *consStatus) {
					break;
				}
				// advance
				iter++;
			}
			if (iter == constructorTypes.end()) { // if there were no conflicts, add the constructor's type to the list
				constructorTypes.push_back((TypeList *)(consStatus.type));
			} else { // otherwise, flag the conflict as an error
				Token curDefToken = cons->child->t;
				semmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"duplicate constructor of type "<<*consStatus<<" in object definition");
				failed = true;
			}
		} else { // otherwise, if we failed to derive a type for this constructor
			failed = true;
		}
	}
	// derive names and types for all of the members
	vector<string> memberNames;
	vector<Type *> memberTypes;
	Tree *pipes = conss->next; // Pipes
	for (Tree *pipe = pipes->child; pipe != NULL; pipe = (pipe->next != NULL) ? pipe->next->next->child : NULL) {
		Tree *pipec = pipe->child; // Declaration or NonEmptyTerms
		if (*pipec == TOKEN_Declaration) { // if it's a member declaration, log it in the lists
			TypeStatus memberStatus = getStatusDeclaration(pipec, inStatus);
			if (*memberStatus) { // if we successfully derived a type for this Declaration
				memberNames.push_back(pipec->child->t.s);
				memberTypes.push_back(memberStatus.type);
			} else { // else if we failed to derive a type
				failed = true;
			}
		}
	}
	if (!failed) { // if we successfully derived the lists
		((ObjectType *)softType)->constructorTypes = constructorTypes;
		((ObjectType *)softType)->memberNames = memberNames;
		((ObjectType *)softType)->memberTypes = memberTypes;
	} else { // else if we failed to derive the lists
		delete softType; // delete the fake type
		softType = errType; // replace the fake type with the error type
	}
}

TypeStatus getStatusObject(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
// LOL
	GET_STATUS_FOOTER;
}

TypeStatus getStatusType(Tree *tree, int suffixVal, int depthVal, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *typec = tree->child; // FilterType or NonArraySuffixedIdentifier
	if (*typec == TOKEN_FilterType) { // if it's a regular filter type
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
	} else if (*typec == TOKEN_NonArraySuffixedIdentifier) { // if it's an identifier (object) type
		status = getStatusSuffixedIdentifier(typec);
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusTypeList(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	vector<Type *> list;
	Tree *treeCur = tree;
	bool failed = false;
	for(;;) { // invariant: treeCur is a TypeList
		Tree *type = treeCur->child; // Type
		Tree *typeSuffix = type->next; // TypeSuffix
		// derive the suffix and depth first, since we'll need to know then to construct the Type object
		int suffixVal;
		int depthVal = 0;
		if (typeSuffix->child == NULL) {
			suffixVal = SUFFIX_CONSTANT;
		} else if (*(typeSuffix->child) == TOKEN_SLASH) {
			suffixVal = SUFFIX_LATCH;
		} else if (*(typeSuffix->child) == TOKEN_StreamTypeSuffix) {
			suffixVal = SUFFIX_STREAM;
			Tree *sts = typeSuffix->child; // StreamTypeSuffix
			for(;;) {
				depthVal++;
				// advance
				if (sts->child->next != NULL) {
					sts = sts->child->next; // StreamTypeSuffix
				} else {
					break;
				}
			}
		} else { // *(typeSuffix->child) == TOKEN_ArrayTypeSuffix
			suffixVal = SUFFIX_ARRAY;
			Tree *ats = typeSuffix->child; // ArrayTypeSuffix
			for(;;) {
				depthVal++;
				// advance
				if (ats->child->next != NULL) {
					ats = ats->child->next; // ArrayTypeSuffix
				} else {
					break;
				}
			}
		}
		// construct the Type object
		TypeStatus curTypeStatus = getStatusType(type, suffixVal, depthVal, inStatus);
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

TypeStatus getStatusParamList(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	vector<Type *> list;
	Tree *treeCur = tree;
	bool failed = false;
	for(;;) { // invariant: treeCur is a ParamList
		Tree *type = treeCur->child->child; // Type
		Tree *typeSuffix = type->next; // TypeSuffix
		// derive the suffix and depth first, since we'll need to know then to construct the Type object
		int suffixVal;
		int depthVal = 0;
		if (typeSuffix->child == NULL) {
			suffixVal = SUFFIX_CONSTANT;
		} else if (*(typeSuffix->child) == TOKEN_SLASH) {
			suffixVal = SUFFIX_LATCH;
		} else if (*(typeSuffix->child) == TOKEN_StreamTypeSuffix) {
			suffixVal = SUFFIX_STREAM;
			Tree *sts = typeSuffix->child; // StreamTypeSuffix
			for(;;) {
				depthVal++;
				// advance
				if (sts->child->next != NULL) {
					sts = sts->child->next; // StreamTypeSuffix
				} else {
					break;
				}
			}
		} else { // *(typeSuffix->child) == TOKEN_ArrayTypeSuffix
			suffixVal = SUFFIX_ARRAY;
			Tree *ats = typeSuffix->child; // ArrayTypeSuffix
			for(;;) {
				depthVal++;
				// advance
				if (ats->child->next != NULL) {
					ats = ats->child->next; // ArrayTypeSuffix
				} else {
					break;
				}
			}
		}
		// construct the Type object
		TypeStatus curTypeStatus = getStatusType(type, suffixVal, depthVal, inStatus);
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
TypeStatus getStatusNodeInstantiation(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *tl = tree->child->next; // TypeList
	TypeStatus instantiation = getStatusTypeList(tl, inStatus);
	if (*instantiation) { // if we successfully derived a type for the instantiation
		if (tl->next->next != NULL) { // if there's an initializer, we need to make sure that the types are compatible
			Tree *st = tl->next->next->next; // StaticTerm
			TypeStatus initializer = getStatusStaticTerm(st, inStatus);
			if (*initializer) { //  if we successfully derived a type for the initializer
				// pipe the types into the status
				status = TypeStatus((*initializer , *instantiation), instantiation);
				if (!(*status)) { // if the types are incompatible, throw an error
					Token curToken = st->t;
					semmerError(curToken.fileName,curToken.row,curToken.col,"initializer type incompatible with instantiation");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (instantiation type is "<<*instantiation<<")");
					semmerError(curToken.fileName,curToken.row,curToken.col,"-- (initializer type is "<<*initializer<<")");
				}
			} else { // else if we couldn't derive a type for the initializer
				Token curToken = st->t;
				semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve initializer's type");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
			}
		}
	} else { // else if we couldn't derive a type for the instantiation
		Token curToken = tree->child->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve instantiation type");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

// blindly derives types from headers: does not verify sub-blocks
void getStatusNodeSoft(Tree *tree, Type *&softType, TypeStatus inStatus) {
	Tree *nodec = tree->child;
	if (*nodec == TOKEN_SuffixedIdentifier) {
		softType = getStatusSuffixedIdentifier(nodec, inStatus);
	} else if (*nodec == TOKEN_NodeInstantiation) {
		softType = getStatusNodeInstantiation(nodec, inStatus);
	} else if (*nodec == TOKEN_Filter) {
		Tree *filterc = nodec->child;
		if (*filterc == TOKEN_Block) { // if it's an implicit block-defined filter, its type is a consumer of the input type
			softType = TypeStatus(new FilterType(inStatus), inStatus);
		} else if (*filterc == TOKEN_FilterHeader) { // else if it's an explicit header-defined filter, its type is the type of the header
			softType = new FilterType(); // fake a type for the declaration base
			TypeStatus tempStatus = getStatusFilterHeader(filterc, (FilterType *)softType, inStatus); // derive the (possibly recursive) type of the filter header
			if (!(*tempStatus)) { // if we failed to derive a type for the header
				delete softType; // delete the fake type
				softType = errType; // replace the fake type with the error type
			}
		}
	} else if (*nodec == TOKEN_Object) {
		getStatusObjectSoft(nodec, softType, inStatus); // recurse on the Object version of this function
	} else if (*nodec == TOKEN_PrimOpNode) {
		softType = getStatusPrimOpNode(nodec, inStatus);
	} else if (*nodec == TOKEN_PrimLiteral) {
		softType = getStatusPrimLiteral(nodec, inStatus);
	}
}

// reports errors
TypeStatus getStatusNode(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *nodec = tree->child;
	if (*nodec == TOKEN_SuffixedIdentifier) {
		status = getStatusSuffixedIdentifier(nodec, inStatus);
	} else if (*nodec == TOKEN_NodeInstantiation) {
		status = getStatusNodeInstantiation(nodec, inStatus);
	} else if (*nodec == TOKEN_Filter) {
		status = getStatusFilter(nodec, inStatus);
	} else if (*nodec == TOKEN_Object) {
		status = getStatusObject(nodec, inStatus);
	} else if (*nodec == TOKEN_PrimOpNode) {
		status = getStatusPrimOpNode(nodec, inStatus);
	} else if (*nodec == TOKEN_PrimLiteral) {
		status = getStatusPrimLiteral(nodec, inStatus);

	}
	// if we couldn't resolve a type
	if (!status && *nodec != TOKEN_SuffixedIdentifier) {
		Token curToken = tree->t;
		semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve node's type");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusTypedStaticTerm(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *tstc = tree->child;
	if (*tstc == TOKEN_Node) {
		status = getStatusNode(tstc, inStatus);
	} else if (*tstc == TOKEN_LBRACKET) { // it's an expression
		status = getStatusExp(tstc->next, inStatus); // move past the bracket to the actual Exp node
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusStaticTerm(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *stc = tree->child;
	if (*stc == TOKEN_TypedStaticTerm) {
		status = getStatusTypedStaticTerm(stc, inStatus);
	} else if (*stc == TOKEN_Access) {
// LOL
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusDynamicTerm(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *dtc = tree->child;
	if (*dtc == TOKEN_StaticTerm) {
		status = getStatusStaticTerm(dtc, inStatus);
	} else if (*dtc == TOKEN_Compound) {
// LOL
	} else if (*dtc == TOKEN_Link) {
// LOL
	} else if (*dtc == TOKEN_Send) {
// LOL
	} else if (*dtc == TOKEN_Swap) {
	// LOL
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusSwitchTerm(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	vector<TypeStatus> toes; // vector for logging the destination statuses of each branch
	vector<Tree *> toTrees; // vector for logging the tree nodes of each branch
	Tree *lpCur = tree->child->next->next; // LabeledPipes
	for (;;) { // per-labeled pipe loop
		Tree *lpc = lpCur->child; // StaticTerm or COLON
		// if there is a non-default label on this pipe, check its validity
		if (*lpc == TOKEN_StaticTerm) {
			// derive the label's type
			TypeStatus label = getStatusStaticTerm(lpc, inStatus);
			if (*inStatus != *label) { // if the type doesn't match, throw an error
				Token curToken = lpc->t;
				semmerError(curToken.fileName,curToken.row,curToken.col,"switch label type doesn't match input type");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (label type is "<<*label<<")");
				semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
			}
		}
		// derive the to-type of this label
		Tree *toTree = (*lpc == TOKEN_StaticTerm) ? lpc->next->next : lpc->next; // SimpleTerm
		TypeStatus to = getStatusSimpleTerm(toTree, inStatus);
		// log the to-type and to-tree of this label
		toes.push_back(to);
		toTrees.push_back(toTree);
		// advance
		if (lpCur->child->next->next != NULL && lpCur->child->next->next->next != NULL) {
			lpCur = lpCur->child->next->next->next; // LabeledPipes
		} else {
			break;
		}
	} // per-labeled pipe loop
	// verify that all of the to-types are the same
	TypeStatus firstToStatus = toes[0];
	Tree *firstToTree = toTrees[0];
	for (unsigned int i=1; i < toes.size(); i++) { // for each to-type
		TypeStatus to = toes[i];
		if (*to != *firstToStatus) { // if the types don't match, throw an error
			Tree *toTree = toTrees[i];
			Token curToken = toTree->t;
			Token curToken2 = firstToTree->t;
			semmerError(curToken.fileName,curToken.row,curToken.col,"switch destination types are inconsistent");
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (this type is "<<*to<<")");
			semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (first type is "<<*firstToStatus<<")");
		}
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusSimpleTerm(Tree *tree, TypeStatus inStatus) {
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
TypeStatus getStatusSimpleCondTerm(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		inStatus = inStatus.recall->status;
		status = getStatusTerm(tree->child->next, inStatus);
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusClosedTerm(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *ctc = tree->child;
	if (*ctc == TOKEN_SimpleTerm) {
		status = getStatusSimpleTerm(ctc, inStatus);
	} else if (*ctc == TOKEN_ClosedCondTerm) {
		status = getStatusClosedCondTerm(ctc, inStatus);
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusOpenTerm(Tree *tree, TypeStatus inStatus) {
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
TypeStatus getStatusOpenCondTerm(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		Tree *trueBranch = tree->child->next;
		Tree *falseBranch = trueBranch->next->next;
		inStatus = inStatus.recall->status;
		TypeStatus trueStatus = getStatusClosedTerm(trueBranch, inStatus);
		TypeStatus falseStatus = getStatusOpenTerm(falseBranch, inStatus);
		if (*trueStatus == *falseStatus) { // if the two branches match in type
			status = trueStatus;
		} else { // else if the two branches don't match in type
			Token curToken = tree->child->t; // QUESTION
			Token curToken2 = trueBranch->t; // ClosedTerm
			Token curToken3 = falseBranch->t; // OpenTerm
			semmerError(curToken.fileName,curToken.row,curToken.col,"type mismatch in conditional operator branches");
			semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (true branch type is "<<*trueStatus<<")");
			semmerError(curToken3.fileName,curToken3.row,curToken3.col,"-- (false branch type is "<<*falseStatus<<")");
		}
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusClosedCondTerm(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	if (*inStatus == STD_BOOL) { // if what's coming in is a boolean
		Tree *trueBranch = tree->child->next;
		Tree *falseBranch = trueBranch->next->next;
		inStatus = inStatus.recall->status;
		TypeStatus trueStatus = getStatusClosedTerm(trueBranch, inStatus);
		TypeStatus falseStatus = getStatusClosedTerm(falseBranch, inStatus);
		if (*trueStatus == *falseStatus) { // if the two branches match in type
			status = trueStatus;
		} else { // else if the two branches don't match in type
			Token curToken = tree->child->t; // QUESTION
			Token curToken2 = trueBranch->t; // ClosedTerm
			Token curToken3 = falseBranch->t; // ClosedTerm
			semmerError(curToken.fileName,curToken.row,curToken.col,"type mismatch in conditional operator branches");
			semmerError(curToken2.fileName,curToken2.row,curToken2.col,"-- (true branch type is "<<*trueStatus<<")");
			semmerError(curToken3.fileName,curToken3.row,curToken3.col,"-- (false branch type is "<<*falseStatus<<")");
		}
	} else { // else if what's coming in isn't a boolean
		Token curToken = tree->child->t; // QUESTION
		semmerError(curToken.fileName,curToken.row,curToken.col,"non-boolean input to conditional operator");
		semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusTerm(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *tc2 = tree->child->child;
	if (*tc2 == TOKEN_SimpleCondTerm) {
		status = getStatusSimpleCondTerm(tc2, inStatus);
	} else if (*tc2 == TOKEN_OpenCondTerm) {
		status = getStatusOpenCondTerm(tc2, inStatus);
	} else if (*tc2 == TOKEN_SimpleTerm) {
		status = getStatusSimpleTerm(tc2, inStatus);
	} else if (*tc2 == TOKEN_ClosedCondTerm) {
		status = getStatusClosedCondTerm(tc2, inStatus);
	}
	GET_STATUS_FOOTER;
}

// reports errors
TypeStatus getStatusNonEmptyTerms(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	// scan the pipe left to right
	Tree *curTerm = tree->child; // Term
	TypeStatus outStatus;
	while (curTerm != NULL) {
		outStatus = getStatusTerm(curTerm, inStatus);
		if (*outStatus) { // if we found a proper typing for this term, log it
			curTerm->status = outStatus;
			inStatus = outStatus;
		} else { // otherwise, if we were unable to assign a type to the term, flag an error
			Token curToken = curTerm->t;
			semmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve term's output type");
			semmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type is "<<*inStatus<<")");
			// log the fact that typing failed
			outStatus.type = NULL;
			break;
		}
		// advance
		curTerm = curTerm->next->child; // Term or NULL
	}
	// if we succeeded in deriving an output type, return the mapping of the imput type to the output type
	if (outStatus) {
		status = new FilterType(inStatus, outStatus);
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusDeclaration(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *declarationSub = tree->child->next->next; // TypedStaticTerm, NonEmptyTerms, or NULL
	if (declarationSub != NULL) { // if it's a non-import declaration
		// attempt to derive the type of this Declaration
		if (*declarationSub == TOKEN_TypedStaticTerm) { // if it's a regular declaration
			Tree *tstc = declarationSub->child; // Node or LBRACKET
			if (*tstc == TOKEN_Node) { // possibly recursive node declaration
				// first, set the identifier's type to the declared type of the Node
				getStatusNodeSoft(tstc, tree->status.type);
				if (*(tree->status)) { // if the soft derivation succeeded
					// then, verify types for the declaration sub-block
					status = getStatusNode(tstc);
				} // else if the soft derivation failed, leave status as the error type
			} else if (*tstc == TOKEN_BracketedExp) { // else if it's a non-recursive expression declaration
				// derive the type of the expression without doing any bindings, since expressions must be non-recursive
				status = getStatusBracketedExp(tstc, inStatus);
			}
		} else if (*declarationSub == TOKEN_NonEmptyTerms) { // else if it's a flow-through declaration
			// first, set the identifier's type to the type of the NonEmptyTerms stream (an inputType consumer)
			tree->status = new FilterType(inStatus);
			// then, verify types for the declaration sub-block
			status = getStatusNonEmptyTerms(declarationSub, inStatus);
			// delete the temporary filter type
			delete (tree->status.type);
		}
	} else { // otherwise, if it's an import declaration
// LOL
	}
	GET_STATUS_FOOTER;
}

TypeStatus getStatusPipe(Tree *tree, TypeStatus inStatus) {
	GET_STATUS_HEADER;
	Tree *pipec = tree->child;
	if (*pipec == TOKEN_Declaration) { // if it's a Declaration pipe
		status = getStatusDeclaration(pipec, inStatus);
	} else if (*pipec == TOKEN_NonEmptyTerms) { // else if it's a raw NonEmptyTerms pipe
		status = getStatusNonEmptyTerms(pipec, inStatus);
	}
	GET_STATUS_FOOTER;
}

void traceTypes(vector<Tree *> *parseme) {
	// get a list of Pipe nodes
	vector<Tree *> &pipeList = parseme[TOKEN_Pipe];
	// iterate through the list of Pipes and trace the type flow for each one
	for (unsigned int i=0; i < pipeList.size(); i++) {
		Tree *pipeCur = pipeList[i];
		if (!pipeCur->status) { // if we haven't derived a type for this pipe yet
			getStatusPipe(pipeCur);
		}
	}
}

// main semming function; makes no assumptions about stRoot's value; it's just a return parameter
int sem(Tree *treeRoot, vector<Tree *> *parseme, SymbolTable *&stRoot) {

	// initialize error code
	semmerErrorCode = 0;
	semmerEventuallyGiveUp = eventuallyGiveUp;

	VERBOSE( printNotice("Collecting tree nodes..."); )

	// initialize the symbol table root with the default definitions
	stRoot = genDefaultDefs();

	// populate the symbol table with definitions from the user parseme, and log the used imports/id instances
	vector<SymbolTable *> importList; // import Declaration nodes
	buildSt(treeRoot, stRoot, importList); // get user definitions/imports
	subImportDecls(importList); // resolve and substitute import declarations into the symbol table

	VERBOSE( cout << stRoot; )

	VERBOSE( printNotice("Tracing type flow..."); )

	// assign types to all node streams
	traceTypes(parseme);

	// if there were no errors, free the error type node
	if (!semmerErrorCode) {
		delete errType;
	}

	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
