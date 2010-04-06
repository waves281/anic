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

#ifndef _LANE_TRACING_H_
#define _LANE_TRACING_H_

#include "y.h"

/*
 * lane_tracing.h
 *
 * Used by lane_tracing.c only.
 *
 * @Author: Xin Chen
 * @Created on: 7/26/2008
 * @Last modified: 3/24/2009
 * @Copyright (C) 2008, 2009
 */



/*
 * A macro to determine if a config is the goal production
 * of state 0. Used for getting the "$end" context
 * before testA() in lane-tracing.
 * Single it out and put here, so it's easier to understand.
 */
#define IS_GOAL(o) (o->owner->state_no == 0 && o->ruleID == 0)


/** Data structure for the state-combination in phase 2 table. START */

typedef struct _llist_int llist_int; /* linked list of states */
struct _llist_int {
  int n;
  llist_int * next;
};

/*
 * Similar to llist_int, but has two int fields.
 * Used by LT_cluster only.
 */
typedef struct _llist_int2 llist_int2; /* linked list of states */
struct _llist_int2 {
  int n1;
  int n2;
  llist_int2 * next;
};

typedef struct _llist_context_set llist_context_set;
struct _llist_context_set {
  Configuration * config; // in INC order of config->ruleID.
  SymbolList ctxt; // in INC order of symbol.
  llist_context_set * next;
};


/*
 * to get a State pointer from the state_no, use
 * states_new_array->state_list[state_no].
 */
typedef struct _LT_tbl_entry LT_tbl_entry;
struct _LT_tbl_entry {
  BOOL processed; // whether this entry was processed during regeration.
  int from_state;
  llist_context_set * ctxt_set; // in INC order of config->ruleID.
  llist_int * to_states; // in INC order of state_no.
  LT_tbl_entry * next;
};


/*
 * For state combining purpose.
 */
typedef struct _LT_cluster LT_cluster;
struct _LT_cluster {
  BOOL pairwise_disjoint;
  llist_int2 * states; // in INC order of state_no.
  llist_context_set * ctxt_set; // in INC order of config->ruleID.
  LT_cluster * next;
};


extern LT_cluster * all_clusters;


/* Functions */

extern LT_tbl_entry * LT_tbl_entry_find(State * from);
extern LT_cluster * find_actual_containing_cluster(int state_no);
extern void cluster_dump(LT_cluster * c);
extern llist_int * llist_int_add_inc(llist_int * list, int n);
extern int cluster_contain_state(LT_cluster * c, int state_no);
extern llist_int2 * llist_int2_find_n2(llist_int2 * list, int n2);
extern void llist_int_dump(llist_int * list);


/*
 * For conflicting lanes' head states and associated conflicting contexts.
 */
typedef struct laneHeadState laneHead;
struct laneHeadState {
  State * s; // conflicting lane head state.
  SymbolList contexts; // list of conflicting contexts.
  laneHead * next;
};


/*
 * For (conflict_config, lane_end_config) pairs.
 */
typedef struct _ConfigPairNode ConfigPairNode;
struct _ConfigPairNode {
  Configuration * end; // conflict_config
  Configuration * start; // lane_start_config
  ConfigPairNode * next;
};
typedef ConfigPairNode * ConfigPairList;


/*
 * Functions in lane_tracing.c
 */
extern laneHead * trace_back(
    Configuration * c0, Configuration * c, laneHead * lh_list);


/*
 * Functions in lrk_util.c
 */
extern ConfigPairList ConfigPairList_combine(
         ConfigPairList t, ConfigPairList s);
extern ConfigPairList ConfigPairList_insert(ConfigPairList list,
    Configuration * conflict_config, Configuration * lane_start_config);
extern void ConfigPairList_dump(ConfigPairList list);
extern ConfigPairNode * ConfigPairList_find(
    ConfigPairList list, Configuration * conflict_config);
extern void ConfigPairList_destroy(ConfigPairList list);

/* Set - a linked list of objects. */
typedef struct _Object_item Object_item;
struct _Object_item {
  void * object;
  Object_item * next;
};
typedef Object_item Set;
extern Set * Set_insert(Set * set, void * object);
extern Object_item * Set_find(Set * set, void * object);
extern Set * Set_delete(Set * set, void * object);
extern void Set_dump(Set * set, void (* set_item_dump)(void *));


/* List - a single linked list. */
typedef struct {
  int count;
  Object_item * head;
  Object_item * tail;
} List; 
extern List * List_create();
extern void List_insert_tail(List * t, void * object);
extern void List_destroy(List * t);
extern void List_dump(List * t, void (* list_item_dump)(void *));

extern void print_symbolList(void * object);


/*
 * for (configuration, conflict context) pair
 */
typedef struct {
  Configuration * c;
  SymbolList ctxt;  // conflict context symbols
  Configuration * tail;
} cfg_ctxt;

extern cfg_ctxt * cfg_ctxt_create(
         Configuration * c, SymbolList s, Configuration * tail);
extern void cfg_ctxt_destroy(cfg_ctxt * cc);
extern void cfg_ctxt_dump(cfg_ctxt * cc);


#endif


