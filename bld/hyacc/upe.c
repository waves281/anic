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
 * upe.c
 *
 * For unit production elimination and further remove redundant states.
 * This is separated out from y.c to make y.c size smaller.
 *
 * @Author: Xin Chen
 * @Date started: March 9, 2007
 * @Last modified: March 9, 2007
 * @Copyright (C) 2007, 2008
 */

#include "y.h"
#include "mrt.h"


/*
 * State numbers are added to the dynamic array cmbined_states
 * in function createNewUPSState(), where the size of
 * combined_states is given by old_states_count.
 * combined_states is dynamically allocated there and does not
 * need expansion later.
 */
typedef struct {
  int * combined_states;
  int combined_states_count;
  int state_no; // state_no of this new state.
} UnitProdState;

/* assume 500 new states maximal. */
static int UPS_SIZE = 64;
#define UPS_MAX_SIZE 65536   /* 2^16 */
static UnitProdState * ups;
static int ups_count = 0;


/*
 * sort an integer array increasingly.
 * Uses insertion sort.
 */
void sort_int(int array[], int array_len) {
  register int i, j, tmp;
  for (i = 1; i < array_len; i ++) {
    tmp = array[i];
    for (j = i; j > 0 && array[j - 1] > tmp; j --) {
      array[j] = array[j - 1];
    }
    array[j] = tmp;
  }
}


void print_int_array(int a[], int count) {
  int i;
  yyprintf2("count = %d\n", count);
  for (i = 0; i < count; i ++) {
    if (i > 0) yyprintf(", ");
    yyprintf2("%d", a[i]);
  }
  yyprintf("\n");
}


/*
 * Called by function remove_unit_production_step1and2() only.
 * 
 * This function does this:
 *
 * For given state s and leaf x, we have
 * non-terminal symbols y such that y => x. 
 * if goto actions exist for some y's, then
 * get target state numbers of all y's goto actions,
 * as well as the target state number of x's shift/goto
 * actions if any.
 *
 * The number of such target states numbers is unitProdCount.
 * Note that it's bounded by non_terminal_count + 1, where 1
 * is for the leaf itself. This is because in the extreme
 * case all non terminals y => x.
 *
 * BASICALLY, unitProdCount == 0 or unitProdCount >= 2!
 * It is NOT possible that unitProdCount == 1.
 * This is because if there is a y successor of state s,
 * then y => x means there will also be x successors for s.
 *
 * unitProdCount >= 2. This also suggests that any
 * new state is not one from 0 to total states count - 1.
 *
 * These target states are to be combined in the next step.
 */
void getUnitProdShift(int state, SymbolTblNode * leaf,
       MRParents * parents, 
       int unitProdDestStates[], int * unitProdCount) {

  //printf("getUnitProdShift for '%s'\n", leaf);
  int i;
  char action;
  int state_dest;
  SymbolTblNode * n;
  (* unitProdCount) = 0;

  for (i = 0; i < parents->count; i ++) {
    n = parents->parents[i]->snode;
    getAction(n->type, getCol(n), state, & action, & state_dest);
    //printf("%c, %d\n", action, state_dest);
    if (action == 'g') {
      unitProdDestStates[* unitProdCount] = state_dest;
      (* unitProdCount) ++;
    }
  }

  // Note: leaf itself can be a non-terminal.
  // so action can be g too.
  if ((* unitProdCount) > 0) {
    getAction(leaf->type, getCol(leaf), state, & action, & state_dest);
    if (action == 's' || action == 'g') { 
      unitProdDestStates[* unitProdCount] = state_dest;
      (* unitProdCount) ++;
    }
  }

  sort_int(unitProdDestStates, * unitProdCount);
}


void writeUnitProdShift(int state, SymbolTblNode * leaf,
     int unitProdDestStates[], int unitProdCount, 
     int new_ups_state) {
  int i;
  yyprintf3("state %d, leaf '%s' -", state, leaf->symbol);
  yyprintf2(" combine these states to new state %d:\n",
            new_ups_state);
  for (i = 0; i < unitProdCount; i ++) {
    if (i > 0) yyprintf(", ");
    yyprintf2("%d", unitProdDestStates[i]);
  }
  yyprintf("\n");
}


void checkUPSSize() {
  if (ups_count < UPS_SIZE) return;

  //yyprintf2("checkUPSSize: max size %d reached\n", UPS_SIZE);
  //writeUPSStates(); exit(0);

  if (2 * UPS_SIZE >= UPS_MAX_SIZE) {
    printf("checkUPSSize: max size %d reached\n", UPS_MAX_SIZE);
    printf("Too many UPS states. \n");
    //writeUPSStates();
    exit(0);
  }

  UPS_SIZE *= 2;
  HYY_EXPAND(ups, UnitProdState, UPS_SIZE);

  //printf("expand ups size to %d\n", UPS_SIZE);
  //yyprintf2("checkUPSSize: expand ups size to %d\n", UPS_SIZE);
}


void writeUPSState(UnitProdState * ups) {
  yyprintf2("State no: %d. Is combination of these states:\n", 
          ups->state_no);
  print_int_array(ups->combined_states, 
                  ups->combined_states_count);
}


void writeUPSStates() {
  int i;
  yyprintf("==New states for unit production removal");
  yyprintf2(" (total %d):\n", ups_count);
  for (i = 0; i < ups_count; i ++) {
    writeUPSState(& ups[i]);
  }
}


void createNewUPSState(int new_state, int old_states[], 
                       int old_states_count) {
  int i;
  UnitProdState * new_ups;

  checkUPSSize(); // expand if necessary.

  new_ups = & ups[ups_count];
  new_ups->state_no = new_state;
  new_ups->combined_states_count = old_states_count;
  
  // allocate dynamic array int * combined_states.
  HYY_NEW(new_ups->combined_states, int, 
          new_ups->combined_states_count);

  // copy state numbers.
  for (i = 0; i < old_states_count; i ++)
    new_ups->combined_states[i] = old_states[i];

  ups_count ++;

  //printf("ups_count = %d\n", ups_count);
  //printf("new UPS state %d, is combination of states: \n", ups_count - 1);
  //printIntArray(old_states, old_states_count);
}


BOOL isSameUPSState(int a[], int b[], int count) {
  int i;
  for (i = 0; i < count; i ++) {
    if (a[i] != b[i]) return FALSE;
  }
  return TRUE;
}


/*
 * find the new state which combines states in a[].
 * Return: 
 *   state no. if found, or -1 if not found.
 */
int getUPSState(int a[], int count) {
  int i;
  if (ups_count == 0) return -1;

  for (i = 0; i < ups_count; i ++) {
    if (count == ups[i].combined_states_count) {
      if (isSameUPSState(ups[i].combined_states, 
           a, count) == TRUE) return ups[i].state_no;
    }
  }
  return -1;
}


BOOL isUnitProduction(int rule_no) {
  if (rule_no >= grammar.rule_count) {
    printf("isUnitProduction error: ");
    printf("array index (%d) out of bound\n", rule_no);
    exit(0);
  }
  if (grammar.rules[rule_no]->RHS_count == 1 && 
      strlen(grammar.rules[rule_no]->nRHS_head->snode->symbol) > 0) 
    return TRUE;

  return FALSE;
}


/*
 * Called by function insertActionsOfCombinedStates().
 */
void insertActionOfSymbol(SymbolTblNode * symbol, int new_state,
       int old_state_index, int old_states[]) {

  char action = 0;
  int state_dest = 0;

  getAction(symbol->type, getCol(symbol), old_states[old_state_index],
            & action, & state_dest);

  if (action == 0) return;

  //printf("insert action %c to dest_state %d for new state \
  //%d on symbol %s\n", action, state_dest, new_state, symbol->symbol);

  if (action == 'a') {
    insertAction(symbol, new_state, CONST_ACC);
  } else if (action == 's' || action == 'g') {
    insertAction(symbol, new_state, state_dest);
  } else if (action == 'r') {
    if (isUnitProduction(state_dest) == FALSE) {
      insertAction(symbol, new_state, (-1) * state_dest);
    }
  }
}



/*
 * For a combined state, get actions from each of the states
 * from which the combined state is made of, and copy these
 * actions to the combined state.
 *
 * Called by remove_unit_production_step1and2() only.
 */
void insertActionsOfCombinedStates(int new_state, int src_state,
   int old_states[], int old_states_count) {

  //printf("Source state: %d. ", src_state);
  //printf("Combine these states into state %d:\n", new_state);
  //print_int_array(old_states, old_states_count);

  SymbolNode * a;
  Grammar * g = & grammar;
  int i;

  // copy actions of old_states to new_state.

  for (i = 0; i < old_states_count; i ++) {
    // Copy action of end marker strEnd.
    insertActionOfSymbol(hashTbl_find(strEnd), new_state, i, old_states);

    // copy actions of terminals.
    for (a = g->terminal_list; a != NULL; a = a->next) {
      insertActionOfSymbol(a->snode, new_state, i,
                           old_states);
    }

    // copy actions of non_terminals.
    for (a = g->non_terminal_list; a != NULL; a = a->next) {
      insertActionOfSymbol(a->snode, new_state, i,
                           old_states);
    }
  } // end for

}


/*
 * step 3. Delete transitions wrt. LHS of unit productions.
 * equivalent to remove all non-terminal goto actions.
 * new states don't have these, so just remove those 
 * of the old state.
 *
 * Actually, THIS IS NOT EVEN NECESSARY if all is needed
 * is stdout output. This is because after getting all
 * parent symbols we can ignore to output them in the
 * step writeFinalParsingTable!
 */
void remove_unit_production_step3() {
  SymbolNode * a;
  int i;

  for (i = 0; i < ParsingTblRows; i ++) {
    for (a = grammar.non_terminal_list; a != NULL; a = a->next) {
      // use "" as action and 0 as dest state clears it.
      // Only those non-terminals y => x are cleared.
      if (isParentSymbol(a->snode) == TRUE) {
        updateAction(getCol(a->snode), i, 0);
      }
    }
  }
}


/*
 * Check if n is in integer array a.
 * Returns TRUE if exists, FALSE if not exists.
 */
BOOL inIntArray(int n, int a[], int a_size) {
  int i;
  for (i = 0; i < a_size; i ++) {
    if (n == a[i]) return TRUE;
  }
  return FALSE;
}


/*
 * Note that action 'g' applies to non-terminals only.
 * But include it does not affect the cases for strEnd
 * and terminals, which have only action 's'. So just
 * include checking for 'g' for all three situations.
 */
void getReachableStatesForSymbol(char * symbol, int cur_state,
  int states_reachable[], int * states_count) {
  char action;
  int state_dest;

  SymbolTblNode * n = hashTbl_find(symbol);

  getAction(n->type, getCol(n), cur_state, & action, & state_dest);
  if ((action == 's' || action == 'g') && 
      inIntArray(state_dest, states_reachable,
                       * states_count) == FALSE) {
    states_reachable[* states_count] = state_dest;
    (* states_count) ++;
    getReachableStates(state_dest, states_reachable,
                       states_count);
  }
}


/* 
 * In the parsing table, get states that are reachable
 * from cur_state, and stores the result in array
 * states_reachable[].
 */
void getReachableStates(int cur_state, int states_reachable[],
    int * states_count) {
  SymbolNode * a;

  getReachableStatesForSymbol(strEnd, 
    cur_state, states_reachable, states_count);

  for (a = grammar.terminal_list; a != NULL; a = a->next) {
    getReachableStatesForSymbol(a->snode->symbol,
      cur_state, states_reachable, states_count);
  }

  for (a = grammar.non_terminal_list; a != NULL; a = a->next) {
    if (isParentSymbol(a->snode) == FALSE) {
      getReachableStatesForSymbol(a->snode->symbol,
        cur_state, states_reachable, states_count);
    }
  }
}


void writeParsingTableColHeader() {
  int i;
  for (i = 0; i < ParsingTblCols; i ++) {
    if (isGoalSymbol(ParsingTblColHdr[i]) == FALSE) {
      yyprintf2("%s\t", ParsingTblColHdr[i]->symbol);
    }
  }
  yyprintf("\n");
}


/*
 * get final parsing table column headers.
 * + 1 for end marker strEnd.
 */
void getF_ParsingTblColHdr() {
  SymbolNode * a, * tail;

  tail = F_ParsingTblColHdr = createSymbolNode(hashTbl_find(strEnd));
  F_ParsingTblCols = 1;

  for (a = grammar.terminal_list; a != NULL; a = a->next) {
    tail->next = createSymbolNode(a->snode);
    tail = tail->next;
    F_ParsingTblCols ++;
  }

  for (a = grammar.non_terminal_list; a != NULL; a = a->next) {
    if (isParentSymbol(a->snode) == FALSE &&
        isGoalSymbol(a->snode) == FALSE) {
      tail->next = createSymbolNode(a->snode);
      tail = tail->next;
      F_ParsingTblCols ++;
    }
  }

}


void writeFinalParsingTableColHeader() {
  SymbolNode * a;
  for (a = F_ParsingTblColHdr; a != NULL; a = a->next) {
    yyprintf2("%s\t", a->snode->symbol);
  }
  yyprintf("\n");
}


/*
 * step 4. delete all states at this stage
 * not reachable from state 0.
 * Does a recursive traveral starting from 
 * state 0 to determine reachable state.
 *
 * int * states_reachable and int states_reachable_count
 * are defined in y.h.
 */
void remove_unit_production_step4() {
  states_reachable = (int *) malloc(sizeof(int) * ParsingTblRows);
  if (states_reachable == NULL) {
    YYERR_EXIT("remove_unit_productino_step4 error: out of memory\n");
  }
  states_reachable_count = 0;
  getReachableStates(0, states_reachable, 
                     & states_reachable_count);
  sort_int(states_reachable, states_reachable_count);

  if (DEBUG_REMOVE_UP_STEP_4) {
    yyprintf("\n--remove_unit_production_step4--\n");
    yyprintf("states reachable from state 0:\n");
    print_int_array(states_reachable, states_reachable_count);
  }

  getF_ParsingTblColHdr();
}


BOOL isReachableState(int state) {
  if (state == 0 || inIntArray(state, states_reachable,
                    states_reachable_count) == TRUE) {
    return TRUE;
  }
  return FALSE;
}


/*
 * The parsing table array does not change,
 * only change the output entries.
 */
void printFinalParsingTable() {
  SymbolTblNode * n;
  int row, col;
  int col_size = ParsingTblCols;
  int row_size = ParsingTblRows;
  char action;
  int state;

  yyprintf("\n--Parsing Table--\n");
  yyprintf("State\t");
  writeFinalParsingTableColHeader();

  for (row = 0; row < row_size; row ++) {
    if (isReachableState(row) == TRUE) {
      yyprintf2("%d\t", row);
      for (col = 0; col < ParsingTblCols; col ++) {
        n = ParsingTblColHdr[col];
        if (isGoalSymbol(n) == FALSE && isParentSymbol(n) == FALSE) {
          getAction(n->type, col, row, & action, & state);
          yyprintf3("%c%d\t", action, state);
        }
      }

      yyprintf("\n");
    } // end if
    //else { printf("state %d: not reachable.\n", row); }
  } // end for

  printParsingTableNote();
}


void printIntArray(int a[], int a_ct) {
  int i;
  for (i = 0; i < a_ct; i ++) {
    yyprintf2("%d", a[i]);
    if (i < a_ct - 1) yyprintf(", ");
    if ( (i > 0) && (i % 9 == 0) ) yyprintf("\n");
  }
  yyprintf("\n");
}


/*
 * There are some holes in the parsing table.
 * Now do a first pass to record the "virtual" state number
 * and the "actual" state number correspondence relationship,
 * then when print the parsing table, replace those
 * "virtual" state numbers with corresponding "actual"
 * state numbers.
 *
 * Store the correspondence relationship in array
 * actual_state_no[].
 * actual_state_no[2 * i] is virtual state,
 * actual_state_no[2 * i + 1} is actual state.
 * 
 * The following two are defined in y.h:
 * int actual_state_no[2 * STATE_COLLECTION_SIZE];
 * int actual_state_no_ct;
 */

void get_actual_state_no() {
  int row, i;
  int row_size = ParsingTblRows;
  actual_state_no = (int *) malloc(sizeof(int) * 2 * row_size);
  if (actual_state_no == NULL) {
    YYERR_EXIT("get_actual_state_no error: out of memory\n");
  }

  i = 0;
  actual_state_no_ct = 0;
  for (row = 0; row < row_size; row ++) {
    if (row == 0 || inIntArray(row, states_reachable,
                    states_reachable_count) == TRUE) {
      actual_state_no[actual_state_no_ct ++] = row;
      actual_state_no[actual_state_no_ct ++] = i;
      i ++;
    }
  }
}


int getActualState(int virtual_state) {
  int i;
  for (i = 0; i < actual_state_no_ct; i += 2) {
    if (virtual_state == actual_state_no[i])
      return actual_state_no[i + 1];
  }
  return -1; // this should not happen.
}


void writeActualStateArray() {
  int i;
  
  if (USE_REMOVE_UNIT_PRODUCTION == FALSE) return;

  yyprintf("\n\n--actual state array [actual, pseudo]--\n");
  for (i = 0; i < actual_state_no_ct; i += 2) {
    if (i > 0 && i % 5 == 0) yyprintf("\n");
    yyprintf3("[%d, %d] ", actual_state_no[i], actual_state_no[i+1]);
  }
  yyprintf("\n\n");
}


/*
 * If an action is 's' or 'g', change its target state number
 * from virtual to actual.
 */
void printCondensedFinalParsingTable() {
  SymbolTblNode * n;
  char action;
  int row, col, i, state_no;
  int col_size = ParsingTblCols;
  // value assigned at the end of generate_parsing_table().
  int row_size = ParsingTblRows;

  yyprintf("\n--Final Parsing Table--\n");
  yyprintf("State\t");
  writeFinalParsingTableColHeader();

  i = 0;
  for (row = 0; row < row_size; row ++) {
    if (isReachableState(row) == TRUE) {
      yyprintf2("%d\t", i);
      for (col = 0; col < ParsingTblCols; col ++) {
        n = ParsingTblColHdr[col];
        if (isGoalSymbol(n) == FALSE && isParentSymbol(n) == FALSE) {
          getAction(n->type, col, row, & action, & state_no);
          if (action == 's' || action == 'g')
            state_no = getActualState(state_no);
          yyprintf3("%c%d\t", action, state_no);
        }
      }
      
      i ++;
      yyprintf("\n");
    } // end if
  } // end for

  printParsingTableNote();
}


/*
 * This actually is not needed too (see step 3).
 * Because all we care in x -> a b c
 * is how many symbols we have on the RHS.
 */
void remove_unit_production_step5() {
  int i, index;
  int ct = getGrammarRuleCount();
  
  for (i = 0; i < ct; i ++) {
    index = getIndexInMRParents(grammar.rules[i]->nLHS->snode,
              all_parents);
    if (index >= 0) {
      freeSymbolNode(grammar.rules[i]->nLHS);
      grammar.rules[i]->nLHS = createSymbolNode(
        MRLeaves[leafIndexForParent[index]]->symbol->snode);
    }
  }
}


void remove_unit_production_step1and2() {
  int state, i, unitProdCount, ups_state;
  SymbolTblNode * leaf;
  MRParents * parents; 
  MRParents ** leaf_parents;
  int * unitProdDestStates;

  // as discussed in the function comments of getUnitProdShift(),
  // unitProdDestStates array is bounded by number of non_terminals + 1.
  HYY_NEW(unitProdDestStates, int, grammar.non_terminal_count + 1);
  HYY_NEW(ups, UnitProdState, UPS_SIZE);
  ups_count = 0;
  HYY_NEW(leaf_parents, MRParents *, MRLeaves_count);

  // pre-calculate all parents for each leaf.
  for (i = 0; i < MRLeaves_count; i ++) {
    leaf_parents[i] = createMRParents();
    getParentsForMRLeaf(i, leaf_parents[i]);
    //writeMRParents(MRLeaves[i], leaf_parents[i]);
  }

  if (DEBUG_REMOVE_UP_STEP_1_2) {
    yyprintf("\n--remove_unit_production_step1and2--\n");
    yyprintf("--writeUnitProdShift--\n");
  }

  // now, steps 1 and 2.
  for (state = 0; state < ParsingTblRows; state ++) {
    for (i = 0; i < MRLeaves_count; i ++) {
      leaf = MRLeaves[i]->symbol->snode;
      //printf("state %d, checking leaf %s\n", state, leaf->symbol);
      parents = leaf_parents[i];

      getUnitProdShift(state, leaf, parents,
              unitProdDestStates, & unitProdCount);

      if (unitProdCount > 0) { // unitProdCount >= 2
        ups_state = 
          getUPSState(unitProdDestStates, unitProdCount);
        if (ups_state == -1) {
          ups_state = ParsingTblRows;
          ParsingTblRows ++;
          if (ParsingTblRows == PARSING_TABLE_SIZE) {
            //yyprintf("remove_unit_production message: ");
            //yyprintf("Parsing Table size reached\n");
            expandParsingTable();
          }
          createNewUPSState(ups_state, 
                            unitProdDestStates, unitProdCount);

          // Combine actions of states into state ups_state.
          // Do this only if this combined state does not exist yet.
          insertActionsOfCombinedStates(ups_state, state,
            unitProdDestStates, unitProdCount);

        } // end if (ups_state != -1)

        // Update the link from src_state to leaf transition state
        updateAction(getCol(leaf), state, ups_state); // shift.

        if (DEBUG_REMOVE_UP_STEP_1_2) {
          writeUnitProdShift(state, leaf,
                       unitProdDestStates, unitProdCount, ups_state);
          //yyprintf2(" => new ups_state: %d\n", ups_state);
        }
      } // end if (unitProdCount > 0)
    }
  }
  if (DEBUG_REMOVE_UP_STEP_1_2) {
    yyprintf("--after remove_unit_production_step1and2(), ");
    yyprintf2("total states: %d--\n", ParsingTblRows);
  }

  for (i = 0; i < MRLeaves_count; i ++)
    destroyMRParents(leaf_parents[i]);

  free(unitProdDestStates);
}


////////////////////////////////////////////////////////
// Dr. Pager, Acta Informatica 9, 31-59 (1977), page 38.
////////////////////////////////////////////////////////
void remove_unit_production() {
  buildMultirootedTree();

  remove_unit_production_step1and2();
  remove_unit_production_step3();
  remove_unit_production_step4();
  remove_unit_production_step5();

  n_state_opt12 = states_reachable_count + 1;
}


/////////////////////////////////////////////////////
// Functions for removing unit productions. End.
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Functions for further optimization. Start.
/////////////////////////////////////////////////////

/*
 * Determine if rows i and j in the parsing table are the same.
 */
BOOL isEqualRow(int i, int j) {
  int col;
  char action_i, action_j;
  int state_dest_i, state_dest_j;
  SymbolTblNode * n;

  for (col = 0; col < ParsingTblCols; col ++) {
    n = ParsingTblColHdr[col];
    getAction(n->type, getCol(n), i, & action_i, & state_dest_i);
    getAction(n->type, getCol(n), j, & action_j, & state_dest_j);
    if (action_i != action_j || state_dest_i != state_dest_j)
      return FALSE;
  }

  return TRUE;
}


/*
 * In parsing table row, replace entries whose
 * target state is old_state by new_state.
 */
void updateRepeatedRow(int new_state, int old_state, int row) {
  SymbolNode * a;
  SymbolTblNode * n;
  char action;
  int state_dest;
  Grammar * g = & grammar;
  //printf("In row %d, replace %d by %d\n", row, old_state, new_state);

  // for end marker column strEnd
  n = hashTbl_find(strEnd);
  getAction(n->type, getCol(n), row, & action, & state_dest);
  if (state_dest == old_state)
    updateAction(getCol(hashTbl_find(strEnd)), row, new_state);

  // for terminal columns
  for (a = g->terminal_list; a != NULL; a = a->next) {
    n = a->snode;
    getAction(n->type, getCol(n), row, & action, & state_dest);
    if (state_dest == old_state) 
      updateAction(getCol(a->snode), row, new_state);
  }

  // for non-terminal columns
  for (a = g->non_terminal_list; a != NULL; a = a->next) {
    n = a->snode;
    getAction(n->type, getCol(n), row, & action, & state_dest);
    if (state_dest == old_state)
      updateAction(getCol(a->snode), row, new_state);
  }
}


/*
 * Go through the entire parsing table (only those reachable
 * states, to save time), replace those entries
 * whose target state is old_state by new_state.
 */
void updateRepeatedRows(int new_state, int old_state) {
  int i;

  updateRepeatedRow(new_state, old_state, 0); // row 0.

  for (i = 0; i < states_reachable_count; i ++) 
    updateRepeatedRow(new_state, old_state, states_reachable[i]);
}


/*
 * Remove state i from the array of reachable states.
 */
void removeReachableState(int i) {
  for (; i < states_reachable_count - 1; i ++) {
    states_reachable[i] = states_reachable[i + 1];
  }
  states_reachable_count --;
}


/*
 * Remove the repeated states in the parsing table.
 * It seems that repeated states are always adjacent to each other.
 * The algorithm is:
 *   for each state, 
 *     find all successive states that are the same as it,
 *     go through the entire parsing table to update link to
 *     those repeated states to the first such state.
 *     remove the repeated states from the reachable states.
 *
 * Note:
 * It seems that all equal rows are adjacent to each other,
 * Using this observation, it's faster to go through the cycle.
 * But this is just an observation though.
 * Maybe should use the safer way, to use a double loop.
 * It's O(n^2) anyway.
 */
void furtherOptimization() {
  int i, j, k;
  //n_state_opt12 = states_reachable_count + 1;

  for (k = 0; k < states_reachable_count - 1; k ++) {
    i = states_reachable[k];
    j = states_reachable[k + 1];
    //printf("furtherOpt: i = %d, j = %d\n", i, j);
    do {
      if (isEqualRow(i, j) == FALSE) break;

      updateRepeatedRows(i, j);
      //printf("state %d removed\n", states_reachable[k + 1]);
      removeReachableState(k + 1);  
      if ( (k + 1) == states_reachable_count) break;
      j = states_reachable[k + 1];
      //printf("- furtherOpt: i = %d, j = %d\n", i, j);
    } while (1);
  }

  n_state_opt123 = states_reachable_count + 1;

  if (SHOW_PARSING_TBL && (n_state_opt12 > n_state_opt123)) {
    yyprintf("After further optimization, ");
    yyprintf3("total states reduced from %d to %d\n",
           n_state_opt12, n_state_opt123);
  }
}

/////////////////////////////////////////////////////
// Functions for further optimization. End.
/////////////////////////////////////////////////////

