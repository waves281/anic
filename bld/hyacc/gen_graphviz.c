/*
   This file is part of Hyacc, a LR(0)/LALR(1)/LR(1) parser generator.
   Copyright (C) 2007 Xin Chen. chenx@hawaii.edu

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
 * graphviz.c
 *
 * To produce an input file for graphviz.
 * 
 * @author: Xin Chen
 * @created on: 12/14/2007
 * @last modified: 12/15/2007
 * @Copyright (C) 2007
 */

#include "y.h"

#define DEBUG_GEN_GVIZ 0


typedef struct _gvNode gvNode;
struct _gvNode {
  int target_state;
  SymbolNode * labels;
  SymbolNode * labels_tail;
  gvNode * next;
};


static gvNode * createGvNode(int targetState) {
  gvNode * gvn;
  HYY_NEW(gvn, gvNode, 1);
  gvn->target_state = targetState;
  gvn->labels = NULL;
  gvn->labels_tail = NULL;
  gvn->next = NULL;
  return gvn;
}


static void destroyGvNode(gvNode * n) {
  if (NULL == n) return;
  freeSymbolNodeList(n->labels);
  free(n);
}


static void destroyGvNodeList(gvNode * list) {
  gvNode * tmp;
  if (NULL == list) return;
  while ((tmp = list) != NULL) {
    list = list->next;
    destroyGvNode(tmp);
  }
}


static gvNode * findGvNodeInList(gvNode * list, int targetState) {
  if (NULL == list) return NULL;
  while (list != NULL) {
    if (list->target_state == targetState) return list;
    list = list->next;
  }
  return NULL;
}


/*
 * find label in label list.
 */
static void insertLabelToList(gvNode * n, SymbolTblNode * snode) {
  if (NULL == n || NULL == snode) return; // should not happen.

  if (NULL == n->labels) {
    n->labels = n->labels_tail = createSymbolNode(snode);
  } else {
    SymbolNode * m = n->labels;
    while (m->next != NULL) {
      if (m->snode == snode) return; // already exists.
      m = m->next;
    }
    // now m->next == NULL
    if (m->snode == snode) return; // exists as the last one.
    // else, not exist.
    m->next = createSymbolNode(snode);
  }
}


/*
 * If exists a node n in list, s.t.
 *   n->target_state == targetState, then add snode to n->label_list.
 * else
 *   create a new node and insert it to list.
 */
static gvNode * addGvNodeToList(gvNode * list, 
    int targetState, SymbolTblNode * snode) {
  gvNode * n = findGvNodeInList(list, targetState);
  if (NULL == n) { // targetState NOT found. Add to list.
#if DEBUG_GEN_GVIZ
    printf("target state %d not found (label %s)\n", 
           targetState, snode->symbol);
#endif
    n = createGvNode(targetState);
    n->labels = n->labels_tail = createSymbolNode(snode);
    if (NULL == list) { // first node in list.
      list = n;
    } else { // add to tail of list.
      gvNode * m = list;
      while (m->next != NULL) { m = m->next; }
      m->next = n;
    }
  } else { // found.
    // add snode to the label list of n.
#if DEBUG_GEN_GVIZ
    printf("target state %d found, add label %s\n", 
           targetState, snode->symbol);
#endif
    insertLabelToList(n, snode);
  }
  return list;
}


/* 
 * dump r list 
 */
static void dumpGvNodeList_r(
    gvNode * list, int srcState, FILE * fp) {
  SymbolNode * labels;
  if (NULL == list) return;
  while (list != NULL) {
    fprintf(fp, "  %d -> r%d [ label=\"", 
            srcState, list->target_state);
    // write label list
    for (labels = list->labels; 
         labels->next != NULL; labels = labels->next) {
      fprintf(fp, "%s,", labels->snode->symbol);
    }
    fprintf(fp, "%s", labels->snode->symbol);
    fprintf(fp, "\" style=\"dashed\" ];\n");
    list = list->next;
  }
}


/*
 * dump s list
 */
static void dumpGvNodeList_s(
    gvNode * list, int srcState, FILE * fp) {
  SymbolNode * labels;
  if (NULL == list) return;
  while (list != NULL) {
    fprintf(fp, "  %d -> %d [ label=\"", 
            srcState, list->target_state);
    // write label list
    for (labels = list->labels; 
         labels->next != NULL; labels = labels->next) {
      fprintf(fp, "%s,", labels->snode->symbol);
    }
    fprintf(fp, "%s", labels->snode->symbol);
    fprintf(fp, "\" ];\n");
    list = list->next;
  }
}


static int getGvNodeListLen(gvNode * list) {
  int len = 0;

  while (list != NULL) {
    list = list->next;
    len ++;
  }

  return len;
}


/*
 * Update the reduction list if the following is satisfied:
 * 
 * In case of --lr0 or --lalr: 
 *   If a reduction is the single only REDUCTION at this state,
 *   replace it by ".". Similar to the use of final_state_list
 *   in writing y.output. 3-11-2008.
 * Else (LR(1)):
 *   If a reduction is the single only ACTION at this state,
 *   do the same as above.
 *
 * @created on: 3/11/2008
 */
static gvNode * update_r_list(gvNode * r_list, gvNode * s_list) {
  int state;
  char * strAny = "(any)"; // means: any terminal can cause reduction.

  if (USE_LR0 || USE_LALR) {
    if (getGvNodeListLen(r_list) == 1) {
      state = r_list->target_state;
      destroyGvNodeList(r_list);
      r_list = NULL;
      r_list = addGvNodeToList(r_list, state, hashTbl_insert(strAny));
    }
  } else {
    if (s_list == NULL && getGvNodeListLen(r_list) == 1) {
      state = r_list->target_state;
      destroyGvNodeList(r_list);
      r_list = NULL;
      r_list = addGvNodeToList(r_list, state, hashTbl_insert(strAny));
    }
  }

  return r_list;
}


/*
 * Has the same logic as printParsingTable() in y.c.
 * For O0, O1.
 */
void gen_graphviz_input() {
  char action;
  int state;
  int row, col;
  int row_size = ParsingTblRows; 
  int col_size = ParsingTblCols;
  SymbolTblNode * n;
  gvNode * r_list;
  gvNode * s_list;
  FILE * fp_gviz;

  if ((fp_gviz = fopen(y_gviz, "w")) == NULL) {
    printf("cannot open file %s\n", y_gviz);
    return;
  }

  fputs("digraph abstract {\n\n", fp_gviz);
  fputs("  node [shape = doublecircle]; 0 acc;\n", fp_gviz);
  fputs("  node [shape = circle];\n", fp_gviz);

  for (row = 0; row < row_size; row ++) {
    r_list = NULL;
    s_list = NULL;

    for (col = 0; col < ParsingTblCols; col ++) {
      n = ParsingTblColHdr[col];
      if (isGoalSymbol(n) == FALSE) {
        getAction(n->type, col, row, & action, & state);
        /* printf("%c%d\t", action, state); */
        if (action == 0) {
          /* do nothing */
        } else if (action == 'r') {
          r_list = addGvNodeToList(r_list, state, n);
        } else if (action == 's' || action == 'g') {
          s_list = addGvNodeToList(s_list, state, n);
        } else if (action == 'a') {
          fprintf(fp_gviz, "  %d -> acc [ label = \"%s\" ];\n", 
                  row, n->symbol);
        }
      } // end of if
    } // end of for.

    r_list = update_r_list(r_list, s_list);

    dumpGvNodeList_r(r_list, row, fp_gviz);
    dumpGvNodeList_s(s_list, row, fp_gviz);
    destroyGvNodeList(r_list);
    destroyGvNodeList(s_list);
  }

  fputs("\n}\n", fp_gviz);
  fclose(fp_gviz);
}


/*
 * Has the same logic as printCondensedFinalParsingTable() in y.c.
 * For O2, O3.
 */
void gen_graphviz_input2() {
  SymbolTblNode * n;
  char action;
  int row, col, i, state, srcState;
  int col_size = ParsingTblCols;
  /* value assigned at the end of generate_parsing_table(). */
  int row_size = ParsingTblRows;
  gvNode * r_list;
  gvNode * s_list;
  FILE * fp_gviz;

  if ((fp_gviz = fopen(y_gviz, "w")) == NULL) {
    printf("cannot open file %s\n", y_gviz);
    return; 
  }

  fputs("digraph abstract {\n\n", fp_gviz);
  fputs("  node [shape = doublecircle]; 0 acc;\n", fp_gviz);
  fputs("  node [shape = circle];\n", fp_gviz);

  i = 0;
  for (row = 0; row < row_size; row ++) {
    r_list = NULL;
    s_list = NULL;
    if (isReachableState(row) == TRUE) {
      for (col = 0; col < ParsingTblCols; col ++) {
        n = ParsingTblColHdr[col];
        if (isGoalSymbol(n) == FALSE && isParentSymbol(n) == FALSE) {
          getAction(n->type, col, row, & action, & state);
          if (action == 's' || action == 'g')
            state = getActualState(state);
          /* yyprintf3("%c%d\t", action, state); */
          if (action == 0) {
            /* do nothing */
          } else if (action == 'r') {
            r_list = addGvNodeToList(r_list, state, n);
          } else if (action == 's' || action == 'g') {
            s_list = addGvNodeToList(s_list, state, n);
          } else if (action == 'a') {
            fprintf(fp_gviz, "  %d -> acc [ label = \"%s\" ];\n", 
                    row, n->symbol);
          }
        }
      }
      i ++;

      r_list = update_r_list(r_list, s_list);

      srcState = getActualState(row);
      dumpGvNodeList_r(r_list, srcState, fp_gviz);
      dumpGvNodeList_s(s_list, srcState, fp_gviz);
      destroyGvNodeList(r_list);
      destroyGvNodeList(s_list);
    } /* end if */
  } 

  fputs("\n}\n", fp_gviz);
  fclose(fp_gviz);
}

