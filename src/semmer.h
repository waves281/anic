#ifndef _SEMMER_H_
#define _SEMMER_H_

#include "globalDefs.h"
#include "constantDefs.h"
#include "driver.h"

#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "genner.h"

// SymbolTable node kinds

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

class SymbolTable {
	public:
		// data members
		int kind;
		string id;
		Tree *defSite; // where the symbol is defined in the Tree (Declaration or Param)
		Tree *copyImportSite; // if this node is a copy-import, the site where we're importing from; NULL otherwise
		SymbolTable *parent;
		map<string, SymbolTable *> children;
		// allocators/deallocators
		SymbolTable(int kind, const string &id, Tree *defSite = NULL, Tree *copyImportSite = false);
		SymbolTable(int kind, const char *id, Tree *defSite = NULL, Tree *copyImportSite = false);
		SymbolTable(int kind, const string &id, Type *defType, Tree *copyImportSite = false);
		SymbolTable(int kind, const char *id, Type *defType, Tree *copyImportSite = false);
		SymbolTable(const SymbolTable &st, Tree *copyImportSite = false);
		~SymbolTable();
		// copy assignment operator
		SymbolTable &operator=(const SymbolTable &st);
		// concatenators
		SymbolTable &operator*=(SymbolTable *st);
};

// forward declarations of mutually recursive typing functions

TypeStatus getStatusSymbolTable(SymbolTable *st, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusIdentifier(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusPrimaryBase(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusPrimary(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusBracketedExp(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusExp(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusPrimOpNode(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusPrimLiteral(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusBlock(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusFilterHeader(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusFilter(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusInstructor(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusOutstructor(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusObject(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusType(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusTypeList(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusParam(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
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
TypeStatus getStatusDeclaration(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
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

int sem(Tree *treeRoot, SymbolTable *&stRoot);

#endif
