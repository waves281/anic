#ifndef _SEMMER_H_
#define _SEMMER_H_

#include "globalDefs.h"
#include "constantDefs.h"
#include "driver.h"

#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "genner.h"

// SymbolTree node kinds
#define KIND_STD 1
#define KIND_CLOSED_IMPORT 2
#define KIND_OPEN_IMPORT 3
#define KIND_BLOCK 4
#define KIND_DECLARATION 5
#define KIND_PARAMETER 6
#define KIND_INSTRUCTOR 7
#define KIND_OUTSTRUCTOR 8
#define KIND_FILTER 9
#define KIND_OBJECT 10
#define KIND_FAKE 11

// SymbolTree offset kinds
#define OFFSET_ROOT 0
#define OFFSET_RAW 1
#define OFFSET_BLOCK 2
#define OFFSET_PARTITION 3

class SymbolTree {
	public:
		// data members
		int kind; // node kind
		string id; // string representation of this node used for binding
		Tree *defSite; // where the symbol is defined in the Tree (Declaration or Param)
		SymbolTree *copyImportSite; // if this node is a copy-import, the node from which we're importing; NULL otherwise
		SymbolTree *parent; // pointer ot the parent of this node
		map<string, SymbolTree *> children; // list of this node's children
		int offsetKind; // the kind of child this node apprears as to its lexical parent
		unsigned int offsetIndex; // the offset of this child in the lexical parent's offset kind
		// allocators/deallocators
		SymbolTree(int kind, const string &id, Tree *defSite = NULL, SymbolTree *copyImportSite = NULL);
		SymbolTree(int kind, const char *id, Tree *defSite = NULL, SymbolTree *copyImportSite = NULL);
		SymbolTree(int kind, const string &id, Type *defType, SymbolTree *copyImportSite = NULL);
		SymbolTree(int kind, const char *id, Type *defType, SymbolTree *copyImportSite = NULL);
		SymbolTree(const SymbolTree &st, SymbolTree *parent, SymbolTree *copyImportSite = NULL);
		~SymbolTree();
		// copy assignment operator
		SymbolTree &operator=(const SymbolTree &st);
		// concatenators
		SymbolTree &operator*=(SymbolTree *st);
};

// forward declarations of mutually recursive typing functions

TypeStatus getStatusSymbolTree(SymbolTree *st, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusIdentifier(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusPrimaryBase(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusPrimary(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusBracketedExp(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusExp(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusPrimOpNode(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusPrimLiteral(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusBlock(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusFilterHeader(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus verifyStatusFilter(Tree *tree);
TypeStatus getStatusFilter(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus verifyStatusInstructor(Tree *tree);
TypeStatus getStatusInstructor(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus verifyStatusOutstructor(Tree *tree);
TypeStatus getStatusOutstructor(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus verifyStatusObject(Tree *tree);
TypeStatus getStatusObject(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusType(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusTypeList(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusParamList(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusInstantiationSource(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusInstantiation(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusNode(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusTypedStaticTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusAccess(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusStaticTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusDynamicTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusSwitchTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusSimpleTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusSimpleCondTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusOpenOrClosedCondTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusNonEmptyTerms(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusDeclaration(Tree *tree);
TypeStatus getStatusPipe(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));

// semantic analysis helper blocks

#define GET_STATUS_HEADER \
	/* if the type is memoized, short-circuit evaluate */\
	if (tree->status.type != NULL) {\
		return tree->status;\
	}\
	/* otherwise, compute the type normally */

#define returnType(x) \
	/* memoize the return value and jump to the intermediate code generation point */\
	tree->status = TypeStatus((x), inStatus.retType);\
	goto genIRTree

#define returnTypeRet(x,y) \
	/* memoize the return value and jump to the intermediate code generation point */\
	tree->status = TypeStatus((x),(y));\
	goto genIRTree

#define returnStatus(x) \
	/* memoize the return value and jump to the intermediate code generation point */\
	tree->status = (x);\
	goto genIRTree

#define GET_STATUS_CODE \
	/* if we failed to do a returnType, returnTypeRet, or returnStatus, memoize the error type and return from this function */\
	tree->status = TypeStatus(errType, NULL);\
	return (tree->status);\
	/* label the entry point for intermediate code tree generation */\
	genIRTree:\
	/* if we derived a valid return status, proceed to build the intermediate code tree */\
	if (tree->status.type->category != CATEGORY_ERRORTYPE) {

#define returnCode(x) \
	/* memoize the intermediate code tree and return from this function */\
	tree->status.code = (x);\
	return (tree->status)

#define GET_STATUS_FOOTER \
	/* close the if-statement */\
	}\
	/* if we failed to do a returnCode, simply return from this function */\
	return (tree->status)

// main semantic analysis function

int sem(Tree *treeRoot, SymbolTree *&stRoot, SchedTree *&codeRoot);

#endif
