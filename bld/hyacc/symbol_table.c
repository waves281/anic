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
 * symbol_table.c 
 *
 * Stores all symbols of the grammar and relevant information
 * of these symbols.
 *
 * @Author: Xin Chen
 * @Created on: 2/16/2007
 * @Last modified: 3/21/2007
 * @Copyright (C) 2007
 */

#include "y.h"

#define DEBUG_HASHTBL 0


/*******************************************
 * Functions for RuleIDNode.
 *******************************************/


RuleIDNode * createRuleIDNode(int ruleID) {
  RuleIDNode * r;
  r = (RuleIDNode *) malloc(sizeof(RuleIDNode));
  if (r == NULL) 
    YYERR_EXIT("createRuleIDNode: out of memory\n");

  r->ruleID = ruleID;
  r->next = NULL;
  return r;
}


void writeRuleIDList(SymbolTblNode * n) {
  RuleIDNode * a;
  printf("%s: ", n->symbol);
  if ((a = n->ruleIDList) != NULL) {
    printf("%d", a->ruleID);

    for (a = a->next; a != NULL; a = a->next) {
      printf(", %d", a->ruleID);
    }
  }
  printf("\n");
}


void writeNonTerminalRuleIDList() {
  SymbolNode * a;
  int count = 0;
  printf("--Nonterminal symbol rule index list--\n");
  for (a = grammar.non_terminal_list; a != NULL; a = a->next) {
    printf("%d: ", count ++);
    writeRuleIDList(a->snode);
  }
}


void destroyRuleIDList(RuleIDNode * r) {
  RuleIDNode * a, * b;

  if (r == NULL) return;

  a = r;
  while (a != NULL) {
    b = a->next;
    free(a);
    a = b;
  }
}


/*******************************************
 * Functions for SymbolNode.
 *******************************************/


/*
 * Create a symbol node, used by Production, Context etc.
 */
SymbolNode * createSymbolNode(SymbolTblNode * sn) {
  SymbolNode * n;

  if (sn == NULL) 
    YYERR_EXIT("createSymbolNode error: snode is NULL\n");

  n = (SymbolNode *) malloc(sizeof(SymbolNode));
  if (n == NULL) 
    YYERR_EXIT("createSymbolNode error: out of memory\n");
  n->snode = sn;
  n->next = NULL;
  return n;
}


void freeSymbolNode(SymbolNode * n) {
  if (n == NULL) return;
  free(n);
}


void freeSymbolNodeList(SymbolNode * a) {
  SymbolNode * b;
  if (a == NULL) return;

  while (a != NULL) {
    b = a->next;
    free(a);
    a = b;
  }
}


SymbolNode * findInSymbolList(SymbolList a, SymbolTblNode * s) {
  SymbolNode * b;

  for (b = a; b != NULL; b = b->next) {
    if (b->snode == s) return b;
  }

  return NULL;
}


int getSymbolListLen(SymbolList a) {
  int len;
  SymbolNode * b;

  len = 0;
  for (b = a; b != NULL; b = b->next) { len ++; }
  return len;
}


/*
 * Given a symbol list a, returns a clone of it.
 */
SymbolList cloneSymbolList(const SymbolList a) {
  SymbolNode * b, *c, * clone;
  if (NULL == a) return NULL;

  clone = c = createSymbolNode(a->snode);
  for (b = a->next; b != NULL; b = b->next) {
    c->next = createSymbolNode(b->snode);
    c = c->next;
  }

  return clone;
}

/*
 * Remove s from list a.
 * @return: the new list.
 */
SymbolList removeFromSymbolList(
    SymbolList a, SymbolTblNode * s, int * exist) {
  SymbolNode * b, * tmp;
  * exist = 1;

  if (a->snode == s) { // is the first node.
    b = a;
    a = a->next;
    freeSymbolNode(b);
    return a;
  }

  for (b = a; b->next != NULL; b = b->next) {
    if (b->next->snode == s) {
      tmp = b->next;
      b->next = tmp->next;
      freeSymbolNode(tmp);
      return a;
    }
  }

  // b->next is NULL. s is NOT in list a.
  * exist = 0;
  return a;
}


/*
 * Find in a sorted (INCREMENTAL) list.
 */
SymbolNode * findInIncSymbolList(SymbolList a, SymbolTblNode * s) {
  SymbolNode * b;

  for (b = a; b != NULL; b = b->next) {
    if (b->snode == s) return b;
    else if (strcmp(s->symbol, b->snode->symbol) > 0) {
      return NULL;
    }
  }

  return NULL;
}


/*
 * Insert symbol n into INC list a.
 * @Return: the result list.
 */
SymbolNode * insertIncSymbolList(SymbolList a, SymbolTblNode * n) {
  int cmp;
  SymbolNode * b, * b_prev;
  if (NULL == n) return a;
  if (NULL == a) return createSymbolNode(n);

  for (b_prev = NULL, b = a; b != NULL; b_prev = b, b = b->next) {
    cmp = strcmp(n->symbol, b->snode->symbol);
    if (cmp < 0) { // insert after b_prev, before b.
      if (b_prev == NULL) { // insert at the head.
        b_prev = createSymbolNode(n);
        b_prev->next = b;
        return b_prev;
      } else { // insert in the middle.
        b_prev->next = createSymbolNode(n);
        b_prev->next->next = b;
        return a;
      }
    } else if (cmp > 0) { // go on.
      continue;
    } else { // equals. already exists.
      return a;
    }
  }

  // b is NULL. insert at the end.
  b_prev->next = createSymbolNode(n);
  
  return a;
}


/*
 * Combine list b into a. Both lists are in INC order.
 * Returns the head of the combined list.
 * This can be used when combining contexts.
 */
SymbolNode * combineIncSymbolList(SymbolList a, SymbolList b) {
  SymbolNode * na, * nb, * na_prev;
  int cmp;
  if (a == NULL) return cloneSymbolList(b);
  if (b == NULL) return a;

  na_prev = NULL;
  na = a;
  nb = b;

  while (1) {
    cmp = strcmp(na->snode->symbol, nb->snode->symbol);
    //printf("strcmp(%s, %s) = %d\n", 
    //       na->snode->symbol, nb->snode->symbol, cmp);
    if (cmp == 0) {
      na_prev = na;
      na = na->next;
      nb = nb->next;
    } else if (cmp > 0) { // insert b before na.
      if (na_prev == NULL) { // insert at the head of a.
        na_prev = createSymbolNode(nb->snode);
        na_prev->next = a;
        a = na_prev;
      } else { // insert in the middle of list a before na.
        na_prev->next = createSymbolNode(nb->snode);
        na_prev->next->next = na;
      }
      nb = nb->next;
    } else { // cmp < 0.
      na_prev = na;
      na = na->next;
    }

    if (na == NULL) { 
      na_prev->next = cloneSymbolList(nb); // attach the rest of nb.
      break; 
    } else if (nb == NULL) { 
      break; 
    }
    //writeSymbolList(a, "a");
  }

  return a;
}


void writeSymbolList(SymbolList a, char * name) {
  SymbolNode * b = a;
  if (name != NULL) printf("%s: ", name);
  if (b != NULL) {
    printf("%s", b->snode->symbol);
    for (b = b->next; b != NULL; b = b->next) {
      printf(", %s", b->snode->symbol);
    }
    printf("\n");
  }
}


/*******************************************
 * Function for SymbolTblNode.
 *******************************************/

/*
 * create a symbol table node, used by hash table HashTbl.
 */
SymbolTblNode * createSymbolTblNode(char * symbol) {
  SymbolTblNode * n;

  if (symbol == NULL) 
    YYERR_EXIT("createSymbolTblNode error: symbol is NULL\n");

  n = (SymbolTblNode *) malloc(sizeof(SymbolTblNode));
  if (n == NULL)
    YYERR_EXIT("createSymbolTblNode error: out of memory\n");
  n->symbol = (char *) malloc(sizeof(char) * (strlen(symbol) + 1));
  strcpy(n->symbol, symbol);
  n->next = NULL;
  n->type = _NEITHER;
  n->vanishable = 0; // default value: FALSE
  n->seq = -1;
  n->ruleIDList = NULL;
  n->TP = NULL; // terminal property.
  n->value = 0;
  return n;
}


/*
 * Empty string and EOF '$' are _NEITHER type.
 */
static char * getSymbolType(SymbolTblNode * n) {
  if (n->type == _TERMINAL) return "T";
  if (n->type == _NONTERMINAL) return "NT";
  return "-";
}


static char * getAssocName(associativity a) {
  if (a == _LEFT) return "left";
  if (a == _RIGHT) return "right";
  if (a == _NONASSOC) return "none";
  return "--unknonw--";
}


/*******************************************
 * Hash table functions.
 *******************************************/

void testHashTbl() {
  hashTbl_insert("xin");
  hashTbl_insert("abe");
  hashTbl_insert("xin");
  hashTbl_insert("");
  hashTbl_insert("");
  
  hashTbl_find("");
  hashTbl_find("abe");
  hashTbl_find("ooo");
  
  hashTbl_dump();
  hashTbl_destroy();
  printf("---------------\n");
}


void hashTbl_init() {
  int i;
  for (i = 0; i < HT_SIZE; i ++) {
    HashTbl[i].count = 0;
    HashTbl[i].next = NULL;
  }

#if DEBUG_HASHTBL
    printf("size of hash table = %d\n", sizeof(HashTbl));
#endif
  //testHashTbl();
}


/*
 * Assumption: symbol != NULL.
 * empty string is allowed.
 */
static int hash_value(char * symbol) {
  int i, sum;
  int len = strlen(symbol);

  sum = 0;
  for (i = 0; i < len; i ++) {
    sum = (sum + symbol[i]) % HT_SIZE;
  }
  
#if DEBUG_HASHTBL
  printf("hash value for %s is %d\n", symbol, sum);
#endif

  return sum;
}


/*
 * If the symbol exists, return the node,
 * otherwise create a node and return the node.
 *
 * So this contains the function of hashTbl_find().
 * If it's unknown whether a symbol exists, use
 * this function to ensure getting a node contains 
 * this symbol.
 */
SymbolTblNode * hashTbl_insert(char * symbol) {
  int v;
  SymbolTblNode * n;

  if (symbol == NULL) return NULL;
  v = hash_value(symbol);

  if (HashTbl[v].next == NULL) {
    HashTbl[v].next = createSymbolTblNode(symbol);
    HashTbl[v].count ++;
    return HashTbl[v].next;
  } else {
    for (n = HashTbl[v].next; n->next != NULL; n = n->next) {
      if (strcmp(n->symbol, symbol) == 0) {
#if DEBUG_HASHTBL
        printf("node for string %s exists\n", symbol);
#endif
        return n;
      }
    }
    // the last node on this linked list.
    if (strcmp(n->symbol, symbol) == 0) {
#if DEBUG_HASHTBL
      printf("node for string %s exists\n", symbol);
#endif
      return n;
    }
    n->next = createSymbolTblNode(symbol);
    HashTbl[v].count ++;
    return n->next;
  }
}


/*
 * Return the node for symbol.
 * If symbol does not exist, return NULL.
 */
SymbolTblNode * hashTbl_find(char * symbol) {
  int v;
  SymbolTblNode * n;

  if (symbol == NULL) return NULL;
  v = hash_value(symbol);

  for (n = HashTbl[v].next; n != NULL; n = n->next) {
    if (strcmp(n->symbol, symbol) == 0) {
#if DEBUG_HASHTBL
      printf("node for %s is found\n");
#endif
      return n;
    }
  }

#if DEBUG_HASHTBL
  printf("node for %s is NOT found\n", symbol);
#endif

  return NULL;
}


void hashTbl_destroy() {
  int i;
  SymbolTblNode * n, * nnext;

  //printf("--destroy hash table--\n");
  for (i = 0; i < HT_SIZE; i ++) {
    if (HashTbl[i].count > 0) {
      for (n = HashTbl[i].next; n != NULL; n = nnext) {
        nnext = n->next;
        //printf("freeing node for %s\n", n->symbol);
        free(n->symbol);
        destroyRuleIDList(n->ruleIDList);
        free(n);
      }
    }
  }
}


void SymbolTblNode_dump(SymbolTblNode * n) {
  yyprintf6("%s [type=%s,vanish=%s,seq=%d,val=%d", 
            n->symbol, getSymbolType(n), 
            (n->vanishable == 1) ? "T" : "F", 
            n->seq, n->value);
  if (n->type == _TERMINAL && n->TP != NULL) {
    yyprintf3(",prec=%d,assoc=%s", n->TP->precedence, 
             getAssocName(n->TP->assoc));
  }
  yyprintf("]");
}


void hashTbl_dump() {
  int i, count = 0, list_count = 0;
  SymbolTblNode * n;

  yyprintf("\n\n--Hash table--\n");
  for (i = 0; i < HT_SIZE; i ++) {
    if (HashTbl[i].count > 0) {
      list_count ++;
      yyprintf3("HashTbl[%d] (count=%d): ", i, HashTbl[i].count);
      for (n = HashTbl[i].next; n->next != NULL; n = n->next) {
        SymbolTblNode_dump(n);
        yyprintf(", ");
        count ++;
      }
      SymbolTblNode_dump(n);
      yyprintf("\n");
      count ++;
    }
  }
  yyprintf2("--hash table size: %d--\n", HT_SIZE);
  yyprintf5("--symbol count: %d, load factor lamda (%d/%d) = %.3f--\n",
         count, count, HT_SIZE, ((double) count) / HT_SIZE);
  yyprintf5("--list count: %d. Hash Table cell usage (%d/%d) = %.3f--\n",
            list_count, list_count, HT_SIZE, 
            ((double) list_count) / HT_SIZE);
  yyprintf2("--symbols per list: %.3f--\n", 
         ((double) count) / list_count);

  //hashTbl_destroy();
}


