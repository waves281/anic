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
 * LR(0) functions.
 *
 * @Author: Xin Chen
 * @Created on: 2/24/2008
 * @Last modified: 3/24/2009
 * @Copyright (C) 2008, 2009
 */

#include "y.h"


static void propagateOriginatorChange(State * s);


void addSuccessorConfigToState_LR0(State * s, int ruleID) {
  Configuration * c;

  if (s->config_count >= s->config_max_count - 1) {
    s->config_max_count *= 2;
    HYY_EXPAND(s->config, Configuration *, s->config_max_count);
  }

  // marker = 0, isCoreConfig = 0.
  c = createConfig(ruleID, 0, 0);
  c->owner = s;

  s->config[s->config_count] = c;
  s->config_count ++;  
}


/*
 * Assumption: public variable config_queue contains 
 * the configurations to be processed.
 */
void getConfigSuccessors_LR0(State * s) {
  RuleIDNode * r;
  SymbolTblNode * scanned_symbol = NULL;
  Configuration * config;
  int index;

  while (queue_count(config_queue) > 0) {
    config = s->config[queue_pop(config_queue)];

    if (config->marker >= 0 &&
        config->marker < grammar.rules[config->ruleID]->RHS_count) {
      scanned_symbol = getScannedSymbol(config);

      if (isNonTerminal(scanned_symbol) == TRUE) {

        for (r = scanned_symbol->ruleIDList; r != NULL; r = r->next) {
          // Grammar.rules[r->ruleID] starts with this scanned symbol.
  
          // If not an existing config, add to state s.
          index = isCompatibleSuccessorConfig(s, r->ruleID);

          if (index == -1) { // new config.
            addSuccessorConfigToState_LR0(s, r->ruleID);
            queue_push(config_queue, s->config_count - 1);
            index = s->config_count - 1;
          } // else is an existing old config, do nothing.

        } // end for
      } // else, is a terminal, stop.
    }  // end if config-marker >= 0 ...
  } // end of while
}


void getClosure_LR0(State * s) {
  int i;
  //queue_clear(config_queue);
  for (i = 0; i < s->config_count; i ++) {
    queue_push(config_queue, i);
  }
  getConfigSuccessors_LR0(s);
}


/*
 * For LR(0). Insert a/r actions to the ENTIRE row.
 */
void insertReductionToParsingTable_LR0(
       Configuration * c, int state_no) {
  int col;
  SymbolTblNode * n;
  int max_col = grammar.terminal_count + 1;

  if (grammar.rules[c->ruleID]->nLHS->snode ==
      grammar.goal_symbol->snode) { //accept, action = "a";
    // note this should happen only if the end is $end.
    insertAction(hashTbl_find(strEnd), state_no, CONST_ACC);
  } else { //reduct, action = "r";
    for (col = 0; col < max_col; col ++) {
      n = ParsingTblColHdr[col];
      insertAction(n, state_no, (-1) * c->ruleID);
    }
  }
}


/*
 * if context set is empty, fill all cells in the ENTIRE row;
 * otherwise, fill cells with lookaheads in the context set.
 */
void insertReductionToParsingTable_LALR(
       Configuration * c, int state_no) {
  SymbolNode * a;
  int col;
  SymbolTblNode * n;
  int max_col = grammar.terminal_count + 1;

  if (grammar.rules[c->ruleID]->nLHS->snode ==
      grammar.goal_symbol->snode) { //accept, action = "a";
    // note this should happen only if the end is $end.
    insertAction(hashTbl_find(strEnd), state_no, CONST_ACC);
  } else { //reduct, action = "r";
    a = c->context->nContext;
    if (a != NULL) {
      for (a = c->context->nContext; a != NULL; a = a->next) {
        insertAction(a->snode, state_no, (-1) * c->ruleID);
      }
    } else { 
      for (col = 0; col < max_col; col ++) {
        n = ParsingTblColHdr[col];
        insertAction(n, state_no, (-1) * c->ruleID);
      }
    }
  }
}


/*
 * For each of the new temp states,
 *   If it is not one of the existing states in 
 *     states_new, then add it to states_new.
 *   Also always add the transition to parsing table.
 *
 * Note:
 * The "is_compatible" variable is of NO use here,
 * since configurations don't have contexts. So such
 * states are always the "same", but not compatible.
 */
void addTransitionStates2New_LR0(
       State_collection * coll, State * src_state) {
  State * os, * next, * s;
  s = coll->states_head;

  while (s != NULL) {
    next = s->next;

    // searchSameStateHashTbl() checks for SAME (not compatible) states.
    if ((os = searchSameStateHashTbl(s)) == NULL) { 
      insert_state_to_PM(s);

      // Add this new state as successor to src_state.
      addSuccessor(src_state, s);

    } else { // same with an existing state.
      addSuccessor(src_state, os);
      destroyState(s); // existing or compatible. No use.
    }

    s = next;
  } // end of while.
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
void transition_LR0(State * s) {
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

  if (coll->state_count > 0) {
    addTransitionStates2New_LR0(coll, s);
  }
}


/*
 * First fill acc, s, g.
 * Then fill r. r is filled to terminal symbols only. And if
 * a cell is already a/s/g, don't fill this r.
 */
static void outputParsingTableRow_LR0(State * s) {
  int i, ct;
  Configuration * c;
  State * t;

  ct = s->config_count;

  // insert a/r actions.
  for (i = 0; i < ct; i ++) {
    c = s->config[i];

    if (isFinalConfiguration(c) == TRUE || 
        isEmptyProduction(c) == TRUE) {
      insertReductionToParsingTable_LR0(c, s->state_no);
    }
  }

  // insert s/g actions.
  ct = s->successor_count;
  for (i = 0; i < ct; i ++) {
    t = s->successor_list[i];
    insertAction(t->trans_symbol->snode, s->state_no, t->state_no);
  }
}


static void outputParsingTableRow_LALR(State * s) {
  int i, ct;
  Configuration * c;
  State * t;

  ct = s->config_count;

  // insert a/r actions.
  for (i = 0; i < ct; i ++) {
    c = s->config[i];
    //printf("%d.%d\n", s->state_no, c->ruleID);

    if (isFinalConfiguration(c) == TRUE ||
        isEmptyProduction(c) == TRUE) {
      insertReductionToParsingTable_LALR(c, s->state_no);
    }
  }

  // insert s/g actions.
  ct = s->successor_count;
  for (i = 0; i < ct; i ++) {
    t = s->successor_list[i];
    insertAction(t->trans_symbol->snode, s->state_no, t->state_no);
  }
}


/*
 * Output parsing table from the parsing machine.
 * This is different from before. The previous implementation
 * for LR(1) is this is done when transitioning states. Now for
 * LR(0) since all default actions are reduce, the previous 
 * method does not work very well.
 */
void outputParsingTable_LR0() {
  State * s;
  int i, cols, rows;
  rows = states_new_array->state_count;
  cols = n_symbol + 1;

  // expand size of parsing table array if needed.
  if (rows >= PARSING_TABLE_SIZE) {
    expandParsingTable();
  }

  memset((void *) ParsingTable, 0, cols * rows * 4);

  for (i = 0; i < rows; i ++) {
    s = states_new_array->state_list[i];
    outputParsingTableRow_LR0(s); 
  }
}


/*
 *
 */
void outputParsingTable_LALR() {
  State * s;
  int i, cols, rows;
  rows = states_new_array->state_count;
  cols = n_symbol + 1;

  // expand size of parsing table array if needed.
  if (rows >= PARSING_TABLE_SIZE) {
    expandParsingTable();
  }

  memset((void *) ParsingTable, 0, cols * rows * 4);

  for (i = 0; i < rows; i ++) {
    s = states_new_array->state_list[i];
    outputParsingTableRow_LALR(s);
  }
}


void updateParsingTable_LR0() {
  ParsingTblRows = states_new->state_count;
  n_state_opt1 = states_new->state_count;

  // this fills the conflict list, so is need for lalr processing.
  outputParsingTable_LR0();
}


void generate_LR0_parsing_machine() {
  State * new_state = states_new->states_head;

  if (DEBUG_GEN_PARSING_MACHINE == TRUE) {
    yyprintf("\n\n--generate parsing machine--\n");
  }

  while (new_state != NULL) {
    if (DEBUG_GEN_PARSING_MACHINE == TRUE) {
      yyprintf3("%d states, current state is %d\n", 
               states_new->state_count, new_state->state_no);
    }

    getClosure_LR0(new_state); // get closure of this state.

    // get successor states and add them to states_new.
    transition_LR0(new_state);

    new_state = new_state->next; // point to next unprocessed state.
  }

  updateParsingTable_LR0();

  //show_conflict_count();
}

