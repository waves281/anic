/*
   This file is part of Hyacc, a LR(0)/LALR(1)/LR(1) parser generator.
   Copyright (C) 2008, 2009 Xin Chen. chenx@hawaii.edu

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
 * Lane tracing functions.
 *
 * @Author: Xin Chen
 * @Created on: 2/28/2008
 * @Last modified: 3/24/2009
 * @Copyright (C) 2008, 2009
 */

#include "stack_config.h"
#include "lane_tracing.h"

#define DEBUG_PHASE_1 0


/* 
 * Variables used for lane tracing. Start. 
 */

static stack * LANE;
static stack * STACK;
static int TRACE_FURTHER;
static int TEST_FAILED;

/*
 * Current final config that is traced in trace_back() function.
 * Used for phase 2 state combining.
 */
Configuration * cur_red_config; 
static BOOL GRAMMAR_AMBIGUOUS = FALSE;

#define FLAG_ON  1
#define FLAG_OFF 0

static Configuration * LT_MARKER = (Configuration *) -1;
static Configuration * LT_ZERO = 0;

extern BOOL in_lanetracing;
BOOL IN_EDGE_PUSHING_LANE_TRACING;
SymbolList EDGE_PUSHING_CONTEXT_GENERATED;

/*
 * Variables used for lane tracing. End. 
 */

static void get_originators(Configuration * c0, Configuration * c);
static void get_transitors(Configuration * c0, Configuration * c);

static void DO_LOOP();
static void CHECK_LANE_TOP();
static int findSuccessorStateNo(int state_no, SymbolTblNode * snode);
void my_writeState(State * s);
static void set_transitors_pass_thru_on(
    Configuration * cur_config, Configuration * o);
static BOOL inheritParentContext(State * s, State * parent);


/*
 * Initialize this to NULL at the beginning of phase2_regeneration2().
 * This list is in the order of cluster insertion.
 */
LT_cluster * all_clusters;
LT_cluster * all_clusters_tail;
LT_cluster * new_cluster;
/*
 * Initialized to TRUE. If in regeneration context conflicts occur,
 * set this to FALSE, which means the grammar is NOT LR(1).
 */
BOOL all_pairwise_disjoint; 


/*
 * Initialize this to NULL at the beginning of lane_tracing_phase2().
 * This list is in INC order on from_state->state_no.
 */
LT_tbl_entry * LT_tbl;

/* declaration of functions */
void dump_LT_tbl();
LT_tbl_entry * LT_tbl_findEntry(int from_state);
BOOL cluster_trace_new_chain_all(int parent_state, LT_tbl_entry * e);


void dump_llist_context_set(llist_context_set * contxt_set) {
  llist_context_set * c = contxt_set;
  SymbolNode * n;
  if (c == NULL) return;

  for (; c != NULL; c = c->next) {
    // first write the config.
    printf("%d.%d:", c->config->owner->state_no, c->config->ruleID);
    // then write the context symbol list.
    printf("{ ");
    for (n = c->ctxt; n != NULL; n = n->next) {
      printf("%s ", n->snode->symbol);
    }
    printf("} ");
  }
}


/*
 * Assumption: contxt_set is in INC order.
 */
llist_context_set * llist_context_set_create(
    Configuration * config) { //, SymbolList contxt_set) {
  llist_context_set * s;
  HYY_NEW(s, llist_context_set, 1);

  s->config = config;
  s->ctxt = NULL; //cloneSymbolList(contxt_set);
  s->next = NULL;
  return s;
}


llist_context_set_destroy(llist_context_set * s) {
  freeSymbolNodeList(s->ctxt);
  free(s);
}


/*
 * Add (merge) contxt_set to c->ctxt.
 */
void llist_context_set_addContext(
    llist_context_set * c, SymbolList contxt_set) {
  SymbolNode * n;
  int exist;

  if (NULL == c) return;

  for (n = contxt_set; n != NULL; n = n->next) {
    //if (strlen(n->snode->symbol) == 0) continue; // ignore empty string.
    c->ctxt = insertSymbolList_unique_inc(c->ctxt, n->snode, & exist);
  }
}


/*
 * Return a clone of c.
 */
llist_context_set * llist_context_set_clone(llist_context_set * c) {
  llist_context_set * d, * d_next, * c_next;

  if (NULL == c) return NULL;

  d_next = d = llist_context_set_create(c->config);
  d->ctxt = cloneSymbolList(c->ctxt);

  for (c_next = c->next; c_next != NULL; c_next = c_next->next) {
    d_next->next = llist_context_set_create(c_next->config);
    d_next->next->ctxt = cloneSymbolList(c_next->ctxt);
    d_next = d_next->next;
  }

  //printf("result d: "); dump_llist_context_set(d); puts("");
  return d;
}


// llist_int functions. START.


llist_int * llist_int_create(int n) {
  llist_int * item;
  HYY_NEW(item, llist_int, 1);
  item->n = n;
  item->next = NULL;
  return item;
}


void llist_int_destroy(llist_int * list) {
  llist_int * tmp;
  while (list != NULL) {
    tmp = list;
    list = list->next;
    free(tmp);
  }
}


/*
 * Add n to the list in INC order.
 */
llist_int * llist_int_add_inc(llist_int * list, int n) {
  llist_int * t, * t_prev;
  int cmp;

  if (list == NULL) { return llist_int_create(n); }
  // else, list is not empty. insert in INC order.

  for (t_prev = NULL, t = list; t != NULL; t_prev = t, t = t->next) {
    cmp = t->n - n;
    if (cmp == 0) return list; // exists already.
    if (cmp > 0) {             // insert before t.
      if (t_prev == NULL) {    // insert as the head.
        list = llist_int_create(n);
        list->next = t;
      } else {                 // insert in the middle
        t_prev->next = llist_int_create(n);
        t_prev->next->next = t;
      }
      return list;
    }
    // else, go on to the next state in list.
  }
  // now is at the end of the list. Add to list tail.
  t_prev->next = llist_int_create(n);

  return list;
}


/*
 * Add n to the head of list.
 * Return: pointer to the head of the list.
 */
llist_int * llist_int_add_head(llist_int * list, int n) {
  llist_int * t;
  t = llist_int_create(n);
  t->next = list;
  return t;
}


void llist_int_dump(llist_int * list) {
  llist_int * x;
  if (list == NULL) return;

  for (x = list; x != NULL; x = x->next) {
    printf("%d ", x->n);
  }
}

// llist_int functions. END.


// llist_int2 functions. START.

llist_int2 * llist_int2_create(int n1, int n2) {
  llist_int2 * item;
  HYY_NEW(item, llist_int2, 1);
  item->n1 = n1;
  item->n2 = n2;
  item->next = NULL;
  return item;
}


void llist_int2_destroy(llist_int2 * list) {
  llist_int2 * tmp;
  while (list != NULL) {
    tmp = list;
    list = list->next;
    free(tmp);
  }
}


/*
 * Add n1, n2 to the list in INC order of n1.
 */
llist_int2 * llist_int2_add_inc(llist_int2 * list, int n1, int n2) {
  llist_int2 * t, * t_prev;
  int cmp;

  if (list == NULL) { return llist_int2_create(n1, n2); }
  // else, list is not empty. insert in INC order.

  for (t_prev = NULL, t = list; t != NULL; t_prev = t, t = t->next) {
    cmp = t->n1 - n1;
    if (cmp == 0) return list; // exists already.
    if (cmp > 0) {             // insert before t.
      if (t_prev == NULL) {    // insert as the head.
        t_prev = list = llist_int2_create(n1, n2);
        list->next = t;
      } else {                 // insert in the middle
        t_prev->next = llist_int2_create(n1, n2);
        t_prev->next->next = t;
      }
      return list;
    }
    // else, go on to the next state in list.
  }
  // now is at the end of the list. Add to list tail.
  t_prev->next = llist_int2_create(n1, n2);

  return list;
}


/*
 * Add n to the head of list.
 * Return: pointer to teh head of the list.
 */
llist_int2 * llist_int2_add_head(llist_int2 * list, int n1, int n2) {
  llist_int2 * t;
  t = llist_int2_create(n1, n2);
  t->next = list;
  return t;
}


/*
 * Find the node in a llist_int list whose first entry is n1.
 */
llist_int2 * llist_int2_find_n1(llist_int2 * list, int n1) {
  llist_int2 * t;
  for (t = list; t != NULL; t = t->next) {
    if (t->n1 == n1) return t;
  }
  return NULL;
}


/*
 * Find the node in a llist_int list whose first entry is n2.
 */
llist_int2 * llist_int2_find_n2(llist_int2 * list, int n2) {
  llist_int2 * t;
  for (t = list; t != NULL; t = t->next) {
    if (t->n2 == n2) return t;
  }
  return NULL;
}


void llist_int2_dump(llist_int2 * list) {
  llist_int2 * x;
  if (list == NULL) return;

  for (x = list; x != NULL; x = x->next) {
    printf("(%d, %d) ", x->n1, x->n2);
  }
}

// llist_int2 functions, END.


LT_tbl_entry * LT_tbl_entry_create(State * from, State * to) {
  LT_tbl_entry * e;
  HYY_NEW(e, LT_tbl_entry, 1);
  e->from_state = from->state_no;

  if (e->from_state != 
        states_new_array->state_list[e->from_state]->state_no) {
    printf("ERROR (create_LT_tbl_entry): state_no not equal!\n");
    exit(1);
  }

  e->processed = FALSE;
  e->ctxt_set = NULL;
  e->to_states = (NULL == to) ? NULL : llist_int_create(to->state_no);
  e->next = NULL;
  return e;
}


/*
 * A LT_tbl_entry can have more than one to_state.
 * add to_state in increasing order of the state_no.
 */
void LT_tbl_entry_addToState(LT_tbl_entry * e, State * to) {
  if (to == NULL || e == NULL) return;
  e->to_states = llist_int_add_inc(e->to_states, to->state_no);
}


/*
 * Add an entry (from_state, to_state) to the LT_tbl, 
 * don't add the (config, context) information here.
 */
static void LT_tbl_entry_add(State * from, State * to) {
  LT_tbl_entry * e, * e_prev;
  if (LT_tbl == NULL) {
    LT_tbl = LT_tbl_entry_create(from, to);
    return;
  }
  // search if the from state already exists.
  for (e_prev = NULL, e = LT_tbl; e != NULL;
       e_prev = e, e = e->next) {
    if (e->from_state == from->state_no) {
      LT_tbl_entry_addToState(e, to); // add to state if not on list.
      return;
    }
    if (e->from_state > from->state_no) { // insert before e.
      if (e_prev == NULL) { // insert as the head.
        LT_tbl = LT_tbl_entry_create(from, to);
        LT_tbl->next = e;
      } else { // insert between e_prev and e
        e_prev->next = LT_tbl_entry_create(from, to);
        e_prev->next->next = e;
      }
      return;
    }
  }
  // now is at the end of the table LT_tbl, add to list tail.
  e_prev->next = LT_tbl_entry_create(from, to);
  return;
}


/*
 * Find from state in the LT_tbl. 
 * If not found, insert it.
 */
LT_tbl_entry * LT_tbl_entry_find_insert(State * from) {
  LT_tbl_entry * e, * e_prev;
  if (NULL == LT_tbl) { //insert as the first
    LT_tbl = LT_tbl_entry_create(from, NULL);
    return LT_tbl;
  }
  
  for (e_prev = NULL, e = LT_tbl; e != NULL; e_prev = e, e = e->next) {
    if (e->from_state == from->state_no) return e;
    if (e->from_state > from->state_no) { // insert here.
      if (e_prev == NULL) { // insert as the first.
        LT_tbl = LT_tbl_entry_create(from, NULL);
        LT_tbl->next = e;
        return LT_tbl;
      } else { // insert in the middle.
        e_prev->next = LT_tbl_entry_create(from, NULL);
        e_prev->next->next = e;
        return e_prev->next;
      }
    } 
    // else, go on to check the next entry.
  }

  // otherwise, insert at the end.
  e_prev->next = LT_tbl_entry_create(from, NULL);
  return e_prev->next;
}


/*
 * Find from state in the LT_tbl.
 * Same as LT_tbl_entry_find_insert() except that this has no insert.
 *
 * There can be at most one entry found.
 */
LT_tbl_entry * LT_tbl_entry_find(State * from) {
  LT_tbl_entry * e, * e_prev;
  if (NULL == LT_tbl) { //insert as the first
    LT_tbl = LT_tbl_entry_create(from, NULL);
    return LT_tbl;
  }

  for (e_prev = NULL, e = LT_tbl; e != NULL; e_prev = e, e = e->next) {
    if (e->from_state == from->state_no) return e;
    if (e->from_state > from->state_no) { // insert here.
      if (e_prev == NULL) { // insert as the first.
        LT_tbl = LT_tbl_entry_create(from, NULL);
        LT_tbl->next = e;
        return LT_tbl;
      } else { // insert in the middle.
        e_prev->next = LT_tbl_entry_create(from, NULL);
        e_prev->next->next = e;
        return e_prev->next;
      }
    }
    // else, go on to check the next entry.
  }

  return NULL;
}


/*
 * Find the cur_red_config in the LT_tbl_entry e.
 * If not found, then insert it.
 */
static llist_context_set * llist_context_set_get(LT_tbl_entry * e) {
  llist_context_set * c, * c_prev;
  if (NULL == e) {
    printf("llist_context_set_get error: e is NULL\n");
    exit(1);
  }

  if (e->ctxt_set == NULL) { // empty, insert as the first one.
    e->ctxt_set = llist_context_set_create(cur_red_config);
    return e->ctxt_set;
  }

  // else, try to find it in the list.
  for (c_prev = NULL, c = e->ctxt_set; c != NULL; 
       c_prev = c, c = c->next) {
    if (c->config == cur_red_config) return c; // found.
    if (c->config->ruleID > cur_red_config->ruleID) { // insert here
      if (c_prev == NULL) { // insert as the first
        e->ctxt_set = llist_context_set_create(cur_red_config);
        e->ctxt_set->next = c;
        return e->ctxt_set;
      } else { // insert in the middle.
        c_prev->next = llist_context_set_create(cur_red_config);
        c_prev->next->next = c;
        return c_prev->next;
      }
    }
    // else, go on to check the next.
  }
  // else, insert at the list tail.
  c_prev->next = llist_context_set_create(cur_red_config);
  return c_prev->next;
}


/*
 * Add the (from_state, config, context) 
 * information to an entry in the LT_tbl.
 * 
 * Note:
 * 1) The from_state is unique for each entry.
 * 2) The LT_tbl_entry_add function must have been called on the 
 *    from_state before calling this function, so "from" state
 *    always exists.
 *
 * The current config is "cur_red_config" defined at the top.
 */
static void LT_tbl_entry_addContext(State * from, SymbolList ctxt) {
  LT_tbl_entry * e;
  llist_context_set * c;

  if (ctxt == NULL) return;

  // 1) locate the LT_tbl_entry for "from" state.
  e = LT_tbl_entry_find_insert(from);
  if (NULL == e) {
    printf("LT_tbl_entry_addContext error: state %d NOT found\n", 
           from->state_no);
    exit(1);
  }

  // 2) locate the llist_context_set from the current config.
  c = llist_context_set_get(e);

  // 3) add/merge the context.
  llist_context_set_addContext(c, ctxt);
}


static void dump_LT_tbl_entry(LT_tbl_entry * e) {
  if (NULL == e) return;

  printf("%d \t| ", e->from_state);
  // dump_config_context.
  dump_llist_context_set(e->ctxt_set);
  printf("\t| ");
  llist_int_dump(e->to_states);
  puts("");
}


void dump_LT_tbl() {
  LT_tbl_entry * e;
  if (LT_tbl == NULL) return;
  
  printf("FROM \t| CONFIG:{CONTEXT} | TO\n");
  for (e = LT_tbl; e != NULL; e = e->next) {
    dump_LT_tbl_entry(e);
  }
}

/** END */


/************************************************************ 
 *  phase2_regeneration2 START. 
 */


/*
 * Return TRUE if a and b are disjoint, FALSE otherwise.
 * Similar to the function hasEmptyIntersection() in y.c.
 * This can be put into symbol_table.c.
 */
static BOOL symbolList_disjoint(SymbolList a, SymbolList b) {
  while (a != NULL && b != NULL) {
    if (a->snode == b->snode) return FALSE;
    if (strcmp(a->snode->symbol, b->snode->symbol) < 0) {
      a = a->next;
    } else {
      b = b->next;
    }
  }
  return TRUE;
}

/*
 * Check the given context sets are pair_wise disjoint.
 */
static BOOL pairwise_disjoint(llist_context_set * ctxt_set) {
  llist_context_set * a, * b;

  // if ctxt_set contains less than 2 nodes, return TRUE.
  if (ctxt_set == NULL || ctxt_set->next == NULL) return TRUE;

  for (a = ctxt_set; a->next != NULL; a = a->next) {
    for (b = a->next; b != NULL; b = b->next) {
      if (symbolList_disjoint(a->ctxt, b->ctxt) == FALSE) {
        return FALSE;
      }
    }
  }

  return TRUE;
}


/*
 * Create a new cluster and add it to the all_clusters set.
 * e - the start state/LT_tbl_entry of this cluster.
 *
 * By default, the states element's n1 and n2 are the same.
 */
static LT_cluster * cluster_create(LT_tbl_entry * e) {
  LT_cluster * c;
  HYY_NEW(c, LT_cluster, 1);
  
  // by default, n1 and n2 are the same.
  c->states = llist_int2_create(e->from_state, e->from_state);
  c->ctxt_set = llist_context_set_clone(e->ctxt_set);
  c->pairwise_disjoint = pairwise_disjoint(e->ctxt_set);
  c->next = NULL;

  if (c->pairwise_disjoint == FALSE) all_pairwise_disjoint = FALSE;

  return c;
}


/*
 * Not really neccesssary, but can be used to save running time space.
 */
static void cluster_destroy(LT_cluster * c) {
  llist_int2_destroy(c->states);
  llist_context_set_destroy(c->ctxt_set);
  free(c);
}


void cluster_dump(LT_cluster * c) {
  llist_int2 * n, * m;
  llist_int * s;
  LT_tbl_entry * e;

  printf("states: \n");
  for (n = c->states; n != NULL; n = n->next) {
    printf("%d/%d [to: ", n->n1, n->n2);

    if ((e = LT_tbl_findEntry(n->n1)) != NULL) {
      for (s = e->to_states; s != NULL; s = s->next) {
        m = llist_int2_find_n1(c->states, s->n);
        printf("%d/%d ", s->n, (m == NULL) ? -1 : m->n2);
      }
    }
  
    printf("]\n");
  }

  printf("context sets: ");
  dump_llist_context_set(c->ctxt_set);
  
  puts("");
}


static void all_clusters_dump() {
  LT_cluster * c;
  puts("--all_clusters.START--");
  for (c = all_clusters; c != NULL; c = c->next) {
    cluster_dump(c);
  }
  puts("--END--");
}


/*
 * Return:
 *   the splitted state's no if state_no is in c->states list
 *   -1 otherwise.
 *
 * Note state_no here is the virtual state_no: the one 
 * splitted from. So there could be more than one cluster
 * contain it.
 */
int cluster_contain_state(LT_cluster * c, int state_no) {
  llist_int2 * s;

  if (state_no < 0) return -1;
  //printf("cluster_cotain_state(state_no: %d)\n", state_no);
  if (c == NULL || c->states == NULL) return -1;

  for (s = c->states; s != NULL; s = s->next) {
    if (s->n1 == state_no) {
      //puts("found");
      return s->n2;
    }
  }
  //puts("NOT found");

  return -1;
}


/*
 * Return:
 *   the splitted state's no if state_no is in c->states list
 *   -1 otherwise.
 *
 * Note state_no here is the actual state_no.
 * There could be only one cluster contains it.
 */
int cluster_contain_actual_state(LT_cluster * c, int state_no) {
  llist_int2 * s;

  if (state_no < 0) return -1;
  if (c == NULL || c->states == NULL) return -1;

  for (s = c->states; s != NULL; s = s->next) {
    if (s->n2 == state_no) {
      return s->n1;
    }
  }

  return -1;
}


/*
 * Combine the two chains dst and src:
 *   if src contains a llist_context_set node whose config is NOT in dst,
 *     add it in INC order.
 *   if src contains a llist_context_set node whose config is in dst,
 *     combine the context.
 */
llist_context_set * llist_context_set_mergeChain(
    llist_context_set * dst, llist_context_set * src) {
  llist_context_set * a, * a_prev, * b;
  int cmp;

  if (src == NULL) return dst;
  if (dst == NULL) return llist_context_set_clone(src);

  b = src;
  for (a_prev = NULL, a = dst; a != NULL; a_prev = a, a = a->next) {
    while (b != NULL) {
      cmp = a->config->ruleID - b->config->ruleID;
      if (cmp == 0) { // same config, combine contexts.
        llist_context_set_addContext(a, b->ctxt);
        a_prev = a;
        a = a->next;
        b = b->next;
      } else if (cmp < 0) {
        a_prev = a;
        a = a->next;
      } else { // cmp > 0, insert b to dst before a.
        if (a_prev == NULL) { // add to the head
          a_prev = dst = llist_context_set_create(b->config);
          llist_context_set_addContext(dst, b->ctxt);
          dst->next = a;
        } else { // add to the middle.
          a_prev->next = llist_context_set_create(b->config);
          llist_context_set_addContext(a_prev->next, b->ctxt);
          a_prev->next->next = a;
        }
        b = b->next;
      }
      if (a == NULL) { break; }
    } // end of while(b != NULL).

    if (b == NULL || a == NULL) break;
  }

  if (b != NULL) { // clone b and its tail to the end of a_prev.
    a_prev->next = llist_context_set_clone(b);
  }

  return dst;
}


OriginatorList * cloneOriginatorList(OriginatorList * o) {
  int i, ct;
  OriginatorList * s;
  HYY_NEW(s, OriginatorList, 1);
  s->size = o->size;
  s->count = o->count;
  HYY_NEW(s->list, Configuration *, s->size);

  for (i = 0, ct = s->count; i < ct; i ++) {
    s->list[i] = o->list[i];
  }

  return s;
}


void copyConfig_LALR(Configuration * dst, Configuration * src) {
  if (dst == NULL || src == NULL) return;

  dst->ruleID = src->ruleID;
  dst->nMarker = src->nMarker;
  dst->marker = src->marker;
  copyContext(dst->context, src->context);
  dst->owner = src->owner;

  dst->isCoreConfig = src->isCoreConfig;
  dst->COMPLETE = src->COMPLETE;
  dst->IN_LANE = src->IN_LANE;
  dst->ORIGINATOR_CHANGED = src->ORIGINATOR_CHANGED;
  dst->LANE_END = src->LANE_END;
  dst->LANE_CON = src->LANE_CON;

  dst->originators = cloneOriginatorList(src->originators);
  dst->transitors = cloneOriginatorList(src->transitors);
}


State * cloneState(State * s) {
  int i, ct;
  State * t; // clone

  HYY_NEW(t, State, 1);
  
  t->next = s->next;
  t->config_max_count = s->config_max_count;
  HYY_NEW(t->config, Configuration *, t->config_max_count);
  t->config_count = s->config_count;
  t->core_config_count = s->core_config_count;

  for (i = 0, ct = t->config_count; i < ct; i ++) {
    t->config[i] = createConfig(-1, 0, 1);
    copyConfig_LALR(t->config[i], s->config[i]);
    t->config[i]->owner = t;
  }

  t->state_no = s->state_no;
  t->trans_symbol = createSymbolNode(s->trans_symbol->snode);

  t->successor_max_count = s->successor_max_count;
  HYY_NEW(t->successor_list, State *, t->successor_max_count);
  t->successor_count = s->successor_count;
  for (i = 0, ct = t->successor_count; i < ct; i ++) {
    t->successor_list[i] = s->successor_list[i];
  }

  t->parents_list = StateList_clone(s->parents_list);

  t->ON_LANE = s->ON_LANE;
  t->COMPLETE = s->COMPLETE;
  t->PASS_THRU = s->PASS_THRU;
  t->REGENERATED = s->REGENERATED;

  return t;
}


/*
 * In the successor list of src_state, replace s_old with s_new.
 */
void replaceSuccessor(State * src_state, 
                      State * s_new, State * s_old) {
  int i, ct;
  if (s_new == s_old) return;

  for (i = 0, ct = src_state->successor_count; i < ct; i ++) {
    if (src_state->successor_list[i] == s_old) {
      src_state->successor_list[i] = s_new;
      return;
    }
  }
}


/*
 * Add the LT_tbl_entry e->from_state to c.
 *
 * Return: the parent state_no: 
 *   if a splitted state is created, return it's state_no.
 *   else, return the old state_no.
 */
int cluster_add_LT_tbl_entry(LT_cluster * c, int from_state,
    llist_context_set * e_ctxt,
    int e_parent_state_no, BOOL copy) {
  State * s_parent, * s, * s_copy;
  int state_no = from_state;

  if (copy == TRUE) {
    // make a new state by copying e->from_state, 
    // add it to the end of states_new array, and add here.
    s_parent = states_new_array->state_list[e_parent_state_no];
    s = states_new_array->state_list[state_no];
    s_copy = cloneState(s);
    // insert a state to the parsing machine. Defined in y.c.
    insert_state_to_PM(s_copy);

    // Replace src_state's previous succcessor with s_copy.
    replaceSuccessor(s_parent, s_copy, s);

    state_no = s_copy->state_no;
  }

  // add state_no.
  // n1: original state, n2: splitted state.
  c->states = llist_int2_add_inc(c->states, from_state, state_no);
  // combine context sets.
  c->ctxt_set = llist_context_set_mergeChain(c->ctxt_set, e_ctxt); 

  return state_no;
}


/*
 * From all_clusters, find the one(s) that contains
 * the state (whose state number is state_no).
 *
 * Note there could be more than one such cluster if the 
 * given state was splitted before.
 *
 * This function is like a iterator. To use this function,
 * initialized c = NULL, then call in a loop:
 *   c = NULL;
 *   while ((c = find_containing_cluster(c, state_no) != NULL) {
 *     ...
 *   }
 */
LT_cluster * find_containing_cluster(LT_cluster * c, int state_no) {
  c = (NULL == c) ? all_clusters : c->next;

  for (; c != NULL; c = c->next) {
    if (cluster_contain_state(c, state_no) >= 0) {
      return c;
    }
  }
  return NULL;
}


/*
 * Different from find_containing_cluster in that:
 *
 * Find the cluster than contains the ACTUAL state_no.
 * There can be at most one such cluster.
 */
LT_cluster * find_actual_containing_cluster(int state_no) {
  LT_cluster * c;

  for (c = all_clusters; c != NULL; c = c->next) {
    if (cluster_contain_actual_state(c, state_no) >= 0) {
      return c;
    }
  }
  return NULL;
}


/*
 * In LT_tbl, find the entry where from_state is state_no.
 *
 * Note that in LT_tbl, the entries are in INC order of state_no.
 */
LT_tbl_entry * LT_tbl_findEntry(int from_state) {
  LT_tbl_entry * e;
  int cmp;

  for (e = LT_tbl; e != NULL; e = e->next) {
    cmp = e->from_state - from_state;
    if (cmp == 0) return e;
    if (cmp > 0) break;
  }

  return NULL; // not found.
}


/*
 * Combine new_part into old_part which is already in all_clusters list.
 *
 * Add each state in new_part to old_part, also merge context sets.
 */
void combine_cluster(LT_cluster * c_new, LT_cluster * c_old) {
  llist_int2 * a;
  // merge states.
  for (a = c_new->states; a != NULL; a = a->next) {
    c_old->states = llist_int2_add_inc(c_old->states, a->n1, a->n2);
  }
  // merge context sets.
  c_old->ctxt_set = llist_context_set_mergeChain(
    c_old->ctxt_set, c_new->ctxt_set);
}


/// functions for updating context in phase 2 regeneration. START.


/*
 * Use macro since it's faster.
 * Called by cluster_trace_new_chain() only.
 */
#define inherit_propagate(state_no, parent_state_no, container, e) { \
  State * s, * s_p;                                                  \
  s = states_new_array->state_list[state_no];                        \
  s_p = states_new_array->state_list[parent_state_no];               \
  if (TRUE == inheritParentContext(s, s_p)) {                        \
    getClosure(s); /* needed only if context changed.*/              \
    lt_phase2_propagateContextChange(state_no, container, e);        \
  }                                                                  \
  /*else { printf("::no propagation for state %d\n", state_no); }*/  \
}


#define clear_inherit_regenerate(state_no, parent_state_no) { \
  State * s, * s_p;                                           \
  s = states_new_array->state_list[state_no];                 \
  s_p = states_new_array->state_list[parent_state_no];        \
  clearStateContext(s);                                       \
  if (TRUE == inheritParentContext(s, s_p)) {                 \
    getClosure(s); /* needed only if context changed.*/       \
  }                                                           \
}


/*
 * Called by phase2_regeneration2() only.
 *
 * Note that if it is state 0, then should add $end
 * to the goal rule before getClosure() !
 */
#define clear_regenerate(state_no) {          \
  State * s;                                  \
  s = states_new_array->state_list[state_no]; \
  clearStateContext(s);                       \
  if (0 == state_no) {                        \
    hashTbl_insert(strEnd);                   \
    s->config[0]->context->nContext =         \
      createSymbolNode(hashTbl_find(strEnd)); \
    s->config[0]->context->context_count = 1; \
  }                                           \
  getClosure(s);                              \
}


/*
 * Function to propagate context change until a state where
 * there is no more context change.
 *
 * Here the children states are those defined in the
 * LT_tbl_entry's to_states list.
 */
void lt_phase2_propagateContextChange(
    int state_no, LT_cluster * c, LT_tbl_entry * e) {
  llist_int * t;
  LT_tbl_entry * f;
  llist_int2 * t2;

  if (NULL == e) { return; }

  // find all the to_states, do recursively until no context change.
  // in cluster c, state_no has a corresponding true state_no,
  // so are its to_states which can be found from e.

  for (t = e->to_states; t != NULL; t = t->next) {
    if ((f = LT_tbl_findEntry(t->n)) != NULL) {
      // need to replace t->n with the true state_no in cluster c.
      t2 = llist_int2_find_n1(c->states, t->n);
      if (t2 == NULL) {
        continue; // if not found, just ignore this to state.
      }
      inherit_propagate(t2->n2, state_no, c, f);
    }
  }
}


BOOL inheritParentContext(State * s, State * parent) {
  int i, ct;
  Configuration * c, * c_p;
  SymbolTblNode * trans_symbol;
  int config_index;
  BOOL isChanged = FALSE;

  if (s == NULL || parent == NULL) return FALSE;

  trans_symbol = s->trans_symbol->snode;
  ct = parent->config_count;
  for (i = 0; i < ct; i ++) {
    c_p = parent->config[i];
    if (isFinalConfiguration(c_p) == TRUE) continue;
    if (trans_symbol != getScannedSymbol(c_p)) continue;

    c_p->marker ++;
    c = findSimilarCoreConfig(s, c_p, & config_index);
    c_p->marker --;
    if (NULL == c) continue; // should NOT happen.

    if (TRUE == combineContext(c->context, c_p->context)) {
      isChanged = TRUE;
    }
  }

  return isChanged;
}


/*
 * Set a state's all configs' context to empty.
 */
void clearStateContext(State * s) {
  int i, ct;
  if (NULL == s) return;

  ct = s->config_count;
  for (i = 0; i < ct; i ++) {
    clearContext(s->config[i]->context); // defined in y.c.
  }
}


/// functions for updating context in phase 2 regeneration. END.


/*
 * Recursively add states to LT_cluster c along the chain in LT_tbl.
 *
 * Parameters:
 *  @ c - The new cluster,
 *  @ parent_state_no - the parent state no.
 *  @ state_no - the state no of the from_state LT_tbl.
 * Return:
 *  FALSE - combined with a cluster in all_clusters..
 *  TRUE - NOT combined with another cluster in all_clusters.
 */
BOOL cluster_trace_new_chain(int parent_state_no, int state_no) {
  LT_cluster * container, * first_container;
  LT_tbl_entry * e; // for the one with state_no.
  llist_context_set * e_ctxt;
  BOOL is_new_chain = TRUE;
  LT_cluster * c = new_cluster;
  BOOL container_not_found;
  BOOL is_pairwise_disjoint;
  int ret_state;

  //printf("cluster: %d. next state on chain: %d\n", c, state_no);

  // e will be used no matter what happen.
  // If e is NULL, then this is the end of chain.
  e = LT_tbl_findEntry(state_no);

  e_ctxt = (NULL == e) ? NULL : e->ctxt_set;

  // state in in cluster c.
  if ((ret_state = cluster_contain_state(c, state_no)) >= 0) {
    inherit_propagate(ret_state, parent_state_no, c, e);

    replaceSuccessor(
      states_new_array->state_list[parent_state_no], 
      states_new_array->state_list[ret_state], 
      states_new_array->state_list[state_no]);
    return TRUE; // NOTE here it returns TRUE.
  }

  // otherwise, state is NOT in c yet.

  // There could be more than one of this because of
  // copies made. Find the first one that has no conflict.
  // If all have conflict, use the first one.
  container_not_found = TRUE;
  first_container = container = NULL; 

  // note: this while loop can ignore the current cluster since
  // it has already been searched once above.
  while ((container = 
          find_containing_cluster(container, state_no)) != NULL) {
    if (first_container == NULL) first_container = container;

    // if c is NOT pairwise disjoint, then combination is always
    // in conflict, no need to search more, just break out.
    //if (c->pairwise_disjoint == FALSE) break;

    if (container->pairwise_disjoint == TRUE) { 
      llist_context_set * x = llist_context_set_clone(c->ctxt_set);
      x = llist_context_set_mergeChain(x, container->ctxt_set);
      is_pairwise_disjoint = pairwise_disjoint(x);
      llist_context_set_destroy(x);

      if (is_pairwise_disjoint == TRUE) { 
        combine_cluster(c, container); // container is the result.
        is_new_chain = FALSE; // not a new chain.
 
        // This is used so cluster_contain_state() at the beginning
        // of this function can return correct value once c is changed.
        c = new_cluster = container;

        inherit_propagate(state_no, parent_state_no, container, e);

        container_not_found = FALSE;
        break;
      }
    }
    // else, NOT pairwise disjoint, continue to find the next match.
  }  // end of while.


  if (container_not_found == TRUE) {

    if (first_container == NULL) { // always add.
      ret_state = cluster_add_LT_tbl_entry(
                  c, state_no, e_ctxt, parent_state_no, FALSE); 

      if (c->pairwise_disjoint == TRUE &&
          pairwise_disjoint(c->ctxt_set) == FALSE) {
        c->pairwise_disjoint = FALSE;
        all_pairwise_disjoint = FALSE;
      }
      if (NULL != e) e->processed = TRUE;

      clear_inherit_regenerate(state_no, parent_state_no);

    } else { 
      // e is already in another cluster, e.g., first_container;
      // but combined context are NOT pairwise-disjoint.
      // so make a copy e' of e and add it to c.
      ret_state = cluster_add_LT_tbl_entry(
                    c, state_no, e_ctxt, parent_state_no, TRUE);
      if ((c->pairwise_disjoint = pairwise_disjoint(c->ctxt_set))
          == FALSE) {
        all_pairwise_disjoint = FALSE;
      }
      clear_inherit_regenerate(ret_state, parent_state_no);

    }

    if (NULL != e) { // recursively trace the chain.
      is_new_chain = cluster_trace_new_chain_all(ret_state, e);
    }
  }

  return is_new_chain;
}


/*
 * parent_state: state_no of the parent state.
 */
BOOL cluster_trace_new_chain_all(int parent_state, LT_tbl_entry * e) {
  llist_int * s;
  BOOL is_new_chain = TRUE;

  // recursively trace the chain.
  for (s = e->to_states; s != NULL; s = s->next) {
    if (FALSE == cluster_trace_new_chain(parent_state, s->n)) {
      is_new_chain = FALSE;
    }
  }

  return is_new_chain;
}


/*
 * Add c to the all_clusters list.
 * c is always added to the tail of all_clusters.
 */
void all_clusters_add(LT_cluster * c) {
  if (all_clusters == NULL) {
    all_clusters_tail = all_clusters = c;
  } else { // add to the tail.
    all_clusters_tail->next = c;
    all_clusters_tail = all_clusters_tail->next;
  }
}


void phase2_regeneration2() {
  LT_tbl_entry * e, * x;
  BOOL is_new_chain = TRUE;

  all_clusters = all_clusters_tail = NULL; // initialize all_clusters.
  all_pairwise_disjoint = TRUE;

  for (e = LT_tbl; e != NULL; e = e->next) {
    if (e->processed == TRUE) continue;
  
    x = e; // start state of another chain/cluster of states.
    new_cluster = cluster_create(x);

    //printf("== chain head state: %d\n", e->from_state);
    clear_regenerate(x->from_state);

    x->processed = TRUE;

    is_new_chain = cluster_trace_new_chain_all(x->from_state, x);

    // add new_cluster to the all_clusters list.
    if (TRUE == is_new_chain) { all_clusters_add(new_cluster); }
  }

  //all_clusters_dump(); // new chain.

  // if the parsing machine is expanded, update the parsing table.
  if (states_new->state_count > ParsingTblRows) {
    updateParsingTable_LR0();
  }

  printf("LT-LTT end: states count: %d\n", states_new->state_count);
}

/************************************************************
 * phase2_regeneration2() END 
 */


/* for laneHeads list */

static laneHead * createLaneHead(State * s, SymbolTblNode * n) {
  laneHead * h;
  HYY_NEW(h, laneHead, 1);
  h->s = s;
  HYY_NEW(h->contexts, SymbolNode, 1);
  h->contexts->snode = n;
  h->contexts->next = NULL;
  h->next = NULL;
  return h;
}


static void destroyLaneHeadNode(laneHead * h) {
  if (NULL == h) return;

  freeSymbolNodeList(h->contexts);
  free(h);
}


/*
 * Add n to the contexts list of h in INC order.
 *
 * Called by addLaneHeadList() only, h and n are Not NULL.
 */
static void addContextToLaneHead(laneHead * h, SymbolTblNode * n) {
  SymbolNode * sn, * sn_prev, * tmp;
  for (sn_prev = NULL, sn = h->contexts; sn != NULL; 
       sn_prev = sn, sn = sn->next) {
    if (sn->snode == n) return; // already in list, don't add.
    if (strcmp(sn->snode->symbol, n->symbol) > 0) {
      // add n before sn, after sn_prev.
      break;
    }
  }

  HYY_NEW(tmp, SymbolNode, 1);
  tmp->snode = n;
  tmp->next = sn;
  if (sn_prev == NULL) { // add as the first node.
    h->contexts = tmp;
  } else { // add in the middle, after sn_prev, before sn.
    sn_prev->next = tmp;
  }
}


/*
 * Add another pair of s to h.
 * lh_list is in INC order of state's state_no;
 */
static laneHead * addLaneHeadList(laneHead * lh_list, State * s) {
  laneHead * h, * h_prev, * tmp;
  if (s == NULL) {
    YYERR_EXIT("lane_tracing.c addLaneHead error: s is NULL");
  }

  if (NULL == lh_list) {
    return createLaneHead(s, NULL);
  }
  // else, search if s is in list.
  for (h_prev = NULL, h = lh_list; h != NULL; h_prev = h, h = h->next) {
    if (h->s == s) { return lh_list; } // s exists already.
    if (h->s->state_no > s->state_no) break; // guarantee INC order.
  }

  // otherwise, s is NOT in list lh_list. Add another laneHead node.
  if (h == lh_list) { // insert as the first node.
    h = createLaneHead(s, NULL);
    h->next = lh_list;
    return h;
  } else { // insert in the middle after h_prev and before h.
    tmp = createLaneHead(s, NULL);
    h_prev->next = tmp;
    tmp->next = h;
    return lh_list;
  }
}


static void dumpLaneHeadList(laneHead * lh_list) {
  laneHead * h;

  puts("dumpLaneHeadList: ");
  for (h = lh_list; h != NULL; h = h->next) {
    printf("%d ", h->s->state_no);
    printf("\n");
  }
}


/* for originator list */

/*
 * Used when USE_LANE_TRACING == TRUE only.
 * Called in y.c function createConfig().
 */
OriginatorList * createOriginatorList() {
  OriginatorList * o;
  HYY_NEW(o, OriginatorList, 1);
  o->size = OriginatorList_Len_Init;
  o->count = 0;
  HYY_NEW(o->list, Configuration *, o->size);
  return o;
}


void expandOriginatorList(OriginatorList * o) {
  if (NULL == o || o->count < o->size) return;

  o->size *= 2;
  HYY_EXPAND(o->list, Configuration *, o->size);   
}


/*
 * Add originator to c's originator list if it does not exist yet.
 *
 * @parameter:
 *  cycle: 1 - called from combineConfigOriginators().
 *             Is a cycle, should insert to originator list.
 *         0 - called from getConfigSuccessors_LR0().
 *             NOT a cycle, should NOT insert to originator list.
 */
BOOL insertOriginatorList(
    Configuration * c, Configuration * originator, int cycle) {
  int i, ct;
  OriginatorList * o = c->originators;

  if (c == originator && cycle == 0) { return FALSE; }

  if (o->count == o->size) {
    expandOriginatorList(o);
  }

  for (i = 0, ct = o->count; i < ct; i ++) {
    if (originator == o->list[i]) return FALSE; // already exists.
  }

  o->list[o->count ++] = originator;
  c->ORIGINATOR_CHANGED = TRUE;
  return TRUE;
}


void writeOriginatorList(OriginatorList * o) {
  int i;
  Configuration * c;
  if (o == NULL) return;

  for (i = 0; i < o->count; i ++) {
    c = o->list[i];
    printf("%d.%d ", c->owner->state_no, c->ruleID);
  }
}

/* For transitor list */

/*
 * Add transitor to c's transitor list if it does not exist yet.
 */
static BOOL insertTransitorList(Configuration * c, Configuration * transitor) {
  int i, ct;
  OriginatorList * o = c->transitors;

  if (c == transitor) return FALSE;

  if (o->count == o->size) {
    expandOriginatorList(o);
  }

  for (i = 0, ct = o->count; i < ct; i ++) {
    if (transitor == o->list[i]) return FALSE; // already exists.
  }

  o->list[o->count ++] = transitor;
  return TRUE;
}


/* for inadequate states */

StateNoArray * createStateNoArray() {
  StateNoArray * sa;
  HYY_NEW(sa, StateNoArray, 1);
  sa->size = 2; // start value. 
  sa->count = 0;
  HYY_NEW(sa->states, int, sa->size);

  return sa;
}


static void expandStateNoArray(StateNoArray * sa) {
  sa->size *= 2;
  HYY_EXPAND(sa->states, int, sa->size);
}

/*
 * If state_no is not in the inadequate states list, add it.
 */
void addStateNoArray(StateNoArray * sa, int state_no) {
  int i, ct;
  if (sa->count == sa->size) { expandStateNoArray(sa); }

  ct = sa->count;
  for (i = 0; i < ct; i ++) {
    if (sa->states[i] == state_no) { return; } // exists.
  }
  sa->states[sa->count ++] = state_no;
}

void dumpStateNoArray(StateNoArray * sa) {
  int i, ct;
  if (sa == NULL) return;

  ct = sa->count;
  for (i = 0; i < ct; i ++) {
    printf("%d ", sa->states[i]);
  }
  puts("");
}


/* for debug use */
void my_writeSymbolNodeArray(SymbolNode * str) {
  SymbolNode * a;
  for (a = str; a != NULL; a = a->next) {
    if (a != str) printf(", ");
    printf("%s", a->snode->symbol);
  }
}

void my_writeContext(Context * c) {
  SymbolNode * s;

  printf(" {");

  if ((s = c->nContext) != NULL) {
    printf("%s", s->snode->symbol);
    while ((s = s->next) != NULL) {
      printf(", %s", s->snode->symbol);
    }
  }

  printf("} ");
}
static void my_writeProduction(Production * p, int marker) {
  int i;
  SymbolNode * n;

  if (p == NULL) {
    //printf("writeProduction warning: p is NULL\n");
    return;
  }

  printf("%s ", p->nLHS->snode->symbol);
  printf("-> ");

  i = 0;
  for (n = p->nRHS_head; n != NULL; n = n->next) {
    if (i == marker) printf(". ");
    printf("%s ", n->snode->symbol);
    i ++;
  }
  if (i == marker) printf(". ");

  // print this only when marker = -1.
  // i.e. called from writeGrammar().
  if (marker == -1 && p->isUnitProduction == TRUE)
    printf("(unit production)");

  if (marker == -1 && p->lastTerminal != NULL) 
    printf(" (Precedence Terminal: %s)", p->lastTerminal->symbol);

  // if write configration, then don't go to new line.
  // since the context has not been written.
  if (marker < 0) printf("\n");
}

void my_writeConfigOriginators(Configuration * c);

void stdout_writeConfig(Configuration * c) {
  printf("config (%d.%d) : ", c->owner->state_no, c->ruleID);
  if (c == NULL) {
    //printf("writeConfiguration warning: c is NULL\n");
    return;
  }
  my_writeProduction(grammar.rules[c->ruleID], c->marker);
  my_writeContext(c->context);
  ///if (c->isCoreConfig == 1) printf(" (core) ");
  printf("[COMPLETE: %d]", c->COMPLETE);
  printf("[IN_LANE: %d]", c->IN_LANE);
  printf("[LANE_END: %d]", c->LANE_END);
  printf("[LANE_CON: %d]\n", c->LANE_CON);
}
void my_writeState(State * s) {
  int i, ct;
  printf("state_no: %d (core: %d)\n", 
         s->state_no, s->core_config_count);

  ct = s->config_count;
  for (i = 0; i < ct; i ++) {
    stdout_writeConfig(s->config[i]);
  }
}


/*
 * For debug use.
 */
void writeConfigOriginators(Configuration * c) {
  int i, ct = c->originators->count;
  Configuration * o;
  if (ct == 0) return;

  yyprintf3("      %d originator%s: \n", ct, (ct > 1)?"s":"");
  for (i = 0; i < ct; i ++) {
    o = c->originators->list[i];
    yyprintf4("      originator (%d.%d.%d) \n", 
              o->owner->state_no, o->ruleID, o->marker);
  }
}


void writeConfigTransitors(Configuration * c) {
  int i, ct = c->transitors->count;
  Configuration * o;
  if (ct == 0) return;

  yyprintf3("      %d transitor%s: \n", ct, (ct > 1)?"s":"");
  for (i = 0; i < ct; i ++) {
    o = c->transitors->list[i];
    yyprintf4("      transitor (%d.%d.%d) \n",
              o->owner->state_no, o->ruleID, o->marker);
  }
}


void my_writeConfigOriginators(Configuration * c) {
  int i, ct = c->originators->count;
  Configuration * o;
  printf("config has %d originators: \n", ct);
  for (i = 0; i < ct; i ++) {
    o = c->originators->list[i];
    printf("      originator (%d.%d) \n",
              o->owner->state_no, o->ruleID);
  }
}


/*
 * get context for reduce productions in conflict states.
 */
static void getInadequateStateReduceConfigContext(State * s) {
  int i, ct;
  Configuration * c;
  ct = s->config_count;

  for (i = 0; i < ct; i ++) {
    c = s->config[i];
    if (isFinalConfiguration(c) == TRUE) {
      lane_tracing_reduction(c);
    } 
  }
}


static lane_tracing_phase1() {
  int i, ct, state_no;
  State * s;

  ct = states_inadequate->count;

  for (i = 0; i < ct; i ++) {
    state_no = states_inadequate->states[i];
    s = states_new_array->state_list[state_no];
    getInadequateStateReduceConfigContext(s);
  }

  // next will check resolved conflicts.
}


/*
 * called by update_action_table().
 */
static int findSuccessorStateNo(int state_no, SymbolTblNode * snode) {
  int i;
  State * successor;
  State * state = states_new_array->state_list[state_no];

  for (i = state->successor_count - 1; i >= 0; i --) {
    successor = state->successor_list[i];
    if (snode == successor->trans_symbol->snode) {
      return successor->state_no;
    }
  }

  // this should NEVER happen.
  printf("findSuccessorStateNo(%d, %s): ERROR\n",
         state_no, snode->symbol);
  return 0;
}


/*
 * Clear a parsing table row where lookahead is terminal.
 */
#define clearStateTerminalTransitions(state_no) {             \
  memset((void *) (ParsingTable + state_no * ParsingTblCols), \
          0, (grammar.terminal_count + 1) * 4);               \
}


/*
 * clear all the conflicts from states_new_array->conflict_list.
 */
static void clearStateConflicts(int state_no) {
  Conflict * tmp;
  Conflict * list = states_new_array->conflict_list[state_no];
 
  while (list != NULL) {
    tmp = list->next;
    if (list->s > 0) {
      states_new_array->rs_count[state_no] --;
      rs_count --;
    } else {
      states_new_array->rr_count[state_no] --;
      rr_count --;
    }
    destroyConflictNode(list);
    list = tmp;
  }

  states_new_array->conflict_list[state_no] = NULL;
}


/*
 * Algorithm:
 * resolve_conflict_2(State S) {
 *  clear the parsing table row for S where lookahead is terminal.
 *  clear all the conflicts associated with S.
 *
 *  foreach config c in S which is an x-transition or reduce {
 *    insertAction(c) to parsing table. Any conflicts will be recorded.
 *  }
 * }
 */
static void resolve_LALR1_conflicts() {
  int i, j, ct, state_no;
  State * state;
  Configuration * config;
  SymbolNode * contxt;
  Conflict * c;

  states_inadequate->count_unresolved = states_inadequate->count;

  for (i = 0, ct = states_inadequate->count; i < ct; i ++) {
    state_no = states_inadequate->states[i];
    if (state_no < 0) continue; // should never happen.

    state = states_new_array->state_list[state_no];

    //clear the parsing table row for S where lookahead is terminal.
    clearStateTerminalTransitions(state_no);

    //clear all the conflicts associated with S.
    clearStateConflicts(state_no);

    // re-insert actions into parsing table for this state.
    for (j = state->config_count - 1; j >= 0; j --) {
      config = state->config[j];
      if (isFinalConfiguration(config) == TRUE 
          && config->context != NULL) {
        // insert reduce
        for (contxt = config->context->nContext; contxt != NULL;
             contxt = contxt->next) {
          insertAction(contxt->snode, state_no, (-1) * config->ruleID);
        }
      } else if (config->nMarker != NULL &&
                 isTerminal(config->nMarker->snode) == TRUE) { 
          // insert shift.
          insertAction(config->nMarker->snode, state_no, 
            findSuccessorStateNo(state_no, config->nMarker->snode));
      }
    }

    if ((c = states_new_array->conflict_list[state_no]) == NULL) {
      //printf("conflict at state %d is solved\n", state_no);
      states_inadequate->states[i] = -1;
      states_inadequate->count_unresolved --;
    }

  }

  //printParsingTable();
}


void writeConflictingContext(int state_no) {
  Conflict * c;
  c = states_new_array->conflict_list[state_no];
  if (c == NULL) return;

  printf("conflicting contexts: %s", c->lookahead->symbol);

  for (c = c->next; c != NULL; c = c->next) {
    printf(", %s", c->lookahead->symbol);
  }
}


static laneHead * removePassThroughStates(laneHead * lh_list) {
  laneHead * h, * h_prev, * tmp;
  for (h_prev = NULL, h = lh_list; h != NULL; ) {
    if (h->s->PASS_THRU == 1) {
      // now remove this state from lh_list.
      tmp = h;
      if (h_prev == NULL) { // first node
        lh_list = h->next;
        h = lh_list;
      } else { // node in the middle of list.
        h_prev->next = tmp->next;
        h = tmp->next;
      }
      destroyLaneHeadNode(tmp);
    }
    else {
      h_prev = h;
      h = h->next;
    }
  }

  return lh_list;
}


static void GPM(State * new_state) {
  if (DEBUG_GEN_PARSING_MACHINE == TRUE) {
    printf("\n\n--generate parsing machine--\n");
    printf("states_new count: %d\n", states_new->state_count);
  }

  while (new_state != NULL) {
    if (DEBUG_GEN_PARSING_MACHINE == TRUE) {
      yyprintf3("%d states, current state is %d\n", 
               states_new->state_count, new_state->state_no);
    }

    getClosure(new_state); // get closure of this state.

    // get successor states and add them to states_new.
    transition(new_state);

    new_state = new_state->next; // point to next unprocessed state.
  }

  ParsingTblRows = states_new->state_count;
  n_state_opt1 = states_new->state_count;

  printf("LT-PGM end: states count: %d\n", states_new->state_count);
}


static void writeStateNoArray(StateNoArray * a, char * name) {
  int i, ct;
  if (name != NULL) { printf("%s: ", name); }
  for (i = 0, ct = a->count; i < ct; i ++) {
    printf("%d ", a->states[i]);
  }
  puts("");
}


/*
 * Adapted from transition() in y.c.
 */
static State_collection * getStateSuccessors(State * s) {
  int i;
  Configuration * c, * new_config;
  SymbolTblNode * scanned_symbol = NULL;
  State_collection * coll = createStateCollection();
  State * new_state = NULL;

  for (i = 0; i < s->config_count; i ++) {
    c = s->config[i];
    if (isFinalConfiguration(c)) {
      // do nothing.
    } else { // do transit operation.
      scanned_symbol = getScannedSymbol(c);
      if (strlen(scanned_symbol->symbol) == 0) { // empty reduction. 
        continue;
      }
      new_state =
        findStateForScannedSymbol(coll, scanned_symbol);
      if (new_state == NULL) {
        new_state = createState();
        // record which symbol this state is a successor by.
        new_state->trans_symbol =
          createSymbolNode(scanned_symbol);
        addState2Collection(coll, new_state);
      }
      // create a new core config for new_state.
      new_config = createConfig(-1, 0, 1);

      new_config->owner = new_state;
      copyConfig(new_config, c);
      new_config->isCoreConfig = 1;
      new_config->marker ++;
      if (new_config->nMarker != NULL)
        new_config->nMarker = new_config->nMarker->next;

      addCoreConfig2State(new_state, new_config);
    }
  } // end for

  return coll;
}


/*
 * Given the trans_symbol, find the index of the 
 * successor of s that has this trans_symbol.
 */
static int getSuccessorIndex(
    State * s, SymbolTblNode * trans_symbol) {
  int i, len;
  for (i = 0, len = s->successor_count; i < len; i ++) {
    if (s->successor_list[i]->trans_symbol->snode == trans_symbol) {
      return i; //s->successor_list[i];
    }
  }
  // the following code should never be reached.
  puts("lane_tracing.c getCorrespondingSuccessor() error:");
  printf(" NULL found on state %d, trans_symbol %s\n",
         s->state_no, trans_symbol->symbol);
  return -1; // this should NEVER happen
}


/*
 * Update S's successor Y0 to be Y:
 *   add Y as a new state, 
 *   update the parsing table, and the successor link of S
 *   from Y0 to Y.
 * @Input: S is the source state of Y.
 * @Return - TRUE if a new state is really added.
 *          FALSE if an existing state is found.
 */
static BOOL addSplitState(State * Y, State * S, int successor_index) {
  State * os;
  int is_compatible;

    os = searchStateHashTbl(Y, & is_compatible); // same or compatible.

  if (os == NULL) { // No existing state found. Add Y as a new state.
    Y->state_no = states_new->state_count;

    //printf("split - add new state %d\n", Y->state_no);

    addState2Collection(states_new, Y);
    addStateToStateArray(states_new_array, Y);
    // update shift action.
    updateAction(getCol(Y->trans_symbol->snode), S->state_no, Y->state_no);
    // update the Y0 successor link of S to Y.
    S->successor_list[successor_index] = Y;
    if (states_new->state_count >= PARSING_TABLE_SIZE) {
       expandParsingTable();
    }
    return TRUE;
  } else { // same or compatible with an existing state.
    updateAction(getCol(os->trans_symbol->snode), S->state_no, os->state_no);
    S->successor_list[successor_index] = os;
    destroyState(Y);
    return FALSE;
  }
}


static laneHead * addUniqueQueue(State * s, laneHead * lh_list) {
  laneHead * h;
  if ((h = lh_list) == NULL) return createLaneHead(s, NULL);

  if (h->s == s) { // exists, is the first node.
    //state s already on laneHead queue.
    return lh_list;
  }

  for (; h->next != NULL; h = h->next) {
    if (h->next->s == s) { // exists already.
      //state s already on laneHead queue
      return lh_list;
    }
  }
  // now h->next is NULL. insert s to the end.
  h->next = createLaneHead(s, NULL);
  return lh_list;
}


/*
 * Replace the context of S by those of T.
 *
 * Assumption: S and T are as in the phase2_regeneration()
 * function: they contains the same number of configurations.
 * T will be destroyed, so its context lists can be moved to S.
 */
static void regenerateStateContext(State * S, State * T) {
  int i, ct;
  Context * c;

  if (T->core_config_count != S->core_config_count) {
    YYERR_EXIT("regenerate error: inequal config_count");
  }

  // clear the context of S.
  ct = S->config_count;
  for (i = 0; i < ct; i ++) { // -> if final config, remove p.t. entry.
    c = S->config[i]->context;
    freeSymbolNodeList(c->nContext);
    c->nContext = NULL;
    c->context_count = 0;
  }

  // copy the context from T to S.
  ct = T->core_config_count;
  for (i = 0; i < ct; i ++) {
    copyContext(S->config[i]->context, T->config[i]->context);
  }

}


/*
 * Combine the contexts from T to S. No propagation here.
 */
static BOOL combineStateContext(State * s_dest, State * s_src) {
  BOOL isChanged = FALSE;
  int i;

  for (i = 0; i < s_dest->core_config_count; i ++) {
    if (combineContext(s_dest->config[i]->context,
                       s_src->config[i]->context) == TRUE) { 
      isChanged = TRUE;
    }
  }

  return isChanged;
}


/*
 * update reduce actions in parsing table for S.
 * note that transition actions are handled by addSplitState()
 * or keep unchanged.
 */
static void updateStateReduceAction(State * S) {
  int i, ct;
  SymbolNode * lookahead;
  Configuration * c;
  int state_dest;
  char action;

  ct = S->config_count;
  for (i = 0; i < ct; i ++) {
    // update reduce action for final/empty production.
    c = S->config[i];
    if (isFinalConfiguration(c) == TRUE ||
        strlen(getScannedSymbol(c)->symbol) == 0) {
      //printf("updateStateRedAction: %d.%d\n", S->state_no, c->ruleID);
      lookahead = c->context->nContext; 
      for (; lookahead != NULL; lookahead = lookahead->next) {

        getAction(lookahead->snode->type, getCol(lookahead->snode), 
                  S->state_no, & action, & state_dest);
        if (state_dest != c->ruleID) {
          if (action == 0 || action == 'r') {
            updateAction(getCol(lookahead->snode), 
                         S->state_no, (-1) * c->ruleID);
          } 
          else { // else, is "s" or "acc".
            insertAction(lookahead->snode, S->state_no, (-1) * c->ruleID);
          }
        } // end of if
      } // end of for
    } // end of if
  } // end of for
}


/*
 * Phase 2 w/ PGM
 */
static void phase2_regeneration(laneHead * lh_list) {
  laneHead * h;
  int successor_index;
  State * S, * Y, * Y0;
  State_collection * coll;
  State * new_state = NULL;
  BOOL exists;

  // 1) handle the head states and PASS_THRU states.
  for (h = lh_list; h != NULL; h = h->next) {
    S = h->s;
    getClosure(S); // getClosure() is defined in y.c
    updateStateReduceAction(S);

    coll = getStateSuccessors(S);

    for (Y = coll->states_head; Y != NULL; Y = Y->next) {
      //getClosure(Y); //my_writeState(Y);

      successor_index = getSuccessorIndex(S, Y->trans_symbol->snode);
      if (successor_index == -1) continue; // should NEVER happen.
      Y0 = S->successor_list[successor_index];

      if (Y0->PASS_THRU == 0) { // Y0 is NOT on lane.
        continue;  // NOT on 'conflicting' lane.
      }

      if (Y0->REGENERATED == 0) { // is original. Replace old state.
        // replace the context of Y0 with those of Y.
        regenerateStateContext(Y0, Y);
        Y0->REGENERATED = 1;
        lh_list = addUniqueQueue(Y0, lh_list);
      } else { // is regenerated state.
        exists = isCompatibleStates(Y0, Y);

        if (exists == TRUE) { // combine to compatible state Y0.
          combineStateContext(Y0, Y);
          lh_list = addUniqueQueue(Y0, lh_list);
        } else {
          if (addSplitState(Y, S, successor_index) == TRUE) {
            if (new_state == NULL) { new_state = Y; } // split.
          }
        }
      }
    } // end of for.
  } // end of for.

  // 2) handle the new added states.
  //    If there are any new split states, do GPM on them.
  if (new_state != NULL) { 
    GPM(new_state); 
  }
}


static void writeTheSymbolList(SymbolList a) {
  SymbolNode * b = a;
  printf("{");
  if (b != NULL) {
    printf("%s", b->snode->symbol);
    for (b = b->next; b != NULL; b = b->next) {
      printf(", %s", b->snode->symbol);
    }
  }
  else printf("EMPTY");
  printf("}");
}


static SymbolNode * getTheContext(Configuration * o) {
  SymbolNode * gamma, * scanned_symbol, * gamma_theads;
  int exist;
  if (o == NULL) return NULL;

  scanned_symbol = o->nMarker;
  if (scanned_symbol == NULL) return NULL; 

  gamma = scanned_symbol->next;
  gamma_theads = getTHeads(gamma); // Note NULL is a valid terminal.

  // note that "" will be the first node in the INC list,
  // so it's not so inefficient.
  gamma_theads = removeFromSymbolList(
                   gamma_theads, hashTbl_find(""), & exist);

  LT_tbl_entry_addContext(o->owner, gamma_theads);  // add context.

  return gamma_theads;
}


/*
 * Recursively trace back the lane along originators until 
 * a config labeld as LANE_END is found.
 *
 * Note that the originator does NOT include those transition
 * config, so only shift config are included. This guarantees 
 * that we won't end at those intermediate config that are
 * labeled as "LANE_END" by other lanes, since such intermediate
 * config are always transition configs and are not included as
 * originator by this lane.
 *
 * c is the originator of c0.
 */
laneHead * trace_back(
    Configuration * c0, Configuration * c, laneHead * lh_list) {
  Configuration * o;
  int i, len;

  c->LANE_CON = 1; // set as config on conflicting lane.

  if (c->LANE_END == 1) {
    lh_list = addLaneHeadList(lh_list, c->owner);
    return lh_list;
  }

  if (c->originators == NULL) {
    puts("trace_back: c->originators is NULL. error? report bug");
    return lh_list; // should NEVER happen.
  }

  len = c->originators->count;
  for (i = 0; i < len; i ++) {
    o = c->originators->list[i];

    set_transitors_pass_thru_on(c, o); // set PASS_THRU ON.
    if (o->LANE_CON == 0) {
      if (c->owner != o->owner) {
        c->owner->PASS_THRU = 1;
      }

      lh_list = trace_back(c, o, lh_list);
    } else {
      // already on lane. Do nothing.
    }
  }

  return lh_list;
}


/*
 * Get those states from which conflicting lanes start from, and 
 * the associated conflicting contexts for those states.
 *
 * Do this by tracing back each final config from this state.
 */
static laneHead * getStateConflictLaneHead(
    int state_no, laneHead * lh_list) {
  State * s;
  Configuration * con;
  int i, len;

  s = states_new_array->state_list[state_no];
  len = s->config_count;
  for (i = 0; i < len; i ++) {
    con = s->config[i];
    if (isFinalConfiguration(con) == TRUE) {
      cur_red_config = con;
      lh_list = trace_back(NULL, con, lh_list);
    }
  }

  return lh_list;
}


/*
 * Get those states from which conflicting lanes start from,
 * and their associated conflicting contexts.
 */
static laneHead * getConflictLaneHead() {
  int i, state_no;
  laneHead * laneHeadList = NULL;

  for (i = 0; i < states_inadequate->count; i ++) {
    if ((state_no = states_inadequate->states[i]) >= 0) {
      if (states_new_array->rr_count[state_no] > 0) {
        laneHeadList = getStateConflictLaneHead(state_no, laneHeadList);
      }
    }
  }

  laneHeadList = removePassThroughStates(laneHeadList);
  //dumpLaneHeadList(laneHeadList);

  return laneHeadList;
}


static void lane_tracing_phase2() {
  int ct;
  laneHead * laneHeadList;

  LT_tbl = NULL; // initialize the LT_tbl.

  ct = states_inadequate->count_unresolved;

  laneHeadList = getConflictLaneHead();
  if (laneHeadList == NULL) {
    puts("laneHeadList is NULL. return");
    return;
  }

  if (FALSE == USE_COMBINE_COMPATIBLE_STATES) {
    phase2_regeneration2(); // using all_clusters.
  } else {
    phase2_regeneration(laneHeadList);
  }
}


void lane_tracing() {
  IN_EDGE_PUSHING_LANE_TRACING = FALSE;

  lane_tracing_phase1();
  resolve_LALR1_conflicts(); 

  if (USE_LANE_TRACING == TRUE &&
      states_inadequate->count_unresolved > 0) {
    lane_tracing_phase2();
    resolve_LALR1_conflicts(); 
    outputParsingTable_LALR(); 

  } else {
    outputParsingTable_LALR(); 
  }

  if (GRAMMAR_AMBIGUOUS == TRUE) {
    puts("Grammar is ambiguous");
  }
}


/////////////////////////////////////////////////////////////

/* Functions for lane tracing */


static void dump_stacks() {
  puts("__STACK__"); stack_dump(STACK);
  puts("__LANE__ "); stack_dump(LANE);
}


/*
 * Does gamma have a non-null terminal descendent?
 * Input: n - gamma_theads.
 *
 * A null terminal is "", which is an empty string.
 */
BOOL testA(SymbolNode * n) {
  for (; n != NULL; n = n->next) {
    if (strlen(n->snode->symbol) != 0) return TRUE; 
  }
  return FALSE;
}


/*
 * Is the COMPLETE flag for c on?
 */
#define testB(c) (c->COMPLETE == FLAG_ON)


/*
 * Is the IN_LANE flag for c on?
 */
#define testC(c) (c->IN_LANE == FLAG_ON)

/*
 * Is the IN_LANE flag for c on?
 * Actually is the same as testC.
 */
#define testD(c) (c->IN_LANE == FLAG_ON)


/*
 * Insert snode to the list, no repetition allowed, increasing order.
 * Do it like insertion sort.
 *
 * @parameters:
 *  exist - label whether snode already existed.
 */
static SymbolNode * insertSymbolList_unique(
                    SymbolList list, SymbolTblNode * snode, int * exist) {
  SymbolNode * n, * n_prev, * new_node;
  * exist = 0;

  if (list == NULL) return createSymbolNode(snode);

  for (n = list, n_prev = NULL; n != NULL; n_prev = n, n = n->next) {
    if (n->snode == snode) {
      * exist = 1;
      return list; // existing node.
    }
    if (strcmp(n->snode->symbol, snode->symbol) > 0) {
      new_node = createSymbolNode(snode);
      // insert new_snode before n.

      if (n_prev == NULL) {
        new_node->next = list;
        return new_node;
      } else {
        new_node->next = n;
        n_prev->next = new_node;
        return list;
      }
    }
  } // end of for.

  // insert as the last node.
  n_prev->next = createSymbolNode(snode);
  return list;
}


/*
 * Assumption: list != NULL, c != NULL.
 */
SymbolNode * combineContextList(
                    SymbolList list, SymbolList new_list) {
  int exist;

  if (new_list == NULL) return list;
  for (; new_list != NULL; new_list = new_list->next) {
    list = insertSymbolList_unique(list, new_list->snode, & exist);
  }
  return list;
}
   


/*
 * Copy from list to a new symbol list.
 * NOT including nodes that contain empty string.
 */
SymbolList getCONTEXTS_GENERATED(SymbolList list, 
                                 int * null_possible) {
  int exist;
  SymbolNode * sn = NULL;
  * null_possible = 0;

  for (; list != NULL; list = list->next) {
    if (strlen(list->snode->symbol) == 0) {
      * null_possible = 1;
    } else {
      sn = insertSymbolList_unique(sn, list->snode, & exist);
    }
  }
  return sn;
}


static void stack_operation(int * fail_ct, Configuration * o) {
  Configuration * tmp;

  (* fail_ct) ++;

  switch (* fail_ct) {
    case 1:
            stack_push(LANE, o);
            o->IN_LANE = FLAG_ON;
            TEST_FAILED = FLAG_ON;
            break;
    case 2:
            tmp = stack_pop(LANE);
            stack_push(LANE, LT_MARKER);
            stack_push(LANE, tmp);

            stack_push(STACK, LT_MARKER);
            stack_push(STACK, o);
            break;
    default: // fail_ct >= 3
            stack_push(STACK, o);
            break;
  }

#if DEBUG_PHASE_1
  dump_stacks();
#endif

}


static void MOVE_MARKERS(Configuration * o) {
  Configuration * c;
  int ct;
  int r = 0;

  ct = stack_count(LANE) - 1;
  
  for (; ct >= 0; ct --) {
    c = LANE->array[ct];
    if (c == o) break;

    if (c == LT_MARKER) {
      LANE->array[ct] = LT_ZERO;
      r ++;
    }
  }

  if (TEST_FAILED == FLAG_OFF) {
    for (; r > 0; r --) stack_push(LANE, LT_MARKER);
  } else {
    c = stack_pop(LANE);
    for (; r > 0; r --) stack_push(LANE, LT_MARKER);
    stack_push(LANE, c);
  }
}


/*
 * For each symbol s in CONTEXT_GENERATED, do this:
 *   remove s from CONTEXT_GENERATED;
 *   for each config c in the LANE stack top-down
 *     if (s is in c) { break; }
 *     else { add s to c; }
 *
 * NOTE: here it accesses the LANE stack internal member
 *       array directly.
 */
static void CONTEXT_ADDING(
    SymbolList CONTEXT_GENERATED, int cur_config_index) {
  SymbolNode * tmp, * n; 
  int ct, exist;
  Configuration * c;

#if DEBUG_PHASE_1
  puts("CONTEXT ADDING ROUTINE: ");
  writeSymbolList(CONTEXT_GENERATED, "CONTEXT_GENERATED");
#endif

  n = CONTEXT_GENERATED;

  while (n != NULL) {
    for (ct = cur_config_index; ct >= 0; ct --) {
      c = LANE->array[ct];
      if (c != LT_ZERO && c != LT_MARKER) {
#if DEBUG_PHASE_1
  printf("add context to %d.%d\n", c->owner->state_no, c->ruleID);
#endif
        c->context->nContext = insertSymbolList_unique(
          c->context->nContext, n->snode, & exist);
        if (exist == 1) break;
        // else, NOT exist, insert was sucessful.
        c->context->context_count ++;
      }
    }   
    tmp = n;
    n = n->next;
    freeSymbolNode(tmp);
  }

}


static void CONTEXT_ADDING_ROUTINE(
    SymbolList CONTEXT_GENERATED, Configuration * o,
    int cur_config_index, int * fail_ct) {
  CONTEXT_ADDING(CONTEXT_GENERATED, cur_config_index);

  if (TRACE_FURTHER == FLAG_ON) {

#if DEBUG_PHASE_1
  puts("__TRACE_FURTHER is ON");
#endif

    TRACE_FURTHER = FLAG_OFF;
    stack_operation(fail_ct, o);   
  }

#if DEBUG_PHASE_1
  else { puts("__TRACE_FURTHER is OFF"); }
#endif

}



/*
 * Do lane_tracing Phase 1 on a configuration.
 * Pre-assumption: c is a reduction.
 */
void lane_tracing_reduction(Configuration * c) {
  if (NULL == c) return;

#if DEBUG_PHASE_1
  printf("work on reduce config: ");
  stdout_writeConfig(c);
#endif

  if (c->COMPLETE == 1) {
#if DEBUG_EdgePushing
    puts("c->COMPLETE == 1");
#endif
    return; // already evaluated.
  }

  LANE = stack_create();
  STACK = stack_create();

  stack_push(LANE, c); 
  c->IN_LANE = FLAG_ON;
  TRACE_FURTHER = FLAG_OFF;
  TEST_FAILED = FLAG_OFF;

  DO_LOOP();
}


/*
 * Print those configurations where a conflicting lane starts.
 * For debug use only.
 */
static void dumpLaneStartStates(
    Configuration * o, SymbolList gamma_theads) {
  if (NULL == gamma_theads) return;

  printf("START %d.%d: %d.%d generates contexts",
       LANE->array[0]->owner->state_no, LANE->array[0]->ruleID,
       o->owner->state_no, o->ruleID);
  //stdout_writeConfig(o);
  if (gamma_theads == NULL) {
    printf(":\n");
  } else {
    writeSymbolList(gamma_theads, "");
  }
}


void my_showTHeads(SymbolList alpha, SymbolList theads) {
  SymbolNode * a;

  printf("string '");

  a = alpha;
  for (a = alpha; a != NULL; a = a->next) {
    if (a != alpha) putchar(' ');
    printf("%s", a->snode->symbol);
  }

  printf("' has theads: ");

  for (a = theads; a != NULL; a = a->next) {
    if (a != theads) printf(", ");
    printf("%s", a->snode->symbol);
  }

  printf("\n");
}


static void writeConfig(Configuration * c) {
  if (NULL == c) printf("NULL ");
  else printf("%d.%d ", c->owner->state_no, c->ruleID);
}


/*
 * Returns TRUE is one of c's originators is o.
 */
static BOOL is_on_transitor_chain(Configuration * c, Configuration * o) {
  int i, ct;
  ct = c->originators->count;
  for (i = 0; i < ct; i ++) {
    if (c->originators->list[i] == o) return TRUE;
  }
  return FALSE;
}


/*
 * o - the originator. o does not change in the call stack.
 */
static void set_transitors_pass_thru_on(
    Configuration * cur_config, Configuration * o) {
  int i, ct;
  Configuration * c;

  // find the next transitor for originator o.
  ct = cur_config->transitors->count;

  for (i = 0; i < ct; i ++) {
    c = cur_config->transitors->list[i];

    get_originators(c, c); 

    if (is_on_transitor_chain(c, o) == TRUE) {

      if (FALSE == USE_COMBINE_COMPATIBLE_STATES) {
        // add another entry to LT_tbl.
        LT_tbl_entry_add(c->owner, cur_config->owner);
      }

  if (c->owner != cur_config->owner && c->owner != o->owner) {
    // use this criteria because: cur_config does not need to be
    // handled here since it's already handled in trace_back().
    // you also don't want to set o->owner to be PASS_THRU, since 
    // it may be the end state.
    c->owner->PASS_THRU = 1;
  }
      set_transitors_pass_thru_on(c, o);
    } // end is_on_transitor_chain.
  }

  if (o->owner == cur_config->owner) {
    getTheContext(o);
  }
}




/* Get transitor and originator. START. */


/*
 * let s = c->owner, find transitors for c in parent states of s.
 */
static void get_transitors(Configuration * c0, Configuration * c) {
  int i, j;
  Configuration * t; // transitor.
  State * p; // parent_state.
  StateList * L;

  L = c->owner->parents_list;
  if (L == NULL) {
    puts("Error: get_transitors() - L is NULL");
    return;
  }
  if (L->count == 0) return;

  for (i = 0; i < L->count; i ++) {
    p = L->state_list[i];
    // now get transitor for c.
    for (j = 0; j < p->config_count; j ++) {
      t = p->config[j];
      if (t->ruleID == c->ruleID && t->marker == c->marker - 1) {
        // is a transitor.
        insertTransitorList(c, t);
        get_originators(c0, t);
      }
    }
  }
}

/*
 * Get originators and transitors for c.
 *
 * @input: 
 *   - c0 : the original config to find originators for,
 *   - c  : the current config on path.
 * 
 * Transitor of c: a config d in parent state, 
 *   c.ruleID = d.ruleID, c.marker = d.marker + 1
 * Originator: a config d in current state,
 *   d.scanned_symbol = c.LHS_symbol
 */
static void get_originators(Configuration * c0, Configuration * c) {
  int i;
  Configuration * d;

  if (c->isCoreConfig == 1) { // core config, search parent states.
    get_transitors(c0, c);
  } else { // not core config. find originators in current state.
    for (i = 0; i < c->owner->config_count; i ++) {
       d = c->owner->config[i];
       if (c == d) { continue; } // ignore c.
       if (NULL == d->nMarker) { continue; }
       if (d->nMarker->snode == grammar.rules[c->ruleID]->nLHS->snode) {
         insertOriginatorList(c0, d, 1);
       }
    }
  }
}

/* Get transitor and originator. END. */


static void DO_LOOP() {
  int i, ct;
  Configuration * cur_config, * o;
  SymbolNode * gamma, * scanned_symbol, * gamma_theads;
  SymbolNode * CONTEXTS_GENERATED;
  int null_possible, fail_ct;
  int cur_config_index;

  cur_config = stack_top(LANE);
  cur_config_index = stack_count(LANE) - 1;

  if (cur_config == LT_MARKER || cur_config == LT_ZERO ||
      cur_config == NULL) { // should never happen.
    puts("DO_LOOP cur_config error");
    exit(1);
  }

  get_originators(cur_config, cur_config); 

  ct = cur_config->originators->count;
  fail_ct = 0;

#if DEBUG_PHASE_1
  printf("++++++++++TOP of LANE is: ");
  stdout_writeConfig(cur_config);
#endif


  for (i = 0; i < ct; i ++) { 

#if DEBUG_PHASE_1
  puts("________NEXT ORIGINATOR___________________");
#endif

    CONTEXTS_GENERATED = NULL;
    o = cur_config->originators->list[i];

    scanned_symbol = o->nMarker;
    if (scanned_symbol == NULL) gamma = NULL; // shouldn't happen
    else gamma = scanned_symbol->next;

#if DEBUG_PHASE_1
    stdout_writeConfig(o);
    printf("gamma: %s\n", gamma==NULL?"NULL":gamma->snode->symbol);
#endif

    gamma_theads = NULL;
    if (NULL != gamma) { // if not NULL, get theads.
#if DEBUG_PHASE_1
      puts("gamma not NULL, get theads.");
#endif
      gamma_theads = getTHeads(gamma); // get Heads.
#if DEBUG_PHASE_1
      my_showTHeads(gamma, gamma_theads);
#endif
    } 
    else { 
      // gamma is NULL, check if this is goal production.
      // NOTE that there are only TWO goal production in all states.
      // one is in state 0, the other is in the state where the
      // goal production is a final production. The second case
      // won't be traced in lane-tracing at all.
      //if (o->owner->state_no == 0 && o->ruleID == 0) { 
      if (IS_GOAL(o)) {
#if DEBUG_PHASE_1
        puts("GOAL PRODUCTION - generate context: $end");
#endif
        gamma_theads = createSymbolNode(hashTbl_find(strEnd));
      }
    }

#if DEBUG_PHASE_1
  dumpLaneStartStates(o, gamma_theads); 
#endif

    if (testA(gamma_theads) == TRUE) {
#if DEBUG_PHASE_1
      puts("testA true, get CONTEXTS_GENERATED");
#endif
      CONTEXTS_GENERATED = 
        getCONTEXTS_GENERATED(gamma_theads, & null_possible); 

if (IN_EDGE_PUSHING_LANE_TRACING == TRUE) { /// 12-19-2008.
   EDGE_PUSHING_CONTEXT_GENERATED = CONTEXTS_GENERATED;
}

      if (null_possible == 1) { 
#if DEBUG_PHASE_1
  puts("null possible TRUE");
#endif
        if (testB(o)) { // if (o->COMPLETE == FLAG_ON) {
#if DEBUG_PHASE_1
  puts("COMPLETE ON");
#endif

          CONTEXTS_GENERATED = combineContextList(
                    CONTEXTS_GENERATED, o->context->nContext); 
        } else { 
#if DEBUG_PHASE_1
  puts("COMPLETE OFF");
#endif

          if (testD(o)) { // if (o->IN_LANE == FLAG_ON) {
#if DEBUG_PHASE_1
  puts("IN_LANE ON");
  puts("GRAMMAR is AMBIGUOUS");
#endif
            GRAMMAR_AMBIGUOUS = TRUE;
            ///exit(1); //////////////// exit prematurely.
            MOVE_MARKERS(o);
          } else { 
#if DEBUG_PHASE_1
  puts("IN_LANE OFF. set TRACE_FURTHER ON");
#endif

            TRACE_FURTHER = FLAG_ON;
          }
        }
      }
      else { 
#if DEBUG_PHASE_1
  puts("possible null is: FALSE");
#endif
        o->LANE_END = 1; // set LANE_END to be true.

#if DEBUG_PHASE_1
        printf("Found lane_end: %d.%d\n", o->owner->state_no, o->ruleID);
        printf("conflict config: %d.%d, lane head state %d, contexts: ", 
          LANE->array[0]->owner->state_no, LANE->array[0]->ruleID, 
          o->owner->state_no);
        writeSymbolList(gamma_theads, "contexts");
#endif
        // if is in edge_pushing, ignore the context adding routine.
        if (IN_EDGE_PUSHING_LANE_TRACING == TRUE) { continue; }

      }
      // CONTEXT adding routine.
      CONTEXT_ADDING_ROUTINE(
        CONTEXTS_GENERATED, o, cur_config_index, & fail_ct);

    } else {
#if DEBUG_PHASE_1
  puts("testA false");
#endif

      if (testB(o)) { // testB
#if DEBUG_PHASE_1
  puts("testB TRUE");
#endif

        CONTEXTS_GENERATED = combineContextList(
                   CONTEXTS_GENERATED, o->context->nContext);
        CONTEXT_ADDING_ROUTINE(
          CONTEXTS_GENERATED, o, cur_config_index, & fail_ct);
      } else {
#if DEBUG_PHASE_1
  puts("testB false");
#endif

        if (testC(o)) { // testC
#if DEBUG_PHASE_1
  puts("testC TRUE");
#endif

          MOVE_MARKERS(o);

          CONTEXTS_GENERATED = combineContextList(
                     CONTEXTS_GENERATED, o->context->nContext);
          CONTEXT_ADDING_ROUTINE(
            CONTEXTS_GENERATED, o, cur_config_index, & fail_ct);
        } else {
#if DEBUG_PHASE_1
  puts("testC false");
#endif

          stack_operation(& fail_ct, o);
        }
      }
    } // end of else (testA false).

  } // end of for.

#if DEBUG_PHASE_1
  puts("________END OF DO_LOOP____________________");
#endif


  if (TEST_FAILED == FLAG_ON) {
#if DEBUG_PHASE_1
  puts("__TEST_FAILED is ON__");
#endif

    TEST_FAILED = FLAG_OFF;
    DO_LOOP();
  } else {
    CHECK_LANE_TOP();
  }
}


static void POP_LANE() {

#if DEBUG_PHASE_1
  puts("POP_LANE");
#endif

  stack_pop(LANE);
  CHECK_LANE_TOP();
}


static void CHECK_STACK_TOP() {
  Configuration * top = stack_top(STACK);

  if (top == LT_MARKER) {
#if DEBUG_PHASE_1
  puts("__check_stack_top TRUE");
#endif

    stack_pop(STACK);
    POP_LANE();

  } else {
#if DEBUG_PHASE_1
  puts("__check_stack_top FALSE");
#endif

    if (top->COMPLETE == FLAG_ON) {
#if DEBUG_PHASE_1
  puts("__top COMPLETE ON");
#endif

      stack_pop(STACK);
      CHECK_STACK_TOP();
    } else {
#if DEBUG_PHASE_1
  puts("__top COMPLETE OFF");
#endif

      stack_pop(STACK);
      stack_push(LANE, top);
      top->IN_LANE = FLAG_ON;
      DO_LOOP();
    }
  }
}


static void PROPOGATE_CONTEXT_SETS(Configuration * c) {
  State * s;
  Configuration * f;
  int i, ct;
  if (c == NULL || c->COMPLETE == 0 || c->isCoreConfig == 1) return;

#if DEBUG_PHASE_1
  printf("===config %d.%d heuristic propogate context sets===\n",
          c->owner->state_no, c->ruleID);
  stdout_writeConfig(c);
#endif

  s = c->owner;
  ct = s->config_count;
  for (i = 0; i < ct; i ++) {
    f = s->config[i];
    if (f == c) continue;
    if (grammar.rules[f->ruleID]->nLHS->snode != 
         grammar.rules[c->ruleID]->nLHS->snode) continue;
    if (f->marker > 0) continue;
    if (f->COMPLETE == 1) continue; // x-transition of conflict states.

    // otherwise, heuristically propagate context sets.
    //copyContext(f->context, c->context);
    freeContext(f->context);
    f->context = c->context;
    f->COMPLETE = 1;

    // propogate context origin.
    ///f->context->con = c->context->con;
  }

}



static void CHECK_LANE_TOP() {
  Configuration * lane_top = stack_top(LANE);

  if (lane_top == LT_MARKER) {
#if DEBUG_PHASE_1
  puts("check lane top TRUE");
#endif

    CHECK_STACK_TOP();
  } else {
#if DEBUG_PHASE_1
  puts("check lane top FALSE");
#endif

    if (lane_top == LT_ZERO) {
#if DEBUG_PHASE_1
  puts("lane top is ZERO.");
#endif

      POP_LANE();
    } else {
      lane_top->IN_LANE = FLAG_OFF;
      lane_top->COMPLETE = FLAG_ON;
      if (IN_EDGE_PUSHING_LANE_TRACING == FALSE) {
        PROPOGATE_CONTEXT_SETS(lane_top);
      }
      if (stack_count(LANE) == 1) { // the starting reduction
        /////////// END PROGRAMING ///////////////
#if DEBUG_PHASE_1
        puts("=====REDUCTION LANE TRACING ENDS=====");
#endif
      } else {
        POP_LANE();
      }
    }
  }
}




