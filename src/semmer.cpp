#include "semmer.h"

#include "customOperators.h"

// semmer-global variables

int semmerErrorCode;
bool semmerEventuallyGiveUp;

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
	if (st != NULL && st->id[0] != '_') { // if this is not a special system-level binding
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
				printSemmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"redefinition of '"<<st->id<<"'",*this);
				printSemmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (previous definition was here)",*this);
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

// Type functions

// allocators/deallocators

Type::Type(int kind) : kind(kind), base(NULL), suffix(SUFFIX_NONE), next(NULL) {}
Type::Type(int kind, Tree *base) : kind(kind), base(base), suffix(SUFFIX_NONE), next(NULL) {}
Type::Type(int kind, Tree *base, int suffix) : kind(kind), base(base), suffix(suffix), next(NULL) {}

Type::~Type() {
	delete next;
}

// operators

bool Type::operator==(int kind) {
	return (this->kind == kind && base == NULL && suffix == SUFFIX_NONE && next == NULL);
}

bool Type::operator!=(int kind) {
	return (this->kind != kind && base == NULL && suffix == SUFFIX_NONE && next == NULL);
}

bool Type::operator>=(int kind) {
	return (this->kind >= kind && base == NULL && suffix == SUFFIX_NONE && next == NULL);
}

bool Type::operator<=(int kind) {
	return (this->kind <= kind && base == NULL && suffix == SUFFIX_NONE && next == NULL);
}

// to-string functions

// returns a string representation of the given type kind; does *not* work for USR kinds
string typeKind2String(int kind) {
	switch(kind) {
		// null type
		case STD_NULL:
			return "null";
		// standard types
		case STD_NODE:
			return "node";
		case STD_INT:
			return "int";
		case STD_FLOAT:
			return "float";
		case STD_BOOL:
			return "bool";
		case STD_CHAR:
			return "char";
		case STD_STRING:
			return "string";
		// prefix operators
		case STD_NOT:
			return "!";
		case STD_COMPLEMENT:
			return "~";
		case STD_DPLUS:
			return "++";
		case STD_DMINUS:
			return "--";
		// infix operators
		case STD_DOR:
			return "||";
		case STD_DAND:
			return "&&";
		case STD_OR:
			return "|";
		case STD_XOR:
			return "^";
		case STD_AND:
			return "&";
		case STD_DEQUALS:
			return "==";
		case STD_NEQUALS:
			return "!=";
		case STD_LT:
			return "<";
		case STD_GT:
			return ">";
		case STD_LE:
			return "<=";
		case STD_GE:
			return ">=";
		case STD_LS:
			return "<<";
		case STD_RS:
			return ">>";
		case STD_TIMES:
			return "*";
		case STD_DIVIDE:
			return "/";
		case STD_MOD:
			return "%";
		// multi operators
		case STD_PLUS:
			return "+";
		case STD_MINUS:
			return "-";
		// can't happen
		default:
			return "";
	}
}

// returns a string representation of the given type
string type2String(Type *t) {
	// allocate an accumulator for the type
	string acc = "";
	while (t != NULL) { // while there is more of the type to analyse
		// figure out the base type
		string baseType;
		if (t->kind != USR) { // if it's a non-user-defined base type
			baseType = typeKind2String(t->kind); // get the string representation statically
		} else { // else if it's a user-defined type
			baseType = ""; // LOL
		}
		// figure out the suffix
		string suffix;
		if (t->suffix == SUFFIX_NONE) {
			suffix = "";
		} else if (t->suffix == SUFFIX_LATCH) {
			suffix = "\\";
		} else {
			suffix = "";
			for (int i = 0; i < t->suffix; i++) {
				suffix += "\\\\";
				if (i+1 < t->suffix) { // if there's still more rounds to go, add a space
					suffix += " ";
				}
			}
		}
		// accumulate
		acc += (baseType + suffix);
		// advance
		t = t->next;
		// add a comma if necessary
		if (t != NULL) {
			acc += ", ";
		}
	}

	return acc;
}


// Main semantic analysis functions

void catStdNodes(SymbolTable *&stRoot) {
	*stRoot *= new SymbolTable(KIND_STD, "node", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "int", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "float", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "bool", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "char", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "string", NULL);
}

void catStdOps(SymbolTable *&stRoot) {
	// prefix
	*stRoot *= new SymbolTable(KIND_STD, "!", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "~", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "++", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "--", NULL);
	// infix
	*stRoot *= new SymbolTable(KIND_STD, "||", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "&&", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "|", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "^", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "&", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "==", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "!=", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "<", NULL);
	*stRoot *= new SymbolTable(KIND_STD, ">", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "<=", NULL);
	*stRoot *= new SymbolTable(KIND_STD, ">=", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "<<", NULL);
	*stRoot *= new SymbolTable(KIND_STD, ">>", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "*", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "/", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "%", NULL);
	// multi
	*stRoot *= new SymbolTable(KIND_STD, "+", NULL);
	*stRoot *= new SymbolTable(KIND_STD, "-", NULL);
}

void catStdLib(SymbolTable *&stRoot) {
	// standard root
	SymbolTable *stdLib = new SymbolTable(KIND_STD, STANDARD_LIBRARY_STRING, NULL);

	// system nodes
	// streams
	*stdLib *= new SymbolTable(KIND_STD, "in", NULL);
	*stdLib *= new SymbolTable(KIND_STD, "out", NULL);
	*stdLib *= new SymbolTable(KIND_STD, "err", NULL);
	// control nodes
	*stdLib *= new SymbolTable(KIND_STD, "rand", NULL);
	*stdLib *= new SymbolTable(KIND_STD, "delay", NULL);

	// standard library
	// containers
	*stdLib *= new SymbolTable(KIND_STD, "stack", NULL);
	*stdLib *= new SymbolTable(KIND_STD, "map", NULL);
	// filters
	*stdLib *= new SymbolTable(KIND_STD, "filter", NULL);
	*stdLib *= new SymbolTable(KIND_STD, "sort", NULL);
	// generators
	*stdLib *= new SymbolTable(KIND_STD, "gen", NULL);

	// concatenate the library to the root
	*stRoot *= stdLib;
}

SymbolTable *genDefaultDefs() {
	// generate the root block node
	SymbolTable *stRoot = new SymbolTable(KIND_BLOCK, BLOCK_NODE_STRING, NULL);
	// concatenate in the standard types
	catStdNodes(stRoot);
	// concatenate in the standard operators
	catStdOps(stRoot);
	// concatenate in the standard library
	catStdLib(stRoot);
	// finally, return the genrated default symtable
	return stRoot;
}

// recursively extracts the appropriate nodes from the given tree and appropriately populates the passed containers
void extractNodes(Tree *parseme, SymbolTable *st, vector<SymbolTable *> &importList, vector<Tree *> &netsList, bool netsHandled) {
	// base case
	if (parseme == NULL) {
		return;
	}
	// log the current symbol environment in the parseme
	parseme->env = st;
	// recursive cases
	if (parseme->t.tokenType == TOKEN_Identifier) { // if it's an Identifier
		if (parseme->back != NULL && parseme->back->t.tokenType == TOKEN_AT) { // if it's an import Identifier
			// recurse on the right only; i.e. don't log import subidentifiers as use cases
			extractNodes(parseme->next, st, importList, netsList, netsHandled); // right
		}
	} else if (parseme->t.tokenType == TOKEN_NonEmptyTerms && !netsHandled) { // if it's a term stream node
		// log the stream occurence
		netsList.push_back(parseme);
		// recurse down only; NonEmptyTerms never has any right siblings
		extractNodes(parseme->child, st, importList, netsList, true); // right
	} else if (parseme->t.tokenType == TOKEN_Block) { // if it's a block node
		// allocate the new definition node
		SymbolTable *blockDef = new SymbolTable(KIND_BLOCK, BLOCK_NODE_STRING, parseme);
		// if there is a header for to this block, add its parameters into the block node
		if (parseme->back != NULL && parseme->back->t.tokenType == TOKEN_NodeHeader) {
			Tree *nh = parseme->back; // NodeHeader
			if (nh->child->next->child != NULL) { // if there is a parameter list to process
				Tree *param = nh->child->next->child->child; // Param
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
			}
		} // if there is a header attatched to this block
		// finally, link the block node into the main trunk
		*st *= blockDef;
		// recurse
		extractNodes(parseme->child, blockDef, importList, netsList, netsHandled); // child of Block
	} else if (parseme->t.tokenType == TOKEN_Declaration) { // if it's a declaration node
		Token t = parseme->child->next->t;
		if (t.tokenType == TOKEN_EQUALS) { // standard static declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_STATIC_DECL, parseme->child->t.s, parseme);
			// ... and link it in
			*st *= newDef;
			// recurse
			extractNodes(parseme->child, newDef, importList, netsList, netsHandled); // child of Declaration
		} else if (t.tokenType == TOKEN_ERARROW) { // flow-through declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_THROUGH_DECL, parseme->child->t.s, parseme);
			// ... and link it in
			*st *= newDef;
			// recurse
			extractNodes(parseme->child, newDef, importList, netsList, netsHandled); // child of Declaration
		} else if (t.tokenType == TOKEN_Identifier) { // import declaration
			// allocate the new definition node
			SymbolTable *newDef = new SymbolTable(KIND_IMPORT, IMPORT_DECL_STRING, parseme);
			// ... and link it in
			*st *= newDef;
			// also, since it's an import declaration, log it to the import list
			importList.push_back(newDef);
			// recurse
			extractNodes(parseme->child, newDef, importList, netsList, netsHandled); // child of Declaration
		}
	} else { // else if it's not a declaration node
		// recurse normally
		extractNodes(parseme->child, st, importList, netsList, netsHandled); // down
		extractNodes(parseme->next, st, importList, netsList, netsHandled); // right
	}
}

// wrapper for the above function
void extractNodes(Tree *parseme, SymbolTable *st, vector<SymbolTable *> &importList, vector<Tree *> &netsList) {
	extractNodes(parseme, st, importList, netsList, false);
}

// forward declarations
SymbolTable *bindId(string &id, SymbolTable *env);

// binds qualified identifiers in the given symtable environment; returns the tail of the binding
// also, updates id to contain the portion of the identifier that could not be bound
// returns NULL if no binding whatsoever can be found
SymbolTable *bindId(Type *inType, string &id, SymbolTable *env) {
	// base case
	if (env == NULL) {
		return NULL;
	}
	// recall identifier case
	if (id[0] == '.' && id[1] == '.') {

		if (inType != NULL) { // if there is an input type
			// update id to be the rest of the recall identifier
			string idRest = "";
			if (id.length() >= 3) { // if this recall identifier has a rest-part
				idRest = id.substr(3, id.length()-3); // get the rest of the recall identifier string
			}
			id = idRest;

			// return the binding of the current input type, since that's what the recall identifier implicitly binds to
			if (inType->kind == STD_NULL) { // if the incoming type is null, we can't bind to it, so return an error
				return NULL;
			} else if (inType->kind != USR) { // else if it's a non-user-defined base type
				string recString = typeKind2String(inType->kind); // statically get the string representation
				return bindId(recString, env); // statically bind to the specific standard node
			} else { // else if it's a user-defined type
				return NULL; // LOL
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

	// if we've verified the entire qi, return the tail of the latch point
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
	SymbolTable *altResult = bindId(inType, altId, recurseSt);

	// check if the recursive solution is better
	if (altResult != NULL  && ((altId.length() < id.length()) || stCur == NULL)) { // if the other binding succeeded and is better, return it
		id = altId;
		return altResult;
	} else { // otherwise, return the local binding
		return stCur;
	}
}

// wrappers for the above function
SymbolTable *bindId(string &id, SymbolTable *env) {
	return bindId(NULL, id, env);
}


void subImportDecls(vector<SymbolTable *> &importList) {
	bool stdExplicitlyImported = false;
	// per-import loop
	for (vector<SymbolTable *>::iterator importIter = importList.begin(); importIter != importList.end(); importIter++) {
		// extract the import path out of the iterator
		string importPath = id2String((*importIter)->defSite->child->next);
		// standard import special-casing
		if (importPath == "std") { // if it's the standard import
			if (!stdExplicitlyImported) { // if it's the first standard import, flag it as handled and let it slide
				(*importIter)->id = STANDARD_IMPORT_DECL_STRING;
				stdExplicitlyImported = true;
				continue;
			}
		}
		// try to find a binding for this import
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
					printSemmerError(curDefToken.fileName,curDefToken.row,curDefToken.col,"name conflict in importing '"<<importPathTip<<"'",);
					printSemmerError(prevDefToken.fileName,prevDefToken.row,prevDefToken.col,"-- (conflicting definition was here)",);
					goto nextImport;
				}
				// advance
				childIter++;
			}
			// there was no conflict, so just deep-copy the binding in place of the import placeholder node
			**importIter = *binding;
		} else { // else if no binding could be found
			Token t = (*importIter)->defSite->t;
			printSemmerError(t.fileName,t.row,t.col,"cannot resolve import '"<<importPath<<"'",);
		}
		nextImport: ;
	} // per-import loop
}

// forward declarations of mutually recursive typing functions

Type *getPrimaryType(Type *inType, Tree *primary);
Type *getExpType(Type *inType, Tree *exp);
Type *getTermType(Type *inType, Tree *term);

// typing function definitions

Type *getPrimaryType(Type *inType, Tree *primary) {
	Tree *primaryc = primary->child;
	Type *type = NULL;
	if (primaryc->t.tokenType == TOKEN_Identifier) {
		string id = id2String(primaryc); // string representation of this identifier
		string idCur = id; // a destructible copy for the recursion
		SymbolTable *st = bindId(inType, idCur, primaryc->env);
		if (st != NULL) { // if we found some sort of static binding

		} else { // else if there was no static binding at all
			Token t = primaryc->t;
			printSemmerError(t.fileName,t.row,t.col,"cannot resolve '"<<id<<"'",NULL);
		}

		type = NULL; // LOL
	} else if (primaryc->t.tokenType == TOKEN_SLASH) {
		type = NULL; // LOL
	} else if (primaryc->t.tokenType == TOKEN_PrimLiteral) {
		Tree *primLiteralc = primaryc;
		if (primLiteralc->t.tokenType == TOKEN_INUM) {
			type = new Type(STD_INT);
		} else if (primLiteralc->t.tokenType == TOKEN_FNUM) {
			type = new Type(STD_FLOAT);
		} else if (primLiteralc->t.tokenType == TOKEN_CQUOTE) {
			type = new Type(STD_CHAR);
		} else if (primLiteralc->t.tokenType == TOKEN_SQUOTE) {
			type = new Type(STD_STRING);
		}
	} else if (primaryc->t.tokenType == TOKEN_PrefixOrMultiOp) {
		Tree *pomocc = primaryc->child->child;
		Type *subType = getPrimaryType(inType, primaryc->next);
		if (pomocc->t.tokenType == TOKEN_NOT) {
			if (*subType == STD_BOOL) {
				type = subType;
			}
		} else if (pomocc->t.tokenType == TOKEN_COMPLEMENT) {
			if (*subType == STD_INT) {
				type = subType;
			}
		} else if (pomocc->t.tokenType == TOKEN_DPLUS) {
			if (*subType == STD_INT) {
				type = subType;
			}
		} else if (pomocc->t.tokenType == TOKEN_DMINUS) {
			if (*subType == STD_INT) {
				type = subType;
			}
		} else if (pomocc->t.tokenType == TOKEN_PLUS) {
			if (*subType == STD_INT || *subType == STD_FLOAT) {
				type = subType;
			}
		} else if (pomocc->t.tokenType == TOKEN_MINUS) {
			if (*subType == STD_INT || *subType == STD_FLOAT) {
				type = subType;
			}
		}
	} else if (primaryc->t.tokenType == TOKEN_LBRACKET) {
		type = getExpType(inType, primaryc->next);
	}
	// latch the type to the Primary node
	primary->type = type;
	// return the derived type
	return type;
}

Type *getExpType(Type *inType, Tree *exp) {
	Tree *expc = exp->child;
	Type *type = NULL;
	if (expc->t.tokenType == TOKEN_Primary) {
		type = getPrimaryType(inType, expc);
	} else if (expc->t.tokenType == TOKEN_Exp) {
		Tree *expLeft = expc;
		Tree *op = expLeft->next;
		Tree *expRight = op->next;
		Type *typeLeft = getExpType(inType, expLeft);
		Type *typeRight = getExpType(inType, expRight);
		switch (op->t.tokenType) {
			case TOKEN_DOR:
			case TOKEN_DAND:
				if (*typeLeft == STD_BOOL && *typeRight == STD_BOOL) {
					type = typeLeft;
				}
				break;
			case TOKEN_OR:
			case TOKEN_XOR:
			case TOKEN_AND:
				if (*typeLeft == STD_INT && *typeRight == STD_INT) {
					type = typeLeft;
				}
				break;
			case TOKEN_DEQUALS:
			case TOKEN_NEQUALS:
			case TOKEN_LT:
			case TOKEN_GT:
			case TOKEN_LE:
			case TOKEN_GE:
				if (*typeLeft >= STD_MIN_COMPARABLE && *typeRight >= STD_MIN_COMPARABLE &&
						*typeLeft <= STD_MAX_COMPARABLE && *typeRight <= STD_MAX_COMPARABLE &&
						typeLeft->kind == typeRight->kind) {
					type = new Type(STD_BOOL);
				}
				break;
			case TOKEN_LS:
			case TOKEN_RS:
				if (*typeLeft == STD_INT && *typeRight == STD_INT) {
					type = typeLeft;
				}
				break;
			case TOKEN_TIMES:
			case TOKEN_DIVIDE:
			case TOKEN_MOD:
			case TOKEN_PLUS:
			case TOKEN_MINUS:
				if ( (*typeLeft == STD_INT || *typeLeft == STD_FLOAT) &&
						(*typeRight == STD_INT || *typeRight == STD_FLOAT) ) {
					if (*typeRight != STD_FLOAT) {
						type = typeLeft;
					} else {
						type = typeRight;
					}
				}
				break;
			default: // can't happen
				typeLeft = NULL;
				typeRight = NULL;
				break;
		}
	}
	// latch the type to the Exp node
	exp->type = type;
	// return the derived type
	return type;
}

Type *getTermType(Type *inType, Tree *term) {
	Tree *tc2 = term->child->child;
	Type *type = NULL;
	if (tc2->t.tokenType == TOKEN_SimpleCondTerm) {
// LOL
	} else if (tc2->t.tokenType == TOKEN_OpenCondTerm) {
// LOL
	} else if (tc2->t.tokenType == TOKEN_SimpleTerm) {
		Tree *tc3 = tc2->child;
		if (tc3->t.tokenType == TOKEN_DynamicTerm) {
			Tree *tc4 = tc3->child;
			if (tc4->t.tokenType == TOKEN_StaticTerm) {
				Tree *tc5 = tc4->child;
				if (tc5->t.tokenType == TOKEN_TypedStaticTerm) {
					Tree *tc6 = tc5->child;
					if (tc6->t.tokenType == TOKEN_Node) {
						Tree *tc7 = tc6->child;
						if (tc7->t.tokenType == TOKEN_Identifier) {

							// try to find a binding for this identifier
							string id = id2String(tc7);
 							SymbolTable *st = bindId(id, tc7->env);
 							if (st != NULL) { // if we found some sort of static binding
// LOL
							} else { // else if there was no static binding at all
								Token t = tc7->t;
								printSemmerError(t.fileName,t.row,t.col,"cannot resolve '"<<id<<"'",NULL);
							}
						} else if (tc7->t.tokenType == TOKEN_NodeInstantiation) {
// LOL
						} else if (tc7->t.tokenType == TOKEN_TypedNodeLiteral) {
// LOL
						} else if (tc7->t.tokenType == TOKEN_PrimOpNode) {
							Tree *tc9 = tc7->child->child; // the operator token itself
							// generate the type based on the specific operator it is
							switch (tc9->t.tokenType) {
								case TOKEN_NOT:
									type = new Type(STD_NOT, tc9);
									break;
								case TOKEN_COMPLEMENT:
									type = new Type(STD_COMPLEMENT, tc9);
									break;
								case TOKEN_DPLUS:
									type = new Type(STD_DPLUS, tc9);
									break;
								case TOKEN_DMINUS:
									type = new Type(STD_DMINUS, tc9);
									break;
								case TOKEN_DOR:
									type = new Type(STD_DOR, tc9);
									break;
								case TOKEN_DAND:
									type = new Type(STD_DAND, tc9);
									break;
								case TOKEN_OR:
									type = new Type(STD_OR, tc9);
									break;
								case TOKEN_XOR:
									type = new Type(STD_XOR, tc9);
									break;
								case TOKEN_AND:
									type = new Type(STD_AND, tc9);
									break;
								case TOKEN_DEQUALS:
									type = new Type(STD_DEQUALS, tc9);
									break;
								case TOKEN_NEQUALS:
									type = new Type(STD_NEQUALS, tc9);
									break;
								case TOKEN_LT:
									type = new Type(STD_LT, tc9);
									break;
								case TOKEN_GT:
									type = new Type(STD_GT, tc9);
									break;
								case TOKEN_LE:
									type = new Type(STD_LE, tc9);
									break;
								case TOKEN_GE:
									type = new Type(STD_GE, tc9);
									break;
								case TOKEN_LS:
									type = new Type(STD_LS, tc9);
									break;
								case TOKEN_RS:
									type = new Type(STD_RS, tc9);
									break;
								case TOKEN_TIMES:
									type = new Type(STD_TIMES, tc9);
									break;
								case TOKEN_DIVIDE:
									type = new Type(STD_DIVIDE, tc9);
									break;
								case TOKEN_MOD:
									type = new Type(STD_MOD, tc9);
									break;
								case TOKEN_PLUS:
									type = new Type(STD_PLUS, tc9);
									break;
								case TOKEN_MINUS:
									type = new Type(STD_MINUS, tc9);
									break;
								default: // can't happen
									type = NULL;
									break;
							}
						} else if (tc7->t.tokenType == TOKEN_PrimLiteral) {
							Tree *tc8 = tc7->child;
							if (tc8->t.tokenType == TOKEN_INUM) {
								type = new Type(STD_INT);
							} else if (tc8->t.tokenType == TOKEN_FNUM) {
								type = new Type(STD_FLOAT);
							} else if (tc8->t.tokenType == TOKEN_CQUOTE) {
								type = new Type(STD_CHAR);
							} else if (tc8->t.tokenType == TOKEN_SQUOTE) {
								type = new Type(STD_STRING);
							}
						}
					} else if (tc6->t.tokenType == TOKEN_LBRACKET) { // it's an expression
						Type *expType = getExpType(inType, tc6->next);
						tc6->type = expType;
						type = expType;
					}
				} else if (tc5->t.tokenType == TOKEN_Delatch) {
// LOL
				} else if (tc5->t.tokenType == TOKEN_Block) {
// LOL
				}
			} else if (tc4->t.tokenType == TOKEN_Compound) {
// LOL
			} else if (tc4->t.tokenType == TOKEN_Link) {
// LOL
			} else if (tc4->t.tokenType == TOKEN_Send) {
// LOL
			}
		} else if (tc3->t.tokenType == TOKEN_SwitchTerm) {
// LOL
		}
	} else if (tc2->t.tokenType == TOKEN_ClosedCondTerm) {
// LOL
	}
	// latch the type to the Term node
	term->type = type;
	// return the derived type
	return (Type *)0x4; // LOL
}

void traceTypes(vector<Tree *> &netsList) {
	// simply iterate through the list of NonEmptyTerms and trace the types for each one, starting with nullity
	for (unsigned int i=0; i < netsList.size(); i++) {
		// temporaily allocate the null type
		Type *nullType = new Type(STD_NULL);
		// scan the pipe left to right
		Tree *curTerm = netsList[i]->child;
		Type *inType = nullType;
		while (curTerm != NULL) {
			Type *outType = getTermType(inType, curTerm);
			if (outType != NULL) { // if we found a proper typing for this term, log it
				curTerm->type = outType;
			} else { // otherwise, if we were unable to assign a type to the term, flag an error
				Token curToken = curTerm->t;
				printSemmerError(curToken.fileName,curToken.row,curToken.col,"cannot resolve output type for this term",);
				printSemmerError(curToken.fileName,curToken.row,curToken.col,"-- (input type was "<<type2String(inType)<<")",);
				// skip typing this pipe and move on to the next one
				break;
			}
			// advance
			curTerm = curTerm->next->child; // Term
		}
	}
}

// main semming function; makes no assumptions about stRoot's value; it's just a return parameter
int sem(Tree *rootParseme, SymbolTable *&stRoot, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp) {

	// initialize error code
	semmerErrorCode = 0;
	semmerEventuallyGiveUp = eventuallyGiveUp;

	VERBOSE( printNotice("Collecting tree nodes..."); )

	// initialize the symbol table root with the default definitions
	stRoot = genDefaultDefs();

	// populate the symbol table with definitions from the user parseme, and log the used imports/id instances
	vector<SymbolTable *> importList; // import Declaration nodes
	vector<Tree *> netsList; // list of top-level Term nodes
	extractNodes(rootParseme, stRoot, importList, netsList);

	// substitute import declarations
	subImportDecls(importList);

	VERBOSE( cout << stRoot; )

	VERBOSE( printNotice("Tracing type flow..."); )

	// assign types to all node streams
	traceTypes(netsList);

	// finally, return to the caller
	return semmerErrorCode ? 1 : 0;
}
