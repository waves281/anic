#ifndef _SEMMER_H_
#define _SEMMER_H_

#include "mainDefs.h"
#include "system.h"
#include "constantDefs.h"

#include "lexer.h"
#include "parser.h"
#include "types.h"

// SymbolTable node kinds

#define KIND_BLOCK 1
#define KIND_STD 2
#define KIND_IMPORT 3
#define KIND_STATIC_DECL 4
#define KIND_THROUGH_DECL 5
#define KIND_PARAM 6

class SymbolTable {
	public:
		// data members
		int kind;
		string id;
		Tree *defSite; // where the symbol is defined in the Tree (Declaration or Param); NULL for root block and standard nodes
		SymbolTable *parent;
		vector<SymbolTable *> children;
		// allocators/deallocators
		SymbolTable(int kind, string id, Tree *defSite = NULL);
		SymbolTable(SymbolTable &st);
		~SymbolTable();
		// deep-copy assignment operator
		SymbolTable &operator=(SymbolTable &st);
		// concatenators
		SymbolTable &operator*=(SymbolTable *st);
};

// forward declarations of mutually recursive typing functions

TypeStatus getStatusSuffixedIdentifier(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusPrefixOrMultiOp(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusPrimary(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusBracketedExp(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusExp(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusPrimOpNode(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusPrimLiteral(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusBlock(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusFilterHeader(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusFilter(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusObjectBlock(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusTypeList(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusParamList(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusRetList(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusNodeInstantiation(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusNodeSoft(Tree *tree, SymbolTable *base, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusNode(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusTypedStaticTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusStaticTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusDynamicTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusSwitchTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusSimpleTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusSimpleCondTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusClosedTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusOpenTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusOpenCondTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusClosedCondTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusNonEmptyTerms(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusDeclaration(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getStatusPipe(Tree *tree, TypeStatus inStatus = TypeStatus());

// semantic analysis helper blocks

#define GET_TYPE_HEADER \
	/* if the type is memoized, short-circuit evaluate */\
	if (tree->status) {\
		return tree->status;\
	}\
	/* otherwise, compute the type normally */\
	TypeStatus status

#define GET_TYPE_FOOTER \
	/* if we could't resolve a valid type, use the error type */\
	if (!status) {\
		status = errType;\
	}\
	/* latch the status to the tree node */\
	tree->status = status;\
	/* return the derived status block */\
	return status

// main semantic analysis function

int sem(Tree *treeRoot, vector<Tree *> *parseme, SymbolTable *&stRoot);

#endif
