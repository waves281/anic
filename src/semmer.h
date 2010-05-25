#ifndef _SEMMER_H_
#define _SEMMER_H_

#include "mainDefs.h"
#include "system.h"
#include "constantDefs.h"

#include "lexer.h"
#include "parser.h"
#include "types.h"

// SymbolTable node kinds

#define KIND_STD 1
#define KIND_IMPORT 2
#define KIND_BLOCK 3
#define KIND_DECLARATION 4
#define KIND_PARAMETER 5
#define KIND_CONSTRUCTOR 6
#define KIND_FILTER 7
#define KIND_OBJECT 8

class SymbolTable {
	public:
		// data members
		int kind;
		string id;
		Tree *defSite; // where the symbol is defined in the Tree (Declaration or Param)
		SymbolTable *parent;
		vector<SymbolTable *> children;
		// allocators/deallocators
		SymbolTable(int kind, const string &id, Tree *defSite = NULL);
		SymbolTable(int kind, const char *id, Tree *defSite = NULL);
		SymbolTable(int kind, const string &id, Type *defType);
		SymbolTable(int kind, const char *id, Type *defType);
		SymbolTable(const SymbolTable &st);
		~SymbolTable();
		// deep-copy assignment operator
		SymbolTable &operator =(const SymbolTable &st);
		// concatenators
		SymbolTable &operator *=(SymbolTable *st);
};

// forward declarations of mutually recursive typing functions

TypeStatus getStatusSymbolTable(SymbolTable *st, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusSuffixedIdentifier(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusPrefixOrMultiOp(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusPrimary(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusBracketedExp(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusExp(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusPrimOpNode(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusPrimLiteral(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusBlock(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusFilterHeader(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusFilter(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusConstructor(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusObject(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusType(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusTypeList(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusParam(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusParamList(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusNodeInstantiation(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusNode(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusTypedStaticTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusStaticTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusDynamicTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusSwitchTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusSimpleTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusSimpleCondTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusClosedTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusOpenTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusOpenCondTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusClosedCondTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusTerm(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusNonEmptyTerms(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusDeclaration(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));
TypeStatus getStatusPipe(Tree *tree, const TypeStatus &inStatus = TypeStatus(nullType));

// semantic analysis helper blocks

#define GET_STATUS_HEADER \
	/* if the type is memoized, short-circuit evaluate */\
	if (tree->status.type != NULL) {\
		return tree->status;\
	}\
	/* otherwise, prepare to compute the type normally */\
	TypeStatus status(NULL, inStatus.recall)

#define GET_STATUS_FOOTER \
	/* if we could't resolve a valid type, use the error type */\
	if (status.type == NULL) {\
		status.type = errType;\
	}\
	/* latch the status to the tree node */\
	tree->status = status;\
	/* return the derived status block */\
	return status

// main semantic analysis function

int sem(Tree *treeRoot, vector<Tree *> *parseme, SymbolTable *&stRoot);

#endif
