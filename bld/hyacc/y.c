/*
   This file is part of Hyacc, a LR(0)/LALR(1)/LR(1) parser generator.
   Copyright (C) 2007, 2008, 2009 Xin Chen. chenx@hawaii.edu

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

/****************************************************************
 * y.c
 *
 * Generates a LR(1) parsing machine given a grammar.
 *
 * @author: Xin Chen
 * @Date Started: 8/30/2006
 * @Last modified: 3/24/2009
 * Copyright (C) 2007, 2008, 2009
 ****************************************************************/

#include "y.h"
#include "lane_tracing.h"


/* Declaration of functions. */
extern void useGrammar(int grammar_number);
void dump_state_collections();
void destroyState(State * s);
State * createState();
int isFinalConfiguration(Configuration * c);
Configuration * createConfig(int ruleID, int marker, int isCoreConfig);
void copyConfig(Configuration * c_dest, Configuration * c_src);
void initParsingTable();
void initStartState();
void writeParsingTableColHeader();
BOOL combineContext(Context * c_dest, Context * c_src);
Production * createProduction(
       char * lhs, char * rhs[], int rhs_count);
void writeConfiguration(Configuration * c);
void combineCompatibleConfig(State * s);
BOOL combineCompatibleStates(State * s_dest, State * s_src);
void insertReductionToParsingTable(
       Configuration * c, int state_no);
void getReachableStates(int cur_state, int states_reachable[],
    int * states_count);
void writeUPSStates();
void printIntArray(int a[], int a_ct);
State_collection * createStateCollection();
void destroyStateCollection(State_collection * c);
void expandParsingTable();
void copyContext(Context * dest, Context * src);
void copyProduction(Production * dest, Production * src);
void freeProduction(Production * p);
void clearProduction(Production * p);
void clearContext(Context * c);
void freeContext(Context * c);
void freeConfig(Configuration * c);
SymbolTblNode * getScannedSymbol(Configuration * c);
BOOL isCompatibleConfig(Configuration * c1, Configuration * c2);
void insertAction(SymbolTblNode * lookahead, int row, int state_dest);
Conflict * addToConflictArray(int state, SymbolTblNode * lookahead,
                              int action1, int action2);
void writeStateTransitionList();
void writeSymbolNodeArray(SymbolNode * str);

char * hyacc_filename;


int getGrammarRuleCount() { return grammar.rule_count; }


/*
 * For StateList. Start.
 */

StateList * StateList_create() {
  StateList * L;
  HYY_NEW(L, StateList, 1);

  L->count = 0;
  L->size = 5; // assume 5 parents initially.
  HYY_NEW(L->state_list, State *, L->size);

  return L;
}

void StateList_destroy(StateList * L) {
  if (NULL == L || NULL == L->state_list) return;
  free(L->state_list);
  free(L);
}


void StateList_expand(StateList * L) {
  if (NULL == L) return;
  HYY_EXPAND(L->state_list, State *, L->size * 2);
  L->size *= 2;
}

/*
 * Add if not exist yet.
 * @Return: TRUE is added, FALSE if not added.
 */
BOOL StateList_add(StateList * L, State * s) {
  int i;
  if (L == NULL) {
    YYERR_EXIT("StateList_add error: L is NULL\n");
  }
  if (s == NULL) return FALSE;
  for (i = 0; i < L->count; i ++) {
    if (L->state_list[i]->state_no == s->state_no) return FALSE;
  }

  // now add at the end.
  if (L->count == L->size) {
    StateList_expand(L);
  }

  L->state_list[L->count ++] = s;

  return TRUE;
}

/*
 * Called by cloneState() in lane_tracing.c.
 */
StateList * StateList_clone(StateList * L0) {
  StateList * L;
  int i;

  if (NULL == L0) return NULL;

  HYY_NEW(L, StateList, 1);

  L->count = L0->count;
  L->size = L0->size; // assume 5 parents initially.
  HYY_NEW(L->state_list, State *, L->size);

  for (i = 0; i < L->count; i ++) {
    L->state_list[i] = L0->state_list[i];
  }

  return L;
}

void StateList_write(StateList * L) {
  int i;

  if (L == NULL) return;

  yyprintf2("\n  parents_list(%d): ", L->count);
  for (i = 0; i < L->count; i ++) {
    if (i > 0) yyprintf(", ");
    yyprintf2("%d", L->state_list[i]->state_no);
  }
  yyprintf("\n");
}

/*
 * For StateList. End.
 */


/*
 * marker: mark the position in configuration.
 * 0 <= marker <= RHS_count
 * (If marker = -1, don't print it.)
 *
 * In general, marker == -1 only when print grammar rules.
 * marker >= 0 when print configuration production.
 */
void writeProduction(Production * p, int marker) {
  int i;
  SymbolNode * n;

  if (p == NULL) {
    //printf("writeProduction warning: p is NULL\n");
    return;
  }

  yyprintf2("%s ", p->nLHS->snode->symbol);
  yyprintf("-> ");

  i = 0;
  for (n = p->nRHS_head; n != NULL; n = n->next) {
    if (i == marker) yyprintf(". ");
    yyprintf2("%s ", n->snode->symbol);
    i ++;
  }
  if (i == marker) yyprintf(". ");

  // print this only when marker = -1.
  // i.e. called from writeGrammar().
  if (marker == -1 && p->isUnitProduction == TRUE)
    yyprintf("(unit production)");

  if (marker == -1 && p->lastTerminal != NULL)
    yyprintf2(" (Precedence Terminal: %s)", p->lastTerminal->symbol);

  // if write configration, then don't go to new line.
  // since the context has not been written.
  if (marker < 0) yyprintf("\n");
}


/*
 * Write rules of the given grammar.
 */
void writeGrammarRules(Grammar * g) {
  int i;
  int count = 0;
  yyprintf("Rules: \n");
  for (i = 0; i < g->rule_count; i ++) {
    yyprintf2("(%d) ", i);
    writeProduction(g->rules[i], -1);
    count ++;
  }
  yyprintf2("Number of Rules: %d\n", count);
}


/*
 * Write rules of the given grammar which are not
 * unit productions.
 * The goal production is always printed no matter
 * it is a unit production or not.
 */
void writeGrammarRulesNoUnitProd(Grammar * g) {
  int i;
  int count = 0;
  yyprintf("Rules: \n");
  for (i = 0; i < g->rule_count; i ++) {
    if ((isUnitProduction(i) == FALSE) || i == 0) {
      yyprintf2("(%d) ", i);
      writeProduction(g->rules[i], -1);
      count ++;
    }
  }
  yyprintf2("Number of Rules: %d\n", count);
}


/*
 * Returns number of rules excluding unit productions.
 */
int getOptRuleCount(Grammar * g) {
  int i;
  int count = 0;
  for (i = 0; i < g->rule_count; i ++) {
    if ((isUnitProduction(i) == FALSE) || i == 0)
      count ++;
  }
  return count;
}


/*
 * Write terminals of the given grammar.
 */
void writeTerminals(Grammar * g) {
  SymbolNode * a;
  yyprintf2("Terminals (%d): \n", g->terminal_count);

  a = g->terminal_list;

  if (a != NULL) {
    yyprintf2("%s\n", a->snode->symbol);
    for (a = a->next; a != NULL; a = a->next) {
      yyprintf2("%s\n", a->snode->symbol);
    }
  }

  yyprintf("\n");
}


/*
 * Write non-terminals of the given grammar.
 *
 * Note: the part "if (ADD_GOAL_RULE == 1) ..."
 * is just to keep consistent with yacc.
 * Leave this out for now.
 */
void writeNonTerminals(Grammar * g) {
  SymbolNode * a;
  yyprintf2("Non-terminals (%d): \n", g->non_terminal_count);

  a = g->non_terminal_list;
  if (a != NULL) {
    yyprintf2("%s\n", a->snode->symbol);
    for (a = a->next; a != NULL; a = a->next) {
      yyprintf2("%s\n", a->snode->symbol);
    }
  }

  yyprintf("\n");
}


void writeVanishSymbols(Grammar * g) {
  SymbolNode * a;

  yyprintf2("Vanish symbols (%d): \n",
          g->vanish_symbol_count);

  if ((a = g->vanish_symbol_list) != NULL) {
    yyprintf2("%s\n", a->snode->symbol);

    for (a = g->vanish_symbol_list; a != NULL; a = a->next) {
      yyprintf2("%s\n", a->snode->symbol);
    }
  }

  yyprintf("\n");
}


/*
 * Write the given grammar, including its terminals,
 * non-terminals, goal symbol and rules.
 */
void writeGrammar(Grammar * g, BOOL beforeRmUnitProd) {
  yyprintf("\n--Grammar--\n");
  writeTerminals(g);
  writeNonTerminals(g);
  writeVanishSymbols(g);
  yyprintf2("Goal symbol: %s\n", g->goal_symbol->snode->symbol);

  if (beforeRmUnitProd == TRUE ||
      USE_REMOVE_UNIT_PRODUCTION == FALSE) {
    writeGrammarRules(g);
  } else { // after remove unit production.
    writeGrammarRulesNoUnitProd(g);
  }
  yyprintf("\n");
}


/*
 * Free variables dynamically allocated by the program.
 * Called by function main().
 */
void free_vars() {
  int i;
  // free dynamically allocated variables in grammar.
  for (i = 0; i < grammar.rule_count; i ++) {
    freeProduction(grammar.rules[i]);
  }

  // free dynamically allocated variables in states_new.
  destroyStateCollection(states_new);

  free(states_reachable);
  free(actual_state_no);
  free(ParsingTable);

  hashTbl_destroy();

  queue_destroy(config_queue);
}


State_array * createStateArray() {
  int i;
  State_array * s;
  HYY_NEW(s, State_array, 1);
  s->state_count = 0;
  s->size = PARSING_TABLE_INIT_SIZE;
  HYY_NEW(s->state_list, State *, s->size);

  HYY_NEW(s->conflict_list, Conflict *, s->size);
  for (i = 0; i < s->size; i ++) s->conflict_list[i] = NULL;

  HYY_NEW(s->rs_count, int, s->size);
  HYY_NEW(s->rr_count, int, s->size);
  memset((void *)s->rr_count, 0, 4 * s->size);
  memset((void *)s->rs_count, 0, 4 * s->size);

  return s;
}


/*
 * Called by expandParsingTable() only.
 */
void expandStateArray(State_array * a, int new_size) {
  int i;
  HYY_EXPAND(a->state_list, State *, new_size);

  // expand conflict array as well for the states.
  HYY_EXPAND(a->conflict_list, Conflict *, new_size);
  HYY_EXPAND(a->rs_count, int, new_size);
  HYY_EXPAND(a->rr_count, int, new_size);
  for (i = a->size; i < new_size; i ++) {
    a->conflict_list[i] = NULL;
  }

  memset((void *) (a->rs_count + a->size), 0, 4 * a->size);
  memset((void *) (a->rr_count + a->size), 0, 4 * a->size);

  a->size = new_size;
}


void addStateToStateArray(State_array * a, State * s) {
  a->state_list[a->state_count] = s;
  a->state_count ++;
}


Conflict * createConflictNode(int state, SymbolTblNode * lookahead,
                          int r, int s) {
  Conflict * c;
  HYY_NEW(c, Conflict, 1);
  c->state = state;
  c->lookahead = lookahead;
  c->r = r;
  c->s = s;
  c->next = NULL;
  return c;
}


void destroyConflictNode(Conflict * c) {
  free(c);
}


void destroyConflictList(Conflict * a) {
  Conflict * b;
  if (a == NULL) return;

  while (a != NULL) {
    b = a->next;
    free(a);
    a = b;
  }
}


void incConflictCount(int s, int state) {
  if (s > 0) {
    rs_count ++;
    states_new_array->rs_count[state] ++;
  } else {
    rr_count ++;
    states_new_array->rr_count[state] ++;
  }
}


/*
 * insert in incresing order by conflict's state no.
 *
 * Note: no need to check size of conflict arrays,
 * which is handled in expandParsingTable().
 */
Conflict * addToConflictArray(int state, SymbolTblNode * lookahead,
                              int action1, int action2) {
  int r, s; // r < s
  Conflict * c = NULL, * b, * b_prev = NULL;

  if (action1 < action2) { r = action1; s = action2; }
  else { r = action2; s = action1; }

  if (states_new_array->rr_count[state] == 0 &&
      states_new_array->rs_count[state] == 0) {
    c = createConflictNode(state, lookahead, r, s);
    states_new_array->conflict_list[state] = c;
    incConflictCount(s, state);
    return c;
  }

  for (b = states_new_array->conflict_list[state];
       b != NULL; b_prev = b, b = b->next) {
    if (state == b->state && lookahead == b->lookahead &&
        r == b->r && s == b->s) return NULL; // exits already.

    if (state < b->state) {
      c = createConflictNode(state, lookahead, r, s);

      if (b_prev == NULL) { // insert at the head.
        c->next = states_new_array->conflict_list[state];
        states_new_array->conflict_list[state] = c;
      } else {              // insert in the middle.
        c->next = b;
        b_prev->next = c;
      }
      incConflictCount(s, state);
      return c;
    }
  } // end of for.

  c = createConflictNode(state, lookahead, r, s);
  b_prev->next = c; // insert at the tail.
  incConflictCount(s, state);

  return c;
}


/*
 * Initialize variables when the program starts.
 * Called by function main().
 */
void init() {
  states_new = createStateCollection();
  states_new_array = createStateArray(); // size == PARSING_TABLE_SIZE

  if (USE_LALR == TRUE) {
    states_inadequate = createStateNoArray();
    OriginatorList_Len_Init = 2;
  }

  config_queue = queue_create(); // for getClosure().

  // for finding same/compatible states fast.
  initStateHashTbl();
  initStartState();
  initParsingTable();

  ss_count = rr_count = rs_count = 0;
}


void writeContext(Context * c) {
  SymbolNode * s;

  yyprintf(" {");

  if ((s = c->nContext) != NULL) {
    yyprintf2("%s", s->snode->symbol);
    while ((s = s->next) != NULL) {
      yyprintf2(", %s", s->snode->symbol);
    }
  }

  yyprintf("} ");
}


void writeConfiguration(Configuration * c) {
  if (c == NULL) {
    //printf("writeConfiguration warning: c is NULL\n");
    return;
  }
  writeProduction(grammar.rules[c->ruleID], c->marker);

  if (USE_LR0 == TRUE && USE_LALR == FALSE) { // LR(0), no context.
    // do nothing unless is goal production.
    if (c->ruleID == 0) yyprintf2("%s", strEnd);
  } else {
    writeContext(c->context);
  }

  if (c->isCoreConfig == 1) yyprintf(" (core) ");

  if (TRUE == USE_LALR && SHOW_ORIGINATORS == TRUE) {
    if (c->LANE_END == 1) { yyprintf(" [LANE_END]"); }
    if (c->LANE_CON == 1) { yyprintf(" [LANE_CON]"); }
    if (c->COMPLETE == 1) { yyprintf(" [COMPLETE]"); }
    yyprintf("\n");
    //yyprintf2(" [owner: %d]", c->owner->state_no);
    writeConfigOriginators(c);     /* in lane_tracing.c */
    writeConfigTransitors(c);      /* in lane_tracing.c */
  } else {
    yyprintf("\n");
  }

}


void writeCoreConfiguration(State * s) {
  int i;
  yyprintf("~~~~~Core configurations.Start~~~~~\n");
  for (i = 0; i < s->core_config_count; i ++) {
    writeConfiguration(s->config[i]);
  }
  yyprintf("~~~~~Core configurations.End~~~~~\n");
}


void writeSuccessorList(State * s) {
  int i;
  if (s->successor_count > 0) yyprintf("\n");
  //  yyprintf("\n-successor list-\n");
  for (i = 0; i < s->successor_count; i ++) {
    yyprintf3("%s : %d\n",
             s->successor_list[i]->trans_symbol->snode->symbol,
             s->successor_list[i]->state_no);
  }
}


void writeStateConflictList(int state) {
  Conflict * c;

  if (USE_REMOVE_UNIT_PRODUCTION) {
    state = getActualState(state);
    //if (state == -1) return; // removed state.
  }

  if (state < 0) return;

  if (states_new_array->rr_count[state] == 0 &&
      states_new_array->rs_count[state] == 0) return;

  for (c = states_new_array->conflict_list[state]; c != NULL; c = c->next) {
    yyprintf2("%d: ", c->state);
    if (c->s > 0) {
      yyprintf("shift/reduce conflict ");
      yyprintf3("(shift %d, red'n %d)", c->s, (-1) * c->r);
    } else {
      yyprintf("reduce/reduce conflict ");
      yyprintf3("red'n %d, red'n %d]", (-1) * c->s, (-1) * c->r);
    } // end if
    yyprintf2(" on '%s'\n", c->lookahead->symbol);
  } // end for

}


void writeGrammarConflictList() {
  int i;

  if (rs_count == 0 && rr_count == 0) return;
  //yyprintf("==Conflict List==\n\n");
  yyprintf("Conflicts:");
  yyprintf3("  %d shift/reduce, %d reduce/reduce\n\n",
            rs_count, rr_count);

  for (i = 0; i < ParsingTblRows; i ++) {
    if (states_new_array->rs_count[i] > 0) {
      yyprintf4("  state %d: %d shift/reduce conflict%s",
             i, states_new_array->rs_count[i],
             (states_new_array->rs_count[i] == 1)?"":"s");
      if (states_new_array->rr_count[i] > 0) {
        yyprintf3(", %d reduce/reduce conflict%s",
               states_new_array->rr_count[i],
               (states_new_array->rr_count[i] == 1)?"":"s");
      }
      yyprintf("\n");
    } else if (states_new_array->rr_count[i] > 0) {
      yyprintf4("  state %d: %d reduce/reduce conflict%s\n",
                i, states_new_array->rr_count[i],
                (states_new_array->rr_count[i] == 1)?"":"s");
    }
  }
  yyprintf("\n");
}


/*
 *  Used when USE_REMOVE_UNIT_PROD is used.
 */
void writeGrammarConflictList2() {
  int i, state, diff;
  int final_rs_count = 0;
  int final_rr_count = 0;
  State_array * a = states_new_array;

  if (rs_count == 0 && rr_count == 0) return;
  //yyprintf("==Conflict List==\n\n");
  yyprintf("Conflicts:");
  yyprintf3("  %d shift/reduce, %d reduce/reduce]\n\n",
            rs_count, rr_count);

  for (i = 0; i < ParsingTblRows; i ++) {
    if (isReachableState(i) == FALSE) continue;

    state = getActualState(i);

    if (a->rs_count[i] > 0) {
      yyprintf4("  state %d: %d shift/reduce conflict%s", state,
                a->rs_count[i], (a->rs_count[i] == 1)?"":"s");
      final_rs_count += a->rs_count[i];
      if (a->rr_count[i] > 0) {
        yyprintf3(", %d reduce/reduce conflict%s",
               a->rr_count[i], (a->rr_count[i] == 1)?"":"s");
        final_rr_count += a->rr_count[i];
      }
      yyprintf("\n");
    } else if (a->rr_count[i] > 0) {
      yyprintf4("  state %d: %d reduce/reduce conflict%s\n", state,
                a->rr_count[i], (a->rr_count[i] == 1)?"":"s");
      final_rr_count += a->rr_count[i];
    }
  }

  if ((diff = rs_count - final_rs_count) > 0)
    yyprintf3("  [%d shift/reduce conflict%s in removed states]\n",
              diff, (diff > 1)?"s":"");
  if ((diff = rr_count - final_rr_count) > 0)
    yyprintf3("  [%d reduce/reduce conflict%s in removed states]\n",
              diff, (diff > 1)?"s":"");

  yyprintf("\n");
}


void writeState(State * s) {
  int i;
  if (s == NULL) {
    //printf("writeState warning: s is NULL\n");
    return;
  }

  writeStateConflictList(s->state_no);

  yyprintf4("--state %d-- config count:%d, core_config count:%d\n",
            s->state_no, s->config_count, s->core_config_count);
  for (i = 0; i < s->config_count; i ++) {
    writeConfiguration(s->config[i]);
  }

  writeSuccessorList(s);

  if (TRUE == USE_LALR && TRUE == SHOW_ORIGINATORS) {
    StateList_write(s->parents_list);
  }

  //writeCoreConfiguration(s);
  yyprintf("\n");
}


void writeStateCollection(State_collection * c) {
  State * s;

  yyprintf2("==State Collection: (count=%d)\n", c->state_count);

  s = c->states_head;
  while (s != NULL) {
    writeState(s);
    s = s->next;
    //yyprintf("------------------------------------------\n");
    yyprintf("\n");
  }

  if (c->state_count == 0) yyprintf("(empty)\n");
  yyprintf("\n");
}


void destroyState(State * s) {
  int i;
  if (s == NULL) {
    //printf("destroyState warning: s is NULL\n");
    return;
  }

  for (i = 0; i < s->config_count; i ++) {
    freeConfig(s->config[i]);
  }

  free(s->config);
  free(s->trans_symbol);
  free(s->successor_list);
  StateList_destroy(s->parents_list);

  free(s);
}


/*
 * Determine if string s is a non-terminal of grammar g.
 *
 * Instead of searching the entire list of non_terminal_list,
 * use hash table node's type. In O(1) time.
 * The type was obtained when calling get_terminals() and
 * get_nonterminals().
 */
BOOL isNonTerminal(SymbolTblNode * s) {
  if (s->type == _NONTERMINAL) return TRUE;
  return FALSE;
}


BOOL isTerminal(SymbolTblNode * s) {
  if (s->type == _TERMINAL) return TRUE;
  return FALSE;
}


/*
 * Add the given symbol to the context in increasing order.
 * There is no need to sort context later.
 */
BOOL addSymbol2Context(SymbolTblNode * snode, Context * c) {
  SymbolNode * s, * t, * s_prev;
  int cmp_val;

  for (s_prev = NULL, s = c->nContext; s != NULL;
       s_prev = s, s = s->next) {

    cmp_val = strcmp(s->snode->symbol, snode->symbol);
    if (cmp_val == 0) return FALSE; // already in context.
    if (cmp_val > 0) { // s_prev < snode < s
      t = createSymbolNode(snode);
      t->next = s;
      if (s_prev == NULL) { c->nContext = t; }
      else { s_prev->next = t; }
      c->context_count ++;

      return TRUE; // inserted in the middle of context list.
    }
    // else: cmp_val < 0, go to next context node.
  }

  // insert at the end of list. now s == NULL.
  if (s_prev == NULL) { c->nContext = createSymbolNode(snode); }
  else { s_prev->next = createSymbolNode(snode); }
  c->context_count ++;

  return TRUE;
}


/*
 * Insert snode to the list, no repetition allowed, increasing order.
 * Do it like insertion sort.
 *
 * @parameters:
 *  exist - label whether snode already existed.
 */
SymbolNode * insertSymbolList_unique_inc(
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
 * Insert symbol to list tail if not exist, un-ordered.
 */
SymbolNode * insertUniqueSymbolList(
    SymbolList list, SymbolTblNode * snode, int * exist) {
  SymbolNode * n, * n_prev;
  * exist = 0;

  if (list == NULL) return createSymbolNode(snode);

  for (n = list, n_prev = NULL; n != NULL; n_prev = n, n = n->next) {
    if (n->snode == snode) {
      * exist = 1;
      return list; // existing node.
    }
  } // end of for.

  // insert as the last node.
  n_prev->next = createSymbolNode(snode);
  return list;
}


void writeSymbolNodeArray(SymbolNode * str) {
  SymbolNode * a;
  for (a = str; a != NULL; a = a->next) {
    if (a != str) yyprintf(", ");
    yyprintf2("%s", a->snode->symbol);
  }
  yyprintf("\n");
}


void showTHeads(SymbolList alpha, SymbolList theads) {
  SymbolNode * a;

  yyprintf("string '");

  a = alpha;
  for (a = alpha; a != NULL; a = a->next)
    yyprintf2("%s ", a->snode->symbol);

  yyprintf("' has theads: ");

  for (a = theads; a != NULL; a = a->next)
    yyprintf2("%s ", a->snode->symbol);

  yyprintf("\n");
}


void insertAlphaToHeads(SymbolNode * s,
    SymbolNode * heads, SymbolNode * theads) {
  SymbolNode * a;
  SymbolTblNode * snode;
  int exist;

  for (a = s; a != NULL; a = a->next) {
    if (isVanishSymbol(a->snode) == TRUE) {
      // note: a vanishable symbol must be a non-terminal.
      heads->next = insertUniqueSymbolList(
                      heads->next, a->snode, & exist);
    } else { // not vanlish symbol. break after insertion.
      if (isTerminal(a->snode)) { // here actually can insert to tail.
        theads->next = insertSymbolList_unique_inc(
                       theads->next, a->snode, & exist);
      } else { // non_terminal.
        heads->next = insertUniqueSymbolList(
                      heads->next, a->snode, & exist);
      }
      return;
    }  // end for
  }

  // all symbols are vanishable, since didn't return in the cycle.
  snode = hashTbl_find("");
  theads->next = insertSymbolList_unique_inc(
                     theads->next, snode, & exist);
}


/*
 * Insert the RHS symbols to heads up to an unvanishable symbol.
 */
void insertRHSToHeads(SymbolNode * s,
    SymbolNode * heads, SymbolNode * theads) {
  SymbolNode * a;
  int exist;

  for (a = s; a != NULL; a = a->next) {
    if (isVanishSymbol(a->snode) == TRUE) {
      // note: a vanishable symbol must be a non-terminal.
      heads->next = insertUniqueSymbolList(
                      heads->next, a->snode, & exist);
    } else { // not vanlish symbol. break after inserting last one.
      if (isTerminal(a->snode)) {
        theads->next = insertSymbolList_unique_inc(
                       theads->next, a->snode, & exist);
      } else { // non_terminal.
        heads->next = insertUniqueSymbolList(
                      heads->next, a->snode, & exist);
      }
      return;
    }  // end for
  }

}


/*
 * Algorithm:
 *
 * insert each symbol S in alpha to heads until S is NOT vanishable.
 * if S is a terminal, then insert S to theads; else insert to heads.
 * if all symbols are N.T., insert empty string to theads.
 *
 * for each symbol A in heads {
 *   for each grammar rule r where A is the LHS {
 *     for each symbol B in r's RHS until NOT vanishable {
 *       if B is NT, insert B to heads's tail;
 *       else B is T, insert (like insertion sort) to theads.
 *     }
 *   }
 * }
 *
 * @added to replace the old one on: 3/9/2008
 */
SymbolNode * getTHeads(SymbolNode * alpha) {
  SymbolNode * n, * theads, * heads;
  RuleIDNode * rules;
  Production * p;

  // dummy header of the lists heads and theads.
  heads = createSymbolNode(hashTbl_find(""));
  theads = createSymbolNode(hashTbl_find(""));

  insertAlphaToHeads(alpha, heads, theads);

  for (n = heads->next; n != NULL; n = n->next) {
    for (rules = n->snode->ruleIDList;
         rules != NULL; rules = rules->next) {
      p = grammar.rules[rules->ruleID];
      insertRHSToHeads(p->nRHS_head, heads, theads);
    }
  }

  freeSymbolNodeList(heads);

  // remove the dummy header of theads list.
  n = theads;
  theads = theads->next;
  freeSymbolNode(n);

  if (DEBUG_GEN_PARSING_MACHINE == TRUE) {
    yyprintf("==getTHeads: theads for: ");
    writeSymbolNodeArray(alpha);
    writeSymbolNodeArray(theads);
  }

  return theads;
}


/*
 * Helper function for getContext().
 * This section of code is called three times.
 */
void getContext_do(Configuration * cfg, Context * context) {
  SymbolNode * a = cfg->context->nContext;
  while (a != NULL) {
    addSymbol2Context(a->snode, context);
    a = a->next;
  }
}


/*
 * Obtain the context for a configuration.
 */
void getContext(Configuration * cfg, Context * context) {
  SymbolList alpha = NULL; // a list of symbols.
  SymbolList theads = NULL;
  SymbolNode * a;
  Production * production = grammar.rules[cfg->ruleID];

  if (cfg->marker == production->RHS_count - 1) {
    // is last symbol, just copy the context.
    getContext_do(cfg, context);
  } else { // need to find thead(alpha)
    if (SHOW_THEADS) writeConfiguration(cfg);

    // alpha is the string after scanned symbol.
    alpha = cfg->nMarker->next; // we know cfg->nMarker != NULL.
    theads = getTHeads(alpha);

    if (SHOW_THEADS) { showTHeads(alpha, theads); }

    // if theads_count == 0, just copy the context.
    if (theads == NULL) {
      getContext_do(cfg, context);
    } else { // theads_count > 0
      for (a = theads; a != NULL; a = a->next) {
        if (strlen(a->snode->symbol) == 0) { // empty string.
          // Entire alpha vanishable. Copy context.
          getContext_do(cfg, context);
        } else {
          addSymbol2Context(a->snode, context);
        }
      } // end of for

    }
  } // end of if-else

  freeSymbolNodeList(theads);
}


/*
 * Empty a context.
 * Note: if a is NULL, free(a) causes crash.
 */
void clearContext(Context * c) {
  if (c == NULL) return;

  c->context_count = 0;
  freeSymbolNodeList(c->nContext);
  c->nContext = NULL;
}


void freeContext(Context * c) {
  if (c == NULL) return;
  clearContext(c);
  free(c);
}


void clearProduction(Production * p) {
  SymbolNode * a, * b;

  if (p == NULL) return;
  if (p->nLHS != NULL) free(p->nLHS);
  if (p->nRHS_head != NULL) {
    a = p->nRHS_head;
    p->nRHS_head = NULL;
    while(a != NULL) {
      b = a->next;
      freeSymbolNode(a);
      a = b;
    }
  }
}


void freeProduction(Production * p) {
  clearProduction(p);
  free(p);
}


void freeConfig(Configuration * c) {
  if (c == NULL) return;
  clearContext(c->context);
  free(c);
}


/*
 * Determine if productions p1 and p2 are the same.
 * Can be used for general production comparison.
 * In this program this is no longer used.
 * But can be used to find out if the grammar has
 * repeated rules etc.
 */
BOOL isSameProduction(Production * p1, Production * p2) {
  SymbolNode * a, * b;

  if (p1->nLHS->snode != p2->nLHS->snode) { return FALSE; }
  if (p1->RHS_count != p2->RHS_count) { return FALSE; }

  a = p1->nRHS_head;
  b = p2->nRHS_head;

  while (a != NULL) {
    if (a->snode != b->snode) return FALSE;
    a = a->next;
    b = b->next;
  }
  return TRUE;
}


/*
 * Determine if contexts c1 and c2 are the same.
 */
BOOL isSameContext(Context * c1, Context * c2) {
  SymbolNode * a, * b;

  if (c1->context_count != c2->context_count) return FALSE;

  a = c1->nContext;
  b = c2->nContext;
  while (a != NULL) {
    if (a->snode != b->snode) return FALSE;
    a = a->next;
    b = b->next;
  }
  return TRUE;
}


/*
 * Determine if configurations con and c are the same.
 */
BOOL isSameConfig(Configuration * con, Configuration * c) {
  if (con->marker != c->marker) return FALSE;
  if (con->ruleID != c->ruleID) return FALSE;
  if (isSameContext(con->context, c->context)
      == FALSE) return FALSE;
  return TRUE;
}


/*
 * Note that a successor config's marker = 0.
 */
BOOL isExistingSuccessorConfig(State * s, int ruleID, Context * con) {
  int i;
  Configuration * c;
  for (i = 0; i < s->config_count; i ++) {
    c = s->config[i];
    if (c->marker == 0 && ruleID == c->ruleID &&
        isSameContext(con, c->context) == TRUE)
      return TRUE; // existing config
  }
  return FALSE;
}


void addSuccessorConfigToState(State * s, int ruleID, Context * con) {
  Configuration * c;

  if (s->config_count >= s->config_max_count - 1) {
    s->config_max_count *= 2;
    HYY_EXPAND(s->config, Configuration *, s->config_max_count);
  }

  // marker = 0, isCoreConfig = 0.
  c = createConfig(ruleID, 0, 0);
  c->owner = s;

  copyContext(c->context, con);

  s->config[s->config_count] = c;
  s->config_count ++;
}


BOOL isFinalConfiguration(Configuration * c) {
  if (c->marker == grammar.rules[c->ruleID]->RHS_count) return TRUE;
  return FALSE;
}


BOOL isEmptyProduction(Configuration * c) {
  if (grammar.rules[c->ruleID]->RHS_count == 0) return TRUE;
  return FALSE;
}


/*
 * Used by addCoreConfig2State() only.
 * comparison is made alphabetically on:
 * 1) production,
 * 2) marker,
 * 3) context.
 * Actually, for the same state,
 * core configurations are different only
 * by production. But for the same state,
 * different core configuration comparison
 * will be sure to end in production comparison
 * -- either > or <. So does not matter to
 * add in the extra code to compare marker and
 * context. This function thus can be used
 * for general config comparison, although in
 * this program it's used only here.
 */
int config_cmp(Configuration * c1, Configuration * c2) {
  SymbolNode * a, * b;
  int cmp_val, i, count;

  Production * c1_production = grammar.rules[c1->ruleID];
  Production * c2_production = grammar.rules[c2->ruleID];

  // printf("compare LHS\n");
  cmp_val = strcmp(c1_production->nLHS->snode->symbol,
                   c2_production->nLHS->snode->symbol);
  if (cmp_val != 0) return cmp_val;

  // printf("compare RHS\n");
  count = c1_production->RHS_count;
  if (count > c2_production->RHS_count) {
    count = c2_production->RHS_count;
  }
  a = c1_production->nRHS_head;
  b = c2_production->nRHS_head;
  for (i = 0; i < count; i ++) {
    cmp_val = strcmp(a->snode->symbol, b->snode->symbol);
    if (cmp_val != 0) return cmp_val;
    a = a->next;
    b = b->next;
  }

  cmp_val = c1_production->RHS_count -
            c2_production->RHS_count;
  if (cmp_val > 0) { return  1; } //c1 RHS is longer.
  if (cmp_val < 0) { return -1; } //c2 RHS is longer.

  // If productions are the same, go on to compare context.
  //printf("compare marker\n");
  cmp_val = c1->marker - c2->marker;
  if (cmp_val > 0) { return  1; }
  if (cmp_val < 0) { return -1; }

  // If production and marker are the same, go on to compare context.
  // printf("compare context\n");
  count = c1->context->context_count;
  if (count > c2->context->context_count) {
    count = c2->context->context_count;
  }

  a = c1->context->nContext;
  b = c2->context->nContext;
  while (a != NULL) {
    cmp_val = strcmp(a->snode->symbol, b->snode->symbol);
    if (cmp_val != 0) return cmp_val;
    a = a->next;
    b = b->next;
  }

  //printf("compare context count\n");
  cmp_val = c1->context->context_count -
            c2->context->context_count;
  if (cmp_val > 0) { return   1; }
  if (cmp_val < 0) { return  -1; }

  return 0; // the same
}


/*
 * Add core configurations in increasing order
 * The order is by production, and marker.
 * Assumption: a core config won't be inserted twice.
 */
void addCoreConfig2State(State * s, Configuration * new_config) {
  int i, j, cmp_val;

  if (s->config_count >= s->config_max_count - 1) {
    s->config_max_count *= 2;
    HYY_EXPAND(s->config, Configuration *, s->config_max_count);
    //printf("addCoreConfig2State: state size expanded to %d\n",
    //       s->config_max_count);
  }

  for (i = 0; i < s->config_count; i ++) {
    cmp_val = config_cmp(s->config[i], new_config);
    if (cmp_val == 0) {
      // a core config shouldn't be added twice.
      printf("addCoreConfig2State error: %s",
             "a core config shouldn't be added twice.\n");
      exit(1); // should never happen.
    }
    if (cmp_val > 0) break; // found insertion point.
  }

  for (j = s->config_count; j > i; j --) {
    s->config[j] = s->config[j-1];
  }

  s->config[i] = new_config; // i == j is insert point.

  s->config_count ++;
  s->core_config_count ++;
}


/////////////////////////////////////////////////////
// Use config_queue when get closure for a state.
/////////////////////////////////////////////////////


/*
 * Note that a successor config's marker = 0.
 * Returns the index of the compatible config in the state.
 */
int isCompatibleSuccessorConfig(State * s, int ruleID) {
  int i;
  Configuration * c;
  for (i = 0; i < s->config_count; i ++) {
    c = s->config[i];
    if (c->marker == 0 && ruleID == c->ruleID)
      return i; // existing compatible config
  }
  return -1;
}


/*
 * Assumption: public variable config_queue contains
 * the configurations to be processed.
 */
void getConfigSuccessors(State * s) {
  RuleIDNode * r;
  SymbolTblNode * scanned_symbol = NULL;
  Configuration * config;
  int index;
  static Context tmp_context;
  tmp_context.nContext = NULL;

  while (queue_count(config_queue) > 0) {
    config = s->config[queue_pop(config_queue)];

    if (config->marker >= 0 &&
        config->marker < grammar.rules[config->ruleID]->RHS_count) {
      scanned_symbol = getScannedSymbol(config);

      if (isNonTerminal(scanned_symbol) == TRUE) {

        clearContext(& tmp_context); // clear tmp_context
        getContext(config, & tmp_context);

        for (r = scanned_symbol->ruleIDList; r != NULL; r = r->next) {
          // Grammar.rules[r->ruleID] starts with this scanned symbol.

          // If not an existing config, add to state s.
          index = isCompatibleSuccessorConfig(s, r->ruleID);

          if (index == -1) { // new config.
            addSuccessorConfigToState(s, r->ruleID, & tmp_context);
            queue_push(config_queue, s->config_count - 1);

          } else if (combineContext(s->config[index]->context,
                     & tmp_context) == TRUE) { // compatible config
            // if this config has no successor, don't insert to
            // config_queue. This saves time.
            // marker = 0 here, no need to check marker >= 0.
            if (isFinalConfiguration(s->config[index]) == TRUE) continue;
            if (isTerminal(getScannedSymbol(s->config[index]))
                           == TRUE) continue;
            if (queue_exist(config_queue, index) == 1) continue;

            // else, insert to config_queue.
            queue_push(config_queue, index);

          }
          //else { // same config, do nothing. }

        } // end for
      } // else, is a terminal, stop.
    }  // end if config-marker >= 0 ...
  } // end of while
}


void getClosure(State * s) {
  int i;
  //queue_clear(config_queue);
  for (i = 0; i < s->config_count; i ++) {
    queue_push(config_queue, i);
  }
  getConfigSuccessors(s);
}


///////////////////////////////////////////
// State_collection functions. START.
///////////////////////////////////////////

State_collection * createStateCollection() {
  State_collection * c = (State_collection *)
                         malloc(sizeof(State_collection));
  if (c == NULL)
    YYERR_EXIT("createStateCollection error: out of memory\n");

  c->states_head = NULL;
  c->states_tail = NULL;
  c->state_count = 0;

  return c;
}


void destroyStateCollection(State_collection * c) {
  State * s;
  State * next;
  if (c == NULL) return;

  s = c->states_head;
  while (s != NULL) {
    next = s->next;
    destroyState(s);
    s = next;
  }

  free(c);
}


State * addState2Collection(State_collection * c, State * new_state) {
  if (c == NULL || new_state == NULL) return NULL;

  new_state->next = NULL;

  if (c->state_count == 0) {
    c->states_head = new_state;
    c->states_tail = new_state;
  } else {
    c->states_tail->next = new_state;
    c->states_tail = new_state;
  }
  c->state_count ++;

  return c->states_tail;
}

///////////////////////////////////////////
// State_collection functions. END.
///////////////////////////////////////////


/*
 * Get the scanned symbol of configuration c.
 * The scanned symbol can be obtained by nMarker pointer
 * as here, or by marker which needs more calculation.
 */
SymbolTblNode * getScannedSymbol(Configuration * c) {
  if (c->nMarker == NULL) return NULL;
  return c->nMarker->snode;
}


/*
 * Used by function transition.
 * Returns the successor state that is the result of
 * transition following the given symbol.
 */
State * findStateForScannedSymbol(
        State_collection * c, SymbolTblNode * symbol) {
  State * s;
  if (c == NULL) return NULL;

  s = c->states_head;
  while (s != NULL) {
    if (symbol == s->trans_symbol->snode) return s;
    s = s->next;
  }

  return NULL;
}


Context * createContext() {
  Context * c;
  HYY_NEW(c, Context, 1);
  c->nContext = NULL;
  c->context_count = 0;
  c->next = NULL; // used by LR(k) only.
  return c;
}


Configuration * createConfig(
    int ruleID, int marker, int isCoreConfig) {
  Configuration * c;
  c = (Configuration *) malloc(sizeof(Configuration));
  if (c == NULL) {
    printf("createConfig error: out of memory\n");
    exit(0);
  }

  c->context = createContext();

  c->ruleID = ruleID;
  c->marker = marker;
  c->isCoreConfig = isCoreConfig;

  c->nMarker = NULL;
  if (ruleID >= 0)
    c->nMarker = grammar.rules[ruleID]->nRHS_head;

  c->owner = NULL;
  if (USE_LALR == TRUE) {
    c->ORIGINATOR_CHANGED = FALSE;
    c->COMPLETE = 0;
    c->IN_LANE  = 0;
    c->LANE_END = 0;
    c->LANE_CON = 0;
    c->CONTEXT_CHANGED = 0;
    c->originators = createOriginatorList();
    c->transitors = createOriginatorList();
  }

  return c;
}


void copyContext(Context * dest, Context * src) {
  dest->context_count = src->context_count;

  dest->nContext = NULL;
  if (src->nContext != NULL) {
    SymbolNode * a = src->nContext;
    SymbolNode * b = dest->nContext = createSymbolNode(a->snode);
    while ((a = a->next) != NULL) {
      b->next = createSymbolNode(a->snode);
      b = b->next;
    }
  }
}


/*
 * return the copy of a config.
 * used by function transition when creating new state.
 */
void copyConfig(Configuration * c_dest, Configuration * c_src) {
  c_dest->marker = c_src->marker;
  c_dest->isCoreConfig = c_src->isCoreConfig;
  c_dest->ruleID = c_src->ruleID;
  c_dest->nMarker = c_src->nMarker;
  copyContext(c_dest->context, c_src->context);
}


/////////////////////////////////////////////////////////
// Functions for combining compatible states. START.
/////////////////////////////////////////////////////////


/*
 * Two configurations are common core configurations if
 * they have the same production and marker, but
 * DIFFERENT contexts.
 */
BOOL isCommonConfig(Configuration * con, Configuration * c) {
  if (con->marker != c->marker) return FALSE;
  if (con->ruleID != c->ruleID) return FALSE;
  // If all same, then are same config, not common config!
  if (isSameContext(con->context, c->context)
      == TRUE) return FALSE;
  return TRUE;
}


/*
 * Pre-assumption: s1, s2 have at least one core config.
 * Returns true if at least one config pair have common config.
 */
BOOL hasCommonCore(State * s1, State * s2) {
  int i;
  BOOL result;
  if (s1 == NULL || s2 == NULL) return FALSE;
  //printf("hasCommonCore: \n");

  result = FALSE;
  if (s1->core_config_count != s2->core_config_count)
    return FALSE;
  if (s1->core_config_count == 0) return FALSE;
  for (i = 0; i < s1->core_config_count; i ++) {
    if (isSameConfig(s1->config[i], s2->config[i]) == TRUE) {
      // do nothing.
    } else if (isCommonConfig(s1->config[i],
                              s2->config[i]) == TRUE) {
      result = TRUE;
    } else { // not same, and not common core.
      return FALSE;
    }
  }
  return result;
}


/*
 * Pre-assumption: contexts c1 and c2 are sorted in increasing order.
 * See function addSymbol2Context().
 */
BOOL hasEmptyIntersection(Context * c1, Context * c2) {
  SymbolNode * a, * b;

  a = c1->nContext;
  b = c2->nContext;

  // intrinsically this is O(m + n).
  while (a != NULL && b != NULL) {
    if (a->snode == b->snode) return FALSE; // common element found
    else if (strcmp(a->snode->symbol, b->snode->symbol) < 0) {
      a = a->next;
    } else {
      b = b->next;
    }
  }


  return TRUE;
}


/*
 * condition (a).
 * if (a) is satisfied, return TRUE, otherwise FALSE.
 */
BOOL isCompatibleState_a(State * s1, State * s2) {
  int i, j;
  Context * c1, * c2;
  int count = s1->core_config_count;
  for (i = 0; i < count; i ++) {
    for (j = 0; j < count; j ++) {
      if (i != j) {
        c1 = s1->config[i]->context;
        c2 = s2->config[j]->context;
        if (hasEmptyIntersection(c1, c2) == FALSE) {
          return FALSE;
        } // end if
      } // end if
    } // end for
  } // end for
  return TRUE;
}


/*
 * condition (b) or (c).
 * if (b) or (c) is satisfied, return TRUE, otherwise FALSE.
 */
BOOL isCompatibleState_bc(State * s) {
  int i, j;
  Context * c1, * c2;
  int count = s->core_config_count;
  for (i = 0; i < count - 1; i ++) {
    for (j = i + 1; j < count; j ++) {
      c1 = s->config[i]->context;
      c2 = s->config[j]->context;
      if (hasEmptyIntersection(c1, c2) == TRUE) {
        return FALSE;
      } // end if
    } // end for
  } // end for
  return TRUE;
}


/*
 * Pre-assumption:
 *   s1 and s2 have at least one core configuration.
 */
BOOL isCompatibleStates(State * s1, State * s2) {
  int count;

  if (hasCommonCore(s1, s2) == FALSE) return FALSE;

  count = s1->core_config_count;
  if (count == 1) return TRUE;

  // now check context to see if s1 and s2 are compatible.
  if (isCompatibleState_a(s1, s2) == TRUE) return TRUE;
  if (isCompatibleState_bc(s1) == TRUE) return TRUE;
  if (isCompatibleState_bc(s2) == TRUE) return TRUE;

  return FALSE;
}


/*
 * Used by combineCompatibleStates() and propagateContextChange().
 */
void updateStateParsingTblEntry(State * s) {
  SymbolTblNode * scanned_symbol = NULL;
  int i;
  for (i = 0; i < s->config_count; i ++) {
    scanned_symbol = getScannedSymbol(s->config[i]);

    // for final config and empty reduction.
    if (isFinalConfiguration(s->config[i]) == TRUE ||
        strlen(scanned_symbol->symbol) == 0) {
      insertReductionToParsingTable(s->config[i], s->state_no);
    }
  }
}


/*
 * Used by propagateContextChange() and push_state_context_change().
 *
 * Find a similar core config for c in State t.
 *
 * similar core config:
 * - production and marker are the same.
 * - Context does not matter.
 */
Configuration * findSimilarCoreConfig(State * t, Configuration * c,
                                      int * config_index) {
  int i;
  Configuration * tmp;

  for (i = 0; i < t->core_config_count; i ++) {
    tmp = t->config[i];

    // don't compare context, it'll be compared in combineContext().
    if (tmp->marker == c->marker && tmp->ruleID == c->ruleID) {
      (* config_index) = i;
      return tmp;
    }
  }
  return NULL;
}


/*
 * For each successor state s, update context of core
 * configurations, then get closure, add change to
 * parsing table. Do this recursively.
 *
 * In more detail:
 * For each successor state t of s:
 *   look through each config c in the config list of s:
 *     if scanned symbol of c == trans_symbol of t {
 *       // c transits to a core config in t
 *       find core config d in t which is the transition
 *       result of c,
 *       update the context of d from c.
 *       if the context of d is changed {
 *         get closure of d again,
 *         update parsing table, and
 *         propagate context change to d's successors.
 *     }
 */
void propagateContextChange(State * s) {
  BOOL isChanged;
  SymbolTblNode * trans_symbol = NULL;
  Configuration * c, * d;
  int i, j;
  State * t; // successor.
  int config_index;

  if (s->successor_count == 0) return;

  for (i = 0; i < s->successor_count; i ++) {
    t = s->successor_list[i];
    trans_symbol = t->trans_symbol->snode;
    isChanged = FALSE;

    for (j = 0; j < s->config_count; j ++) {
      c = s->config[j]; // bug removed: i -> j. 3-4-2008.
      if (isFinalConfiguration(c) == TRUE) continue;
      // if not a successor on symbol, next.
      if (trans_symbol != getScannedSymbol(c)) continue;

      c->marker += 1;
      d = findSimilarCoreConfig(t, c, & config_index);
      c->marker -= 1;
      if (d == NULL) continue;

      if (combineContext(d->context, c->context) == TRUE) {
        //printf("context of state %d updated\n", t->state_no);
        isChanged = TRUE;

        //queue_clear(config_queue);
        queue_push(config_queue, config_index);
        getConfigSuccessors(t);
      }
    } // end of for

    if (isChanged == TRUE) {
      updateStateParsingTblEntry(t);
      propagateContextChange(t);
    }
  } // end of for
}


/*
 * Assumption: s_dest and s_src have common core,
 *   and are weakly compatible.
 * NOTE: s_dest is from states_new.
 */
BOOL combineCompatibleStates(State * s_dest, State * s_src) {
  BOOL isChanged = FALSE;
  int i;
  for (i = 0; i < s_dest->core_config_count; i ++) {
    if (combineContext(
          s_dest->config[i]->context,
          s_src->config[i]->context) == TRUE) {
      isChanged = TRUE;

      //queue_clear(config_queue);
      queue_push(config_queue, i);
      getConfigSuccessors(s_dest);
    }
  }

  // Now propagate the context change to successor states.
  if (isChanged == TRUE) {
    updateStateParsingTblEntry(s_dest);
    propagateContextChange(s_dest);
  }
  return isChanged;
}


/////////////////////////////////////////////////////////
// Functions for combining compatible states. END.
/////////////////////////////////////////////////////////


/*
 * Determine if two states are the same.
 * This is done by checking if the two states have the
 * same core configurations.
 * Since the core confiurations are both in increasing order,
 * just compare them by pairs.
 */
BOOL isSameState(State * s1, State * s2) {
  int i;

  if (s1->core_config_count != s2->core_config_count) {
    return FALSE;
  }

  for (i = 0; i < s1->core_config_count; i ++) {
    if (isSameConfig(s1->config[i], s2->config[i]) == FALSE)
      return FALSE;
  }

  return TRUE;
}


/*
 * Determine if a state exists in a state collection.
 * This is done by comparing s with each state in sc.
 *
 * NOTE: combining compatible states is done here!
 */
State * isExistingState(State_collection * sc, State * s,
                        int * is_compatible) {
  State * t = sc->states_head;
  int i = 0; // index of state.

  while (t != NULL) {
    if (isSameState(t, s) == TRUE) {
      return t;
    }
    if (USE_COMBINE_COMPATIBLE_STATES) {
      if (isCompatibleStates(t, s) == TRUE) {
        combineCompatibleStates(t, s);
        (* is_compatible) = 1;
        return t;
      }
    }
    i ++;
    t = t->next;
  }

  return NULL;
}


State * createState() {
  State * s = (State *) malloc(sizeof(State));
  if (s == NULL) {
    printf("createState error: out of memeory\n");
    exit(0);
  }
  s->next = NULL;
  s->config_max_count = STATE_INIT_SIZE;
  HYY_NEW(s->config, Configuration *, s->config_max_count);
  s->config_count = 0;
  s->state_no = -1;
  s->trans_symbol = createSymbolNode(hashTbl_find(""));
  s->core_config_count = 0;

  // Initialization for successor list.
  s->successor_max_count = STATE_SUCCESSOR_INIT_MAX_COUNT;
  s->successor_list = (State **)
     malloc(sizeof(State *) * s->successor_max_count);
  if (s->successor_list == NULL) {
    YYERR_EXIT("createState error: out of memory\n");
  }
  s->successor_count = 0;

  s->parents_list = StateList_create(s->parents_list);

  s->PASS_THRU = 0;
  s->REGENERATED = 0;

  return s;
}


void insertReductionToParsingTable(
       Configuration * c, int state_no) {
  SymbolNode * a;

  if (grammar.rules[c->ruleID]->nLHS->snode ==
      grammar.goal_symbol->snode) { //accept, action = "a";
    for (a = c->context->nContext; a != NULL; a = a->next) {
      insertAction(a->snode, state_no, CONST_ACC);
    }
  } else { //reduct, action = "r";
    for (a = c->context->nContext; a != NULL; a = a->next) {
      insertAction(a->snode, state_no, (-1) * c->ruleID);
    }
  }
}


/*
 * Input:
 * s - src state, n - new state.
 * Add n to the successor list of s.
 */
void addSuccessor(State * s, State * n) {
  s->successor_list[s->successor_count] = n;
  s->successor_count ++;

  //printf(":: state %d, succesor is state %d on symbol %s\n",
  //    s->state_no, n->state_no, n->trans_symbol);

  if (s->successor_count >= s->successor_max_count) {
    s->successor_max_count *= 2; // there won't be many
    s->successor_list = (State **) realloc(
       (void *) s->successor_list,
       sizeof(State *) * s->successor_max_count);
    if (s->successor_list == NULL) {
      YYERR_EXIT("addSuccessor error: out of memory\n");
    }
    //printf("Successor list of State %d is expanded to %d\n",
    //        s->state_no, s->successor_max_count);
  }

  if (TRUE == USE_LALR) {
    // add s to the parents_list of n. To get originators in lane-tracing.
    StateList_add(n->parents_list, s);
  }
}


/*
 * Insert a new state to the parsing machine.
 *
 * Used in 3 places: y.c, lr0.c, lane_tracing.c.
 * This can be changed to macro later.
 */
void insert_state_to_PM(State * s) {
  s->state_no = states_new->state_count;

  addState2Collection(states_new, s);
  addStateToStateArray(states_new_array, s);

  // expand size of parsing table array if needed.
  if (states_new->state_count >= PARSING_TABLE_SIZE) {
    expandParsingTable();
  }
}


/*
 * For each of the new temp states,
 *   If it is not one of the existing states in
 *     states_new, then add it to states_new.
 *   Also always add the transition to parsing table.
 *
 * Note:
 * The "is_compatible" variable is for this situation:
 *
 * state s_x is a successor of state s_0.
 * in function isExistingState, s_0 is already in
 * states_new, but s_x is not.
 * now s_0 is in coll too, and when calling function
 * isExistingState, new s_0 is combined to old s_0,
 * which causes propagation of new context to successors
 * of s_0. However this does not include s_x since it's
 * inserted into states_new only after that.
 * The following code ensures that the propagation is
 * performed again over all the states.
 *
 */
BOOL addTransitionStates2New(
       State_collection * coll, State * src_state) {
  BOOL src_state_changed = FALSE;
  int is_compatible;
  State * os, * next, * s;
  s = coll->states_head;

  while (s != NULL) {
    next = s->next;
    //is_compatible = 0;

    // Two alternatives. searchStateHashTbl is slightly faster.
    //if ((os = isExistingState(states_new, s, & is_compatible)) == NULL) {
    if ((os = searchStateHashTbl(s, & is_compatible)) == NULL) {
      insert_state_to_PM(s);

      // Add this new state as successor to src_state.
      addSuccessor(src_state, s);

      // insert shift.
      insertAction(s->trans_symbol->snode,
                   src_state->state_no, s->state_no);

    } else { // same or compatible with an existing state.
      // insert shift.
      insertAction(os->trans_symbol->snode,
                   src_state->state_no, os->state_no);

      addSuccessor(src_state, os);

      destroyState(s); // existing or compatible. No use.

      // This should only happen for general practical method.
      if (os == src_state && is_compatible == 1) {
        src_state_changed = TRUE;
        //printf("src state is changed\n");
      }
    }

    s = next;
  } // end of while.

  return src_state_changed;
}


/*
 * Perform transition opertaion on a state to get successors.
 *
 * For each config c in the state s,
 *   If c is a final config, stop
 *   Else, get the scanned symbol x,
 *     If a new temp state for x does not exist yet, create it.
 *     add x to the temp state.
 * (Now get several new temp states)
 * Add these new temp states to states_new if not existed,
 * and add transition to parsing table as well.
 */
void transition(State * s) {
  int i;
  Configuration * c, * new_config;
  SymbolTblNode * scanned_symbol = NULL;
  State_collection * coll = createStateCollection();
  State * new_state = NULL;

  for (i = 0; i < s->config_count; i ++) {
    c = s->config[i];
    if (isFinalConfiguration(c)) {
      //yyprintf("a final config. so reduce.\n");
      //writeConfiguration(c);
      insertReductionToParsingTable(c, s->state_no);
    } else { // do transit operation.
      scanned_symbol = getScannedSymbol(c);
      if (strlen(scanned_symbol->symbol) == 0) { // insert empty reduction.
        insertReductionToParsingTable(c, s->state_no);
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

  if (coll->state_count > 0) {
   BOOL src_state_changed = addTransitionStates2New(coll, s);
   if (src_state_changed == TRUE) {
     propagateContextChange(s);
   }
  }
}


BOOL isCompatibleConfig(Configuration * c1, Configuration * c2) {
  if (c1 == NULL || c2 == NULL) return FALSE;
  if (c1->marker != c2->marker) return FALSE;
  if (c1->ruleID != c2->ruleID) return FALSE;
  return TRUE;
}


/*
 * Combine c_src into c_dest by copying symbols that
 * occur in c_src but not in c_dest to c_dest.
 *
 * Returns TRUE if any change is made.
 */
BOOL combineContext(Context * c_dest, Context * c_src) {
  SymbolNode * a;
  BOOL isChanged = FALSE;
  if (c_dest == NULL || c_src == NULL) return FALSE;

  for (a = c_src->nContext; a != NULL; a = a->next) {
    if (addSymbol2Context(a->snode, c_dest) == TRUE) {
      isChanged = TRUE;
    }
  }

  return isChanged;
}


/*
 * The main function to generate parsing machine.
 */
void generate_parsing_machine() {
  State * new_state = states_new->states_head;

  if (DEBUG_GEN_PARSING_MACHINE == TRUE) {
    yyprintf("\n\n--generate parsing machine--\n");
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
}


void dump_state_collections() {
  writeStateCollection(states_new);
}


/////////////////////////////////////////////////////////////////
// Parsing table functions.
/////////////////////////////////////////////////////////////////

/*
 * Three places the parsing table is directly manipulated:
 * 1) updateDestState, 2) getAction, 3) insertAction.
 */

/*
 * Use a one-dimension array to store the matrix and
 * calculate the row and column number myself:
 * row i, col j is ParsingTable[col_no * i + j];
 *
 * Here we have:
 * 0 <= i < total states count
 * 0 <= j < col_no
 */
void initParsingTable() {
  int total_cells;

  PARSING_TABLE_SIZE = PARSING_TABLE_INIT_SIZE;
  total_cells = PARSING_TABLE_SIZE * ParsingTblCols;

  HYY_NEW(ParsingTable, int, total_cells);

  memset((void *)ParsingTable, 0, 4 * total_cells);
}


void expandParsingTable() {
  int total_cells = PARSING_TABLE_SIZE * ParsingTblCols;

  HYY_EXPAND(ParsingTable, int, 2 * total_cells);

  memset((void *)(ParsingTable + total_cells), 0, 4 * total_cells);

  PARSING_TABLE_SIZE *= 2;

  expandStateArray(states_new_array, PARSING_TABLE_SIZE);
}



/*
 * Given a state and a transition symbol, find the
 * action and the destination state.
 *
 * row - source state.
 *
 * Results are stored in variables action and state_dest.
 */
void getAction(int symbol_type, int col, int row,
               char * action, int * state_dest) {
  int x = ParsingTable[row * ParsingTblCols + col];

  if (x == 0) {
    * action = 0;
    * state_dest = 0;
    return;
  } else if (x > 0) { // 's' or 'g'
    if (symbol_type == _TERMINAL) {
      * action = 's';
    } else if (symbol_type == _NONTERMINAL) {
      * action = 'g';
    } else {
      printf("getAction error: unknown symbol type %d, col:%d\n",
                  symbol_type, col);
      exit(1);
    }
    * state_dest = x;
    return;
  } else if (x == CONST_ACC) { // 'a'
    * action = 'a';
    * state_dest = 0;
    return;
  } else { // x < 0, 'r'
    * action = 'r';
    * state_dest = (-1) * x;
    return;
  }
}


/*
 * Inserts an action into the parsing table.
 * Input variables include:
 *   state_src - given state.
 *   symbol - transition symbol.
 *   action - action on this symbol at this state.
 *   state_dest - destination state of the action.
 *
 * Actions can be:
 *   r - reduce, state_dest < 0
 *   s - shift,  state_dest > 0
 *   a - accept, state_dest == CONST_ACC
 *   g - goto.   state_dest > 0
 *
 * This is one of the 3 places updating the ParsingTable
 * array directly. The 2nd place is function updateAction().
 * The 3rd place is in the macro
 *   clearStateTerminalTransitions(state_no)
 * in lane_tracing.c.
 */
void insertAction(SymbolTblNode * lookahead, int row, int state_dest) {
  Conflict * c;
  int reduce, shift; // for shift/reduce conflict.
  struct TerminalProperty * tp_s = NULL, * tp_r = NULL;

  int cell = row * ParsingTblCols + getCol(lookahead);

  if (ParsingTable[cell] == 0) {
    ParsingTable[cell] = state_dest;
    return;
  }

  if (ParsingTable[cell] == state_dest) return;

  // ParsingTable[cell] != 0 && ParsingTable[cell] != state_dest.
  // The following code process shift/reduce and reduce/reduce conflicts.

  if (ParsingTable[cell] == CONST_ACC || state_dest == CONST_ACC) {
    if (USE_LR0 == TRUE &&
        (ParsingTable[cell] < 0 || state_dest < 0)) {
      ParsingTable[cell] = CONST_ACC; // ACC wins over reduce.
      return;
    } else {
      printf("warning: conflict between ACC and an action: %d, %d\n",
             ParsingTable[cell], state_dest);
      exit(1);
    }
  }

  // reduce/reduce conflict, use the rule appears first.
  // i.e., the ruleID is smaller, or when negated, is bigger.
  if (ParsingTable[cell] < 0 && state_dest < 0) {
    c = addToConflictArray(row, lookahead,
                           ParsingTable[cell], state_dest);

    if (state_dest > ParsingTable[cell]) ParsingTable[cell] = state_dest;

    if (c != NULL) { c->decision = ParsingTable[cell]; }

    // include r/r conflict for inadequate states.
    if (USE_LALR == TRUE) { addStateNoArray(states_inadequate, row); }

    return;
  }

  // shift/shift conflict.
  if (ParsingTable[cell] > 0 && state_dest > 0) {
    if (SHOW_SS_CONFLICTS == TRUE) {
      printf("warning: shift/shift conflict: %d v.s. %d @ (%d, %s)\n",
             state_dest, ParsingTable[cell], row, lookahead->symbol);
    }
    //exit(1);
    ss_count ++;
    return;
  }

  if (ParsingTable[cell] < 0) {
    reduce = ParsingTable[cell];
    shift = state_dest;
  } else {
    reduce = state_dest;
    shift = ParsingTable[cell];
  }

  tp_s = lookahead->TP;
  if (grammar.rules[(-1) * reduce]->lastTerminal != NULL)
    tp_r = grammar.rules[(-1) * reduce]->lastTerminal->TP;

  if (tp_s == NULL || tp_r == NULL ||
      tp_s->precedence == 0 || tp_r->precedence == 0) {
    c = addToConflictArray(row, lookahead,
                           ParsingTable[cell], state_dest);

    ParsingTable[cell] = shift; // use shift over reduce by default.
    //printf("default using %d\n", ParsingTable[cell]);

    if (c != NULL) { c->decision = ParsingTable[cell]; }

    // include s/r conflicts not handled by precedence/associativity.
    if (TRUE == USE_LALR) { addStateNoArray(states_inadequate, row); }

    return;
  }

  if (tp_r->precedence > tp_s->precedence ||
     (tp_r->precedence == tp_s->precedence && tp_r->assoc == _LEFT)) {
    ParsingTable[cell] = reduce;
    //printf("resolved by using %d\n", ParsingTable[cell]);
    return;
  }

  // include s/r conflicts not handled by precedence/associativity.
  if (TRUE == USE_LALR) { addStateNoArray(states_inadequate, row); }

  ParsingTable[cell] = shift;
  //printf("resolved by using %d\n", ParsingTable[cell]);
}


BOOL isGoalSymbol(SymbolTblNode * snode) {
  if (snode == grammar.goal_symbol->snode) return TRUE;
  return FALSE;
}


void printParsingTableNote() {
  yyprintf("Note: \n");
  yyprintf("1. si means shift and stack state i\n");
  yyprintf("2. ri means reduce by production numbered i\n");
  yyprintf("3. a0 means accept\n");
  yyprintf("4. gi means go to state i\n");
  yyprintf("5. 0 means error\n");
}


/*
 * Print the parsing table after LR(1) parsing machine
 * is generated, by before removing unit productions.
 *
 * Note: The value of variable ParsingTblRows is
 * assigned at the end of function generate_parsing_table().
 *
 * Parsing table: Ref. Aho&Ullman p219.
 */
void printParsingTable() {
  char action;
  int state;
  int row, col;
  int row_size = ParsingTblRows;
  int col_size = ParsingTblCols;
  SymbolTblNode * n;

  yyprintf("\n--Parsing Table--\n");
  yyprintf("State\t");
  writeParsingTableColHeader();

  for (row = 0; row < row_size; row ++) {
    yyprintf2("%d\t", row);
    for (col = 0; col < ParsingTblCols; col ++) {
      n = ParsingTblColHdr[col];
      if (isGoalSymbol(n) == FALSE) {
        getAction(n->type, col, row, & action, & state);
        if (action != '\0') {
    	    yyprintf3("%c%d\t", action, state);
		} else {
			yyprintf3("%c%d\t", ' ', state);
		}
      }
    }
    yyprintf("\n");
  }

  printParsingTableNote();
}


/*
 * Create a state based on the goal production (first
 * production of grammmar) and insert it to states_new.
 *
 * Called by function init() only.
 *
 * Assumption: grammar.rules[0] is the goal production.
 */
void initStartState() {
  int is_compatible= 0;
  State * state0 = createState();
  state0->config_count = 1;
  state0->core_config_count = 1;
  state0->state_no = 0;

  // ruleID = 0, marker = 0, isCoreConfig = 1.
  state0->config[0] = createConfig(0, 0, 1);

  state0->config[0]->owner = state0;
  state0->config[0]->context->context_count = 1;
  hashTbl_insert(strEnd);
  state0->config[0]->context->nContext =
      createSymbolNode(hashTbl_find(strEnd));

  //writeState(state0);

  addState2Collection(states_new, state0);
  addStateToStateArray(states_new_array, state0);

  // insert to state hash table as the side effect of search.
  searchStateHashTbl(state0, & is_compatible);
}



/*
 * Get a list of those states in the ParsingTable whose only
 * actions are a single reduction. Such states are called
 * final states.
 *
 * Use final state default reduction in the hyaccpar parse engine.
 * This significantly decreases the size of the generated parser
 * and overcomes the problem of always need to get the new lookahead
 * token to proceed parsing. Array final_state_list is
 * used in gen_compiler.c, and function writeParsingTblRow() of y.c.
 */
void get_final_state_list() {
  int i, j, row_start, action, new_action;
  SymbolTblNode * n;

  HYY_NEW(final_state_list, int, ParsingTblRows);
  memset((void *) final_state_list, 0, 4 * ParsingTblRows);

#if USE_REM_FINAL_STATE

  if (USE_REMOVE_UNIT_PRODUCTION) {

    for (i = 0; i < ParsingTblRows; i ++) {
      if (isReachableState(i) == FALSE) continue;

      row_start = i * ParsingTblCols;
      action = new_action = 0;
      for (j = 0; j < ParsingTblCols; j ++) {
        n = ParsingTblColHdr[j];

        if (isGoalSymbol(n) == TRUE ||
            isParentSymbol(n) == TRUE) continue;

        new_action = ParsingTable[row_start + j];
        if (new_action > 0 || new_action == CONST_ACC) break;
        if (new_action == 0) continue;
        if (action == 0) action = new_action;
        if (action != new_action) break;
      }
      if (j == ParsingTblCols) final_state_list[i] = action;
    }

  } else {
    for (i = 0; i < ParsingTblRows; i ++) {
      row_start = i * ParsingTblCols;
      action = new_action = 0;
      for (j = 0; j < ParsingTblCols; j ++) {
        new_action = ParsingTable[row_start + j];
        if (new_action > 0 || new_action == CONST_ACC) break;
        if (new_action == 0) continue;
        if (action == 0) action = new_action;
        if (action != new_action) break;
      }
      if (j == ParsingTblCols) final_state_list[i] = action;
    }
  }

#endif
}


void getAvgConfigCount() {
  int i = 0, sum = 0, max = 0, min = 0;
  State * a;
  a = states_new->states_head;
  min = max = a->config_count;
  yyprintf("\n--No. of configurations for each state--\n");
  for (; a != NULL; a = a->next) {
    if ((++ i) % 20 == 1) yyprintf2("\n%d: ", i);
    yyprintf2("%d ", a->config_count);
    sum += a->config_count;
    if (min > a->config_count) min = a->config_count;
    if (max < a->config_count) max = a->config_count;
  }
  yyprintf("\n");
  yyprintf4("Average configurations per state: %.2f (min: %d, max: %d)\n",
            ((double) sum / states_new->state_count), min, max);
}


void show_state_config_info() {
  queue_info(config_queue);
  getAvgConfigCount();
  StateHashTbl_dump();
}

/*
 * print size of different objects. For development use only.
 */
void print_size() {
  printf("size of Grammar: %lu\n", (unsigned long)sizeof(Grammar));
  printf("size of State_collection: %lu\n", (unsigned long)sizeof(State_collection));
  printf("size of State: %lu\n", (unsigned long)sizeof(State));
  printf("size of (State *): %lu\n", (unsigned long)sizeof(State *));
  printf("size of Context: %lu\n", (unsigned long)sizeof(Context));
  printf("size of Production: %lu\n", (unsigned long)sizeof(Production));
  printf("size of Configuration: %lu\n", (unsigned long)sizeof(Configuration));
  printf("size of ParsingTblColHdr: %lu\n", (unsigned long)sizeof(ParsingTblColHdr));
}


void show_conflict_count() {
  // no conflicts.
  if (rs_count == 0 && rr_count == 0 && ss_count == 0) return;

  // no r/r conflicts, and s/r conflicts number is expected.
  if (rs_count == expected_sr_conflict &&
      rr_count == 0 && ss_count == 0) return;

  // otherwise, report conflicts.
  printf("%s: conflicts: ", hyacc_filename);
  if (rs_count > 0) {
    printf("%d shift/reduce", rs_count);
    if (rr_count > 0) printf(", %d reduce/reduce", rr_count);
  } else if (rr_count > 0) {
    printf("%d reduce/reduce", rr_count);
  }
  printf("\n");
  if (ss_count > 0)
    printf("warning: %d shift/shift conflicts\n", ss_count);
}


/* Show statistics of the grammar and it's parsing machine. */
void show_stat() {
  if (USE_VERBOSE == FALSE) return;

  writeStateTransitionList();
  if (SHOW_STATE_CONFIG_COUNT == TRUE) show_state_config_info();
  if (SHOW_ACTUAL_STATE_ARRAY == TRUE) writeActualStateArray();

  yyprintf("\n");
  yyprintf3("%d terminals, %d nonterminals\n",
            grammar.terminal_count, grammar.non_terminal_count);
  yyprintf2("%d grammar rules\n", n_rule);
  if (USE_REMOVE_UNIT_PRODUCTION == TRUE) {
    yyprintf2("%d grammar rules after remove unit productions\n",
              n_rule_opt);
  }
  if (USE_COMBINE_COMPATIBLE_STATES == TRUE) {
    if (USE_LR0 == TRUE) {
      yyprintf2("%d states without optimization\n", n_state_opt1);
    } else {
      yyprintf2("%d states after combine compatible states\n", n_state_opt1);
    }
    if (USE_REMOVE_UNIT_PRODUCTION == TRUE) {
      yyprintf2("%d states after remove unit productions\n", n_state_opt12);
      if (USE_REMOVE_REPEATED_STATES == TRUE)
        yyprintf2("%d states after remove repeated states\n", n_state_opt123);
    }
  } else {
    yyprintf2("%d states without optimization\n", n_state_opt1);
  }

  // conflicts summary.
  yyprintf5("%d shift/reduce conflict%s, %d reduce/reduce conflict%s\n",
            rs_count, (rs_count > 1)?"s":"",
            rr_count, (rr_count > 1)?"s":"");
  if (USE_REMOVE_UNIT_PRODUCTION == TRUE && ss_count > 0) {
    yyprintf3("%d shift/shift conflict%s\n\n",
              ss_count, (ss_count > 1)?"s":"");
  }

}


/////////////////////////////////////////////
// Functions to write state transition list
/////////////////////////////////////////////


/*
 * Used when --lr0 or --lalr is used.
 * Under such situation USE_LR0 or USE_LALR is true.
 */
void writeParsingTblRow_LALR(int state) {
  int col, row_start = state * ParsingTblCols;
  int v;
  SymbolTblNode * s;
  int reduction = 1;
  int only_one_reduction = TRUE;

  // write shift/acc actions.
  // note if a state has acc action, then that's the only action.
  // so don't have to put acc in a separate loop.
  for (col = 0; col < ParsingTblCols; col ++) {
    v = ParsingTable[row_start + col];
    s = ParsingTblColHdr[col];
    if (v > 0) {
      if (USE_REMOVE_UNIT_PRODUCTION == TRUE)
        v = getActualState(v);

      if (ParsingTblColHdr[col]->type == _TERMINAL) {
        yyprintf4("  %s [%d] shift %d\n",
                  s->symbol, s->value, v);
      }
    } else if (v == CONST_ACC) {
      yyprintf3("  %s [%d] Accept\n", s->symbol, s->value);
    } else if (v < 0) {
      if (reduction > 0) { reduction = v; } // first reduction.
      else if (reduction != v) { only_one_reduction = FALSE; }
      // else, is the same as first reduction. do nothing.
    }
  }

  // write reduce actions.
  if (only_one_reduction == TRUE) {
    if (reduction < 0) { yyprintf2("  . reduce (%d)\n", (-1) * reduction); }
    else { yyprintf2("  . error\n", reduction); } // no reduction.
  } else {
    for (col = 0; col < ParsingTblCols; col ++) {
      v = ParsingTable[row_start + col];
      s = ParsingTblColHdr[col];
      if (v < 0 && v != CONST_ACC) {
        yyprintf4("  %s [%d] reduce (%d)\n",
                  s->symbol, s->value, (-1) * v);
      }
    }
  }

  // write goto action.
  yyprintf("\n");
  for (col = 0; col < ParsingTblCols; col ++) {
    v = ParsingTable[row_start + col];
    s = ParsingTblColHdr[col];
    if (v > 0) {
      if (USE_REMOVE_UNIT_PRODUCTION == TRUE)
        v = getActualState(v);

      if (ParsingTblColHdr[col]->type == _NONTERMINAL) {
        yyprintf4("  %s [%d] goto %d\n",
                  s->symbol, s->value, v);
      }
    }
  }

}


void writeParsingTblRow(int state) {
  int col, row_start = state * ParsingTblCols;
  int v;
  SymbolTblNode * s;

  yyprintf("\n");

  if (final_state_list[state] < 0) {
    yyprintf2("  . reduce (%d)\n",
              (-1) * final_state_list[state]);
    return;
  }

  if (USE_LR0 || USE_LALR) {
    writeParsingTblRow_LALR(state);
    return;
  }

  for (col = 0; col < ParsingTblCols; col ++) {
    v = ParsingTable[row_start + col];
    s = ParsingTblColHdr[col];
    if (v > 0) {
      if (USE_REMOVE_UNIT_PRODUCTION == TRUE)
        v = getActualState(v);

      if (ParsingTblColHdr[col]->type == _NONTERMINAL) {
        yyprintf4("  %s [%d] goto %d\n",
                  s->symbol, s->value, v);
      } else {
        yyprintf4("  %s [%d] shift %d\n",
                  s->symbol, s->value, v);
      }
    } else if (v == CONST_ACC) {
      yyprintf3("  %s [%d] Accept\n", s->symbol, s->value);
    } else if (v < 0) {
      yyprintf4("  %s [%d] reduce (%d)\n",
                s->symbol, s->value, (-1) * v);
    }
  }
}


void writeStateInfo(State * s) {
  int i;
  if (s == NULL) {
    //printf("writeState warning: s is NULL\n");
    return;
  }

  writeStateConflictList(s->state_no);

  if (s->PASS_THRU == 1) { yyprintf("[PASS_THRU]\n"); }

  yyprintf2("state %d\n\n", s->state_no);
  yyprintf3("  [config: %d, core config: %d]\n\n",
            s->config_count, s->core_config_count);

  for (i = 0; i < s->config_count; i ++) {
    yyprintf2("  (%d) ", s->config[i]->ruleID);
    writeConfiguration(s->config[i]);
  }

  //writeSuccessorList(s);
  if (TRUE == USE_LALR && TRUE == SHOW_ORIGINATORS) {
    StateList_write(s->parents_list);
  }

  writeParsingTblRow(s->state_no);

  //writeCoreConfiguration(s);
  yyprintf("\n\n");
}


void writeStateCollectionInfo(State_collection * c) {
  State * s;

  yyprintf("\n\n");

  s = c->states_head;
  while (s != NULL) {
    writeStateInfo(s);
    s = s->next;
  }

  if (c->state_count == 0) yyprintf("(empty)\n");
  yyprintf("\n");
}


void writeStateInfoFromParsingTbl() {
  int row;
  for (row = 0; row < ParsingTblRows; row ++) {
    if (isReachableState(row) == TRUE) {
      yyprintf2("\n\nstate %d\n", getActualState(row));
      writeParsingTblRow(row);
    }
  }
  yyprintf("\n\n");
}

/*
 * A list like the list in AT&T yacc and Bison's y.output file.
 */
void writeStateTransitionList() {
  if (SHOW_STATE_TRANSITION_LIST == FALSE) return;

  if (USE_REMOVE_UNIT_PRODUCTION == TRUE) {
    // write from the parsing table.
    writeStateInfoFromParsingTbl();
    writeGrammarConflictList2();
  } else {
    // write from the state objects.
    writeStateCollectionInfo(states_new);
    writeGrammarConflictList();
  }
}





/*
 * LR1 function.
 */
int LR1(int argc, char ** argv) {

  hashTbl_init();

  fp_v = NULL; // for y.output
  if (USE_VERBOSE == TRUE) {
    if ((fp_v = fopen(y_output, "w")) == NULL) {
      printf("cannot open file %s\n", y_output);
      exit(1);
    }
    yyprintf("/* y.output. Generated by HYACC. */\n");
    yyprintf2("/* Input file: %s */\n", hyacc_filename);
  }

  getYaccGrammar(hyacc_filename);

  if (DEBUG_HASH_TBL == TRUE) { hashTbl_dump(); }

  init();
  if (SHOW_GRAMMAR) { writeGrammar(& grammar, TRUE); }

  generate_parsing_machine();

  if (SHOW_PARSING_TBL) printParsingTable();

  if (USE_GRAPHVIZ && ! USE_REMOVE_UNIT_PRODUCTION)
  { gen_graphviz_input(); } /*O0,O1*/

  if (USE_REMOVE_UNIT_PRODUCTION) {
    remove_unit_production();
    if (SHOW_PARSING_TBL)
      yyprintf("\nAFTER REMOVING UNIT PRODUCTION:\n");
    if (SHOW_TOTAL_PARSING_TBL_AFTER_RM_UP) {
      yyprintf("\n--Entire parsing table ");
      yyprintf("after removing unit productions--\n");
      printParsingTable();
    }

    if (SHOW_PARSING_TBL) printFinalParsingTable();
    if (USE_REMOVE_REPEATED_STATES) {
      furtherOptimization();
      if (SHOW_PARSING_TBL) {
        yyprintf("\nAFTER REMOVING REPEATED STATES:\n");
        printFinalParsingTable();
      }
    }
    get_actual_state_no(); /* update actual_state_no[]. */
    if (SHOW_PARSING_TBL) printCondensedFinalParsingTable();
    if (SHOW_GRAMMAR) {
      yyprintf("\n--Grammar after removing unit productions--\n");
      writeGrammar(& grammar, FALSE);
    }
    if (USE_GRAPHVIZ) { gen_graphviz_input2(); } /*O2,O3*/
  }
  get_final_state_list();

  if (USE_GENERATE_COMPILER) generate_compiler(hyacc_filename);

  show_stat();
  show_conflict_count();

  if (USE_VERBOSE == TRUE) fclose(fp_v);
  //free_vars(); // let system take care of it.
  return 0;
}


int LR0(int argc, char ** argv) {

  ///USE_COMBINE_COMPATIBLE_STATES = FALSE; ///
  hashTbl_init();

  fp_v = NULL; // for y.output
  if (USE_VERBOSE == TRUE) {
    if ((fp_v = fopen(y_output, "w")) == NULL) {
      printf("cannot open file %s\n", y_output);
      exit(1);
    }
    yyprintf("/* y.output. Generated by HYACC. */\n");
    yyprintf2("/* Input file: %s */\n", hyacc_filename);
  }

  getYaccGrammar(hyacc_filename);

  if (DEBUG_HASH_TBL == TRUE) { hashTbl_dump(); }

  init();
  if (SHOW_GRAMMAR) { writeGrammar(& grammar, TRUE); }

  generate_LR0_parsing_machine(); //

  if (USE_LALR == TRUE) {
    lane_tracing();
    //outputParsingTable_LALR();
  }

  if (SHOW_PARSING_TBL) printParsingTable();

  if (USE_GRAPHVIZ && ! USE_REMOVE_UNIT_PRODUCTION)
  { gen_graphviz_input(); } /*O0,O1*/

  if (USE_REMOVE_UNIT_PRODUCTION) {
    remove_unit_production();
    if (SHOW_PARSING_TBL)
      yyprintf("\nAFTER REMOVING UNIT PRODUCTION:\n");
    if (SHOW_TOTAL_PARSING_TBL_AFTER_RM_UP) {
      yyprintf("\n--Entire parsing table ");
      yyprintf("after removing unit productions--\n");
      printParsingTable();
    }

    if (SHOW_PARSING_TBL) printFinalParsingTable();
    if (USE_REMOVE_REPEATED_STATES) {
      furtherOptimization();
      if (SHOW_PARSING_TBL) {
        yyprintf("\nAFTER REMOVING REPEATED STATES:\n");
        printFinalParsingTable();
      }
    }
    get_actual_state_no(); /* update actual_state_no[]. */
    if (SHOW_PARSING_TBL) printCondensedFinalParsingTable();
    if (SHOW_GRAMMAR) {
      yyprintf("\n--Grammar after removing unit productions--\n");
      writeGrammar(& grammar, FALSE);
    }
    if (USE_GRAPHVIZ) { gen_graphviz_input2(); } /*O2,O3*/
  }
  get_final_state_list();

  if (USE_GENERATE_COMPILER) generate_compiler(hyacc_filename);

  show_stat();
  show_conflict_count();

  if (USE_VERBOSE == TRUE) fclose(fp_v);
  //free_vars(); // let system take care of it.

  return 0;
}


/*
 * main function.
 */
int main(int argc, char ** argv) {
  int infile_index;
  DEBUG_EXPAND_ARRAY = 0;

  infile_index = get_options(argc, argv);
  hyacc_filename = argv[infile_index];

  if (USE_LR0 == TRUE) {
    LR0(argc, argv);
  } else {
    LR1(argc, argv);
  }

  return 0;
}

