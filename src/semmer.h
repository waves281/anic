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
#define KIND_CONSTRUCTOR 7
#define KIND_FILTER 8
#define KIND_OBJECT 9
#define KIND_FAKE 10

class SymbolTable {
	public:
		// data members
		int kind;
		string id;
		Tree *defSite; // where the symbol is defined in the Tree (Declaration or Param)
		SymbolTable *parent;
		map<string, SymbolTable *> children;
		// allocators/deallocators
		SymbolTable(int kind, const string &id, Tree *defSite = NULL);
		SymbolTable(int kind, const char *id, Tree *defSite = NULL);
		SymbolTable(int kind, const string &id, Type *defType);
		SymbolTable(int kind, const char *id, Type *defType);
		SymbolTable(const SymbolTable &st);
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
TypeStatus getStatusExp(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusPrimOpNode(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusPrimLiteral(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusBlock(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusFilterHeader(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusFilter(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusConstructor(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusObject(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusType(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusTypeList(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusParam(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus getStatusParamList(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
TypeStatus InstantiationSource(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType, errType));
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
	/* memoize the return value */\
	tree->status = TypeStatus((x), inStatus.retType);\
	/* do the actual return */\
	return (tree->status)

#define returnTypeRet(x,y) \
	/* memoize the return value */\
	tree->status = TypeStatus((x),(y));\
	/* do the actual return */\
	return (tree->status)

#define returnStatus(x) \
	/* memoize the return value */\
	tree->status = (x);\
	/* do the actual return */\
	return (tree->status)

#define GET_STATUS_FOOTER \
	/* if we couldn't resolve a valid type, memoize and return the error type */\
	tree->status = TypeStatus(errType, NULL);\
	return (tree->status)

// main semantic analysis function

int sem(Tree *treeRoot, SymbolTable *&stRoot);

#endif
