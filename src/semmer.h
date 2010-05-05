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

TypeStatus getTypeSuffixedIdentifier(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypePrefixOrMultiOp(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypePrimary(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeBracketedExp(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeExp(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypePrimOpNode(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypePrimLiteral(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeBlock(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeFilterHeader(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeFilter(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeObjectBlock(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeTypeList(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeParamList(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeRetList(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeNodeInstantiation(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeNodeSoft(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeNode(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeTypedStaticTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeStaticTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeDynamicTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeSwitchTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeSimpleTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeSimpleCondTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeClosedTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeOpenTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeOpenCondTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeClosedCondTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeTerm(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeNonEmptyTerms(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypeDeclaration(Tree *tree, TypeStatus inStatus = TypeStatus());
TypeStatus getTypePipe(Tree *tree, TypeStatus inStatus = TypeStatus());

// semantic analysis helper blocks

#define GET_TYPE_HEADER \
	/* if the type is memoized, short-circuit evaluate */\
	if (tree->type != NULL) {\
		return tree->type;\
	}\
	/* otherwise, compute the type normally */\
	TypeStatus outStatus;\
	Type *&type = outStatus.type

#define GET_TYPE_FOOTER \
	/* if we could't resolve a valid type, use the error type */\
	if (type == NULL) {\
		type = errType;\
	}\
	/* latch the status to the tree node */\
	tree->status = outStatus;\
	/* return the derived status block */\
	return outStatus

// main semantic analysis function

int sem(Tree *treeRoot, vector<Tree *> *parseme, SymbolTable *&stRoot);

#endif
