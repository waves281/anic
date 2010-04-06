/* 
   This file is part of Hyacc, a LR(0)/LALR(1)/LR(1) parser generator.
   Copyright (C) 2007, 2008 Xin Chen. chenx@hawaii.edu

   Hyacc is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   Hyacc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Hyacc; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * mrt.c
 *
 * Functions for multi-rooted tree. Used by unit production elimination
 * algorithm. 
 *
 * @Author: Xin Chen
 * @Date started: March 9, 2007
 * @Last modified: March 9, 2007
 * @Copyright (C) 2007, 2008
 */

#include "y.h"
#include "mrt.h"

/* Function declarations. */

void getAllMRParents();
void writeAllMRParents();


////////////////////////////////////////////
// Functions for multi-rooted tree. START.
////////////////////////////////////////////


void initArray_leafIndexForParent() {
  leafIndexForParent = (int *) malloc(sizeof(int) * MRParents_INIT_MAX_COUNT);
  if (leafIndexForParent == NULL)
    YYERR_EXIT("initArray_leafIndexForparent error: out of memory\n");
}

void expandArray_leafIndexForParent() {
  leafIndexForParent = (int *) realloc((void *) leafIndexForParent,
                       sizeof(int) * all_parents->max_count);
  if (leafIndexForParent == NULL)
    YYERR_EXIT("expandArray_leafIndexForparent error: out of memory\n");

  if (DEBUG_EXPAND_ARRAY) 
    printf("leafIndexForParent size expanded to %d\n",
           all_parents->max_count);
}

void destroy_leafIndexForParent() {
  free(leafIndexForParent);
}


MRParents * createMRParents() {
  MRParents * p = (MRParents *) malloc(sizeof(MRParents));
  if (p == NULL) 
    YYERR_EXIT("createMRParents error: out of memory\n");

  p->max_count = MRParents_INIT_MAX_COUNT;
  p->parents = (SymbolNode **) 
               malloc(sizeof(SymbolNode *) * p->max_count);
  if (p->parents == NULL) 
    YYERR_EXIT("createMRParents error: out of memory\n");
  p->count = 0;

  return p;
}


void checkArraySize_MRParents(MRParents * p) {
  if (p->count < p->max_count) return;

  p->max_count *= 2;
  p->parents = (SymbolNode **) realloc((void *) p->parents,
               sizeof(SymbolNode *) * p->max_count);
  if (p->parents == NULL)
    YYERR_EXIT("checkArraySize_MRParents error: out of meory\n");

  if (DEBUG_EXPAND_ARRAY)
    printf("MRParents size expanded to %d\n", p->max_count);

  // if all_parents array is expanded, expand leafIndexForParent too.
  if (p == all_parents) expandArray_leafIndexForParent();
}


void destroyMRParents(MRParents * p) {
  int i;
  if (p == NULL) return;
  for (i = 0; i < p->count; i++) free(p->parents[i]);
  free(p->parents);
  free(p);
}


void createMRLeavesArray() {
  MRLeaves_max_count = MRLeaves_INIT_MAX_COUNT;
  MRLeaves = (MRTreeNode **) 
              malloc(sizeof(MRTreeNode *) * MRLeaves_max_count);
  if (MRLeaves == NULL) 
    YYERR_EXIT("createMRLeavesArray error: out of memory\n");
}


void checkMRLeavesArraySize() {
  if (MRLeaves_count < MRLeaves_max_count) return;

  MRLeaves_max_count *= 2;
  MRLeaves = (MRTreeNode **) realloc((void *) MRLeaves,
             sizeof(MRTreeNode *) * MRLeaves_max_count);
  if (MRLeaves == NULL)
    YYERR_EXIT("checkMRLeavesArraySize error: out of memory\n");

  if (DEBUG_EXPAND_ARRAY)
    yyprintf2("MRLeaves size expanded to %d\n", MRLeaves_max_count);
}


void destroyMRLeavesArray() {
  free(MRLeaves);
}


void writeLeafIndexForParent() {
  int i;
  for (i = 0; i < all_parents->count; i ++) {
    if (i > 0) yyprintf(", ");
    yyprintf2("%d", leafIndexForParent[i]);
  }
  yyprintf("\n");
}


/*
 * Find a string in a string array.
 * Returns the array index if found, -1 otherwise.
 */
int getIndexInMRParents(SymbolTblNode * s, MRParents * p) {
  int i;
  for (i = 0; i < p->count; i ++) {
    if (s == p->parents[i]->snode) return i;
  }
  return -1;
}


/*
 * Determines if string s is in an array a of length count.
 */
BOOL isInMRParents(SymbolTblNode * s, MRParents * p) {
  return (getIndexInMRParents(s, p) >= 0);
}


BOOL isParentSymbol(SymbolTblNode * s) {
  return isInMRParents(s, all_parents);
}


MRTreeNode * findNodeInTree(MRTreeNode * node, SymbolTblNode * symbol) {
  if (node == NULL) {
    //printf("findNodeInTree warning: node is null\n");
    return NULL;
  }
  if (node->symbol->snode == symbol) {
    return node;
  } else { // Search parent nodes.
    int i;
    MRTreeNode * p_node;
    for (i = 0; i < node->parent_count; i ++) {
      p_node = findNodeInTree(node->parent[i], symbol);
      if (p_node != NULL) return p_node;
    }
    return NULL;
  }
}


MRTreeNode * findNodeInForest(SymbolTblNode * symbol) {
  int i;
  MRTreeNode * node;
  for (i = 0; i < MRLeaves_count; i ++) {
    node = findNodeInTree(MRLeaves[i], symbol);
    if (node != NULL) return node;
  }
  return NULL;
}


MRTreeNode * createMRTreeNode(SymbolTblNode * symbol) {
  MRTreeNode * node = (MRTreeNode *) malloc(sizeof(MRTreeNode));
  if (node == NULL) {
    printf("createMRTreeNode error: out of memory\n");
    exit(0);
  }
  node->parent = (MRTreeNode **) 
    malloc(sizeof(MRTreeNode *) * MRTreeNode_INIT_PARENT_COUNT);
  if (node->parent == NULL) {
    YYERR_EXIT("createMRTreeNode error: out of memory\n");
  }
  node->parent_count = 0;
  node->parent_max_count = MRTreeNode_INIT_PARENT_COUNT;
  node->symbol = createSymbolNode(symbol);
  return node;
}


void destroyMRTreeNode(MRTreeNode * node) {
  if (node == NULL) return;
  free(node->symbol);
  free(node->parent);
  free(node);
}


/* 
 * Insert a new tree to the Multi-rooted forest.
 * The new tree's root node contains symbol parent,
 * and leaf node contains symbol child.
 */
void insertNewTree(SymbolTblNode * parent, SymbolTblNode * child) {
  MRTreeNode * leaf = createMRTreeNode(child);
  leaf->parent[0] = createMRTreeNode(parent);
  leaf->parent_count = 1;

  // Insert node leaf to the MRLeaves array.
  checkMRLeavesArraySize();
  MRLeaves[MRLeaves_count] = leaf;
  MRLeaves_count ++;
}


/*
 * If the parent array of node reaches size limit,
 * expand it.
 */
void checkParentArraySize(MRTreeNode * node) {
  if (node->parent_count < node->parent_max_count) return;

  node->parent_max_count *= 2;
  node->parent = (MRTreeNode **) realloc((void *) node->parent,
              sizeof(MRTreeNode *) * node->parent_max_count);
  if (node->parent == NULL)
    YYERR_EXIT("checkParentArraySize: out of memory\n");

  if (DEBUG_EXPAND_ARRAY)
    printf("MRTreeNode.parent size expanded to %d\n",
           node->parent_max_count);
}


void insertParent(MRTreeNode * node, SymbolTblNode * symbol) {
  MRTreeNode * parent;
  checkParentArraySize(node);
  parent = createMRTreeNode(symbol);
  node->parent[node->parent_count] = parent;
  node->parent_count ++;
}


/*
 * Returns the index in array MRLeaves[] if given node
 * is a leaf, otherwise returns -1.
 */
int isMRLeaf(MRTreeNode * node) {
  int i;
  for (i = 0; i < MRLeaves_count; i ++) {
    if (node == MRLeaves[i]) return i;
  }
  return -1;
}


void writeBranch(SymbolList branch) {
  SymbolNode * a;
  for (a = branch; a != NULL; a = a->next) {
    if (a != branch) yyprintf(", ");
    yyprintf2("%s", a->snode->symbol);
  }
  if (a != branch) yyprintf(", ");
}


/*
 * Prints out all node sequences starting from 
 * the given node to its ancestors.
 */
void writeLeafBranch(MRTreeNode * node, 
    SymbolList branch, SymbolNode * branch_tail) {

  int i;
  if (node == NULL) {
    //printf("writeLeafBranch warning: node is NULL\n");
    return;
  }
  yyprintf2("%s", node->symbol->snode->symbol);
  if (node->parent_count == 0) {
    yyprintf("\n");
    return;
  }
  yyprintf(", ");

  if (branch->next == NULL) {
    branch_tail->next = branch->next = 
      createSymbolNode(node->symbol->snode);
  } else {
    branch_tail->next->next = createSymbolNode(node->symbol->snode);
    branch_tail->next = branch_tail->next->next;
  }
   
  for (i = 0; i < node->parent_count; i ++) {
    if (i > 0) writeBranch(branch->next);
    writeLeafBranch(node->parent[i], branch, branch_tail);
  }
}


void writeMRForest() {
  int i;
  SymbolList branch = createSymbolNode(hashTbl_find(""));
  SymbolNode * branch_tail = createSymbolNode(hashTbl_find(""));

  yyprintf2("\n==writeMRForest (MRLeaves_count: %d)==\n",
         MRLeaves_count);
  for (i = 0; i < MRLeaves_count; i ++) {
    writeLeafBranch(MRLeaves[i], branch, branch_tail);
  }

  freeSymbolNodeList(branch);
  freeSymbolNode(branch_tail);
}


/*
 * Insert a child node of treeNode. 
 * The child node contains the given symbol.
 */
void insertChild(MRTreeNode * treeNode, SymbolTblNode * symbol) {
  MRTreeNode * child = createMRTreeNode(symbol);
  int leaf_index = isMRLeaf(treeNode);

  if (leaf_index == -1) {
    //treeNode not a leaf, just insert child as a leaf.
    checkMRLeavesArraySize();
    MRLeaves[MRLeaves_count] = child;
    MRLeaves_count ++;
  } else { 
    //change the pointer to treeNode to point to child 
    MRLeaves[leaf_index] = child;
  }

  child->parent[0] = treeNode;
  child->parent_count = 1;
}


/* 
 * Both parent and child nodes are in the Multi-rooted
 * forest already, just add the link between them.
 */
void insertParentChildRelation(MRTreeNode * parent,
                               MRTreeNode * child) {
  int leaf_index;

  checkParentArraySize(child);
  child->parent[child->parent_count] = parent;
  child->parent_count ++;

  // if parent node is a leaf, remove it from the leaves array.
  leaf_index = isMRLeaf(parent);
  if (leaf_index != -1) {
    int i;
    for (i = leaf_index; i < MRLeaves_count - 1; i ++) {
      MRLeaves[i] = MRLeaves[i + 1];
    }
    MRLeaves_count --;
  } 
}


void buildMultirootedTree() {
  MRTreeNode * lhs, * rhs;
  Grammar * g = & grammar;
  int i;

  // initialization.
  all_parents = createMRParents();
  initArray_leafIndexForParent();
  createMRLeavesArray();

  for (i = 1; i < g->rule_count; i ++) {
    if (isUnitProduction(i) == TRUE) {
      lhs = findNodeInForest(g->rules[i]->nLHS->snode);
      rhs = findNodeInForest(g->rules[i]->nRHS_head->snode);
      if (lhs != NULL && rhs == NULL) {
        // insert rhs as child of lhs.
        insertChild(lhs, g->rules[i]->nRHS_head->snode);
      } else if (lhs == NULL && rhs != NULL) {
        // insert lhs as parent of rhs.
        insertParent(rhs, g->rules[i]->nLHS->snode);
      } else if (lhs == NULL && rhs == NULL) {
        // insert as new tree.
        insertNewTree(g->rules[i]->nLHS->snode, 
                      g->rules[i]->nRHS_head->snode);
      } else { // just add this relationship.
        insertParentChildRelation(lhs, rhs);
      } // end if
    } // end if
  } // end for

  getAllMRParents();

  if (DEBUG_BUILD_MULTIROOTED_TREE) {
    writeMRForest();
    writeAllMRParents();
  }
}


/*
 * Adds node itself and all its parent nodes to array
 * parents[] if not already in the array.
 *
 * Called by function getParentsForMRLeaf() only.
 */
void getNode(int leaf_index, MRTreeNode * node, 
             MRParents * parents) {
  if (node == NULL) {
    //printf("getNode warning: node is NULL\n");
    return;
  }

  if (isInMRParents(node->symbol->snode, parents) == FALSE) {
    checkArraySize_MRParents(parents); // expand size if needed.

    if (leafIndexForParent_Done == FALSE) {
      leafIndexForParent[parents->count] = leaf_index;
    }

    parents->parents[parents->count] = 
      createSymbolNode(node->symbol->snode);
    parents->count ++;
  }

  if (node->parent_count > 0) {
    int i;
    for (i = 0; i < node->parent_count; i ++) {
      getNode(leaf_index, node->parent[i], parents);
    }
  }
}


/*
 * Obtains an array of all the parent nodes of the
 * give leaf node.
 * 
 * Note: so far this is called for each cycle of states,
 *       so may not be very efficient.
 */
void getParentsForMRLeaf(int leaf_index,
        MRParents * parents) {
  int i;
  MRTreeNode * node = MRLeaves[leaf_index];
  if (node->parent_count == 0) return;

  for (i = 0; i < node->parent_count; i ++) {
    getNode(leaf_index, node->parent[i], parents);
  }
}


/*
 * Get all the parent nodes in the multi-rooted forest.
 */
void getAllMRParents() {
  int i;
  leafIndexForParent_Done = FALSE;

  for (i = 0; i < MRLeaves_count; i ++) {
    getParentsForMRLeaf(i, all_parents);
  }

  leafIndexForParent_Done = TRUE;
}


/*
 * Prints a list of all the parent nodes in the 
 * multi-rooted forest, as well as the leaf
 * of each parent in parenthesis.
 */
void writeAllMRParents() {
  int i;
  yyprintf("\n==all MR Parents (inside '()' is a corresponding leaf):\n");
  for (i = 0; i < all_parents->count; i ++) {
    yyprintf3("%s (=>%s)\n", all_parents->parents[i]->snode->symbol, 
           MRLeaves[leafIndexForParent[i]]->symbol->snode->symbol);
  }
  yyprintf("\n");
  /* writeLeafIndexForParent(); */
}


/*
 * Prints a list of all the parent nodes of leaf.
 *
 * Pre-assumption: the list of parent nodes of the given
 * leaf is contained the array parents[].
 */
void writeMRParents(MRTreeNode * leaf, MRParents * parents) {
  int i;
  yyprintf2("parents for leaf '%s': ", 
            leaf->symbol->snode->symbol);
  for (i = 0; i < parents->count; i ++) {
    if (i > 0) yyprintf(", ");
    yyprintf2("%s", parents->parents[i]->snode->symbol);
  }
  yyprintf("\n");
}




