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

#ifndef _HYACC_H_
#define _HYACC_H_

/****************************************************************
 * y.h
 *
 * Header file for y.c
 *
 * @Author: Xin Chen
 * @Created on: 8/30/2006
 * @last modified: 3/24/2009
 * @Copyright (C) 2007, 2008, 2009
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h> /* exit, malloc, realloc, free, system. */
#include <string.h> /* strtok, strcpy, strcmp. */
#include <ctype.h>  /* isspace, isdigit. */


#define SYMBOL_INIT_SIZE 128   /* Init size of a symbol string. */
#define SYMBOL_MAX_SIZE  512
#define GRAMMAR_RULE_INIT_MAX_COUNT 256 /* Number of rules allowed. */
#define PARSING_TABLE_INIT_SIZE 1024
#define STATE_INIT_SIZE 1024   /* Number of configs in a state. */
#define STATE_SUCCESSOR_INIT_MAX_COUNT 8
/* 
 * Max line length in yacc input file declaration section
 * starting with "%start" or "%token".
 * Used in gen_compiler.c and get_yacc_grammar.c.
 */
#define LINE_INIT_SIZE 128
/* 
 * Whether add the $accept = ... first goal rule. 
 * This should generally always be set to 1.
 */
#define ADD_GOAL_RULE 1

typedef int BOOL;
#define TRUE  1
#define FALSE 0


/* store output of y.output. */
FILE * fp_v;


#define yyprintf(s) \
{ if (fp_v != NULL) fprintf(fp_v, s); }
#define yyprintf2(s, t) \
{ if (fp_v != NULL) fprintf(fp_v, s, t); }
#define yyprintf3(r, s, t) \
{ if (fp_v != NULL) fprintf(fp_v, r, s, t); }
#define yyprintf4(p, q, r, s) \
{ if (fp_v != NULL) fprintf(fp_v, p, q, r, s); }
#define yyprintf5(p, q, r, s, t) \
{ if (fp_v != NULL) fprintf(fp_v, p, q, r, s, t); }
#define yyprintf6(p, q, r, s, t, u) \
{ if (fp_v != NULL) fprintf(fp_v, p, q, r, s, t, u); }


#define HYY_NEW(name, type, size) \
{\
  name = (type *) malloc(sizeof(type) * (size)); \
  if (name == NULL) {\
    printf("HYY_NEW error: out of memory\n"); \
    exit(1); \
  }\
}


#define HYY_EXPAND(name, type, size) \
{\
  name = (type *) realloc((void *) name, sizeof(type) * (size)); \
  if (name == NULL) {\
    printf("HYY_EXPAND error: out of memory\n"); \
    exit(1); \
  }\
}
    

#define YYERR_EXIT(msg) \
{\
  printf(msg); \
  exit(1); \
}


//////////////////////////////////////////////////////////////////
// Options that can be turned on/off in get_options.c
//////////////////////////////////////////////////////////////////
BOOL USE_COMBINE_COMPATIBLE_CONFIG;
BOOL USE_COMBINE_COMPATIBLE_STATES;
BOOL USE_REMOVE_UNIT_PRODUCTION; 
BOOL USE_REMOVE_REPEATED_STATES;
/* Switches for debug purpose in y.c */
BOOL SHOW_GRAMMAR;
BOOL SHOW_PARSING_TBL;
BOOL DEBUG_GEN_PARSING_MACHINE;
BOOL DEBUG_COMB_COMP_CONFIG;
BOOL DEBUG_BUILD_MULTIROOTED_TREE;
BOOL DEBUG_REMOVE_UP_STEP_1_2;
BOOL DEBUG_REMOVE_UP_STEP_4;
BOOL SHOW_TOTAL_PARSING_TBL_AFTER_RM_UP;
BOOL SHOW_THEADS;
BOOL DEBUG_EXPAND_ARRAY;
BOOL DEBUG_HASH_TBL;
BOOL SHOW_SS_CONFLICTS;
BOOL SHOW_STATE_TRANSITION_LIST;
BOOL SHOW_STATE_CONFIG_COUNT;
BOOL SHOW_ACTUAL_STATE_ARRAY;
/* switch for YYDEBUG in hyaccpar */
BOOL USE_YYDEBUG;
BOOL USE_LINES;
BOOL USE_VERBOSE;
/* used by gen_compiler.c, value obtained in get_options.c */
char * y_tab_c;
char * y_tab_h;
BOOL USE_OUTPUT_FILENAME;
BOOL USE_FILENAME_PREFIX;
BOOL USE_HEADER_FILE;
BOOL USE_GENERATE_COMPILER;
BOOL PRESERVE_UNIT_PROD_WITH_CODE;
/* used by y.c, value obtained in get_options.c */
char * y_output;
/* used by gen_graphviz.c, value obtained in get_options.c */
char * y_gviz;
BOOL USE_GRAPHVIZ;
/* lane-tracing algorithm */
BOOL USE_LR0;
BOOL USE_LALR;
BOOL USE_LANE_TRACING;
BOOL SHOW_ORIGINATORS;

////////////////////////////////////////////
// For symbol table
////////////////////////////////////////////

struct RuleIndexNode {
  int ruleID;
  struct RuleIndexNode * next;
};
typedef struct RuleIndexNode RuleIDNode;

/* _NEITHER: like empty string is neither. */
typedef enum {_TERMINAL, _NONTERMINAL, _NEITHER} symbol_type;

typedef enum {_LEFT, _RIGHT, _NONASSOC} associativity;
struct TerminalProperty {
  BOOL is_quoted;
  int precedence;
  associativity assoc;
};

/* contains a symbol. */
struct SymbolTableNode {
  char * symbol;
  int value; /* symbol value, for parsing table col header. */
  symbol_type type;
  int vanishable;
  struct TerminalProperty * TP;
  int seq; /* sequence (column) number in parsing table. */
  RuleIDNode * ruleIDList;
  struct SymbolTableNode * next;
};
typedef struct SymbolTableNode SymbolTblNode;

/* entry for hash table array. */
typedef struct {
  int count;
  SymbolTblNode * next;
} HashTblNode;

#define HT_SIZE 997 /* should be a prime. */
HashTblNode HashTbl[HT_SIZE];

/* used in other data structures. */
struct SymNode {
  SymbolTblNode * snode;
  struct SymNode * next;
};
typedef struct SymNode SymbolNode;

typedef SymbolNode * SymbolList;


//////////////////////////////////
// For queue used by getClosure()
// For queue.c
//////////////////////////////////

#define QUEUE_ERR_CODE -10000000

typedef struct {
  int * array;
  int head;
  int tail;
  int count;
  int size;

  /* These three are for collecting information purpose. */
  long max_count;
  long sum_count;
  long call_count;
} Queue;

Queue * queue_create();
void queue_destroy(Queue * q);
void queue_expand(Queue * q);
void queue_push(Queue * q, int n);
int queue_pop(Queue * q);
int queue_peek(Queue * q);
int queue_exist(Queue * q, int n);
void queue_dump(Queue * q);
int queue_count(Queue * q);
void queue_info(Queue * q);

Queue * config_queue;


/*
 * Use final state default reduction in the hyaccpar parse engine.
 * This significantly decreases the size of the generated parser
 * and overcomes the problem of always need to get the new lookahead
 * token to proceed parsing.
 * Only when debugging or for other development reason should this
 * be set to 0.
 */
#define USE_REM_FINAL_STATE 1


////////////////////////////////////
// Data structures for HYACC.
////////////////////////////////////

/* Context of a configuration. */
typedef struct _Context Context;
struct _Context {
  SymbolList nContext;
  int context_count;
  Context * next; // Used for LR(k) only.
};


/* Production of a configuration, or rule of a grammar. */
typedef struct {
  SymbolNode * nLHS;
  SymbolList nRHS_head;
  SymbolNode * nRHS_tail;
  int RHS_count;
  unsigned int isUnitProduction : 1; // 1 - true, 0 - false
  unsigned int hasCode : 1; // has associated code: 1 - true, 0 -false
  unsigned int padding : 30;
  SymbolTblNode * lastTerminal; // for precedence information.
} Production;


/*
 * Used for LANE_TRACING.
 */
struct _OriginatorList {
  struct ConfigurationNode ** list;
  int count;
  int size;
};
typedef struct _OriginatorList OriginatorList;
int OriginatorList_Len_Init;

/*
 * This has the same structure as OriginatorList.
 */
typedef struct _OriginatorList TransitorList;


/*
 * configuration's production is grammar.rules[ruleID].
 */
typedef struct ConfigurationNode Configuration;
struct ConfigurationNode {
  int ruleID; // index of rule in grammar.rules[]. 
  SymbolNode * nMarker; // point to scanned symbol.
  int marker; // redundant to nMarker, but make processing easier.
  Context * context;
  struct StateNode * owner;

  unsigned int isCoreConfig : 1; /* 1 - true, 0 - false. */
  /* flags for lane_tracing. */
  unsigned int COMPLETE : 1; 
  unsigned int IN_LANE  : 1;
  unsigned int ORIGINATOR_CHANGED : 1;
  unsigned int LANE_END : 1;  /* last config in lane. */
  unsigned int LANE_CON : 1;  /* label for config on conflict lane. */
  unsigned int CONTEXT_CHANGED : 1; /* for LR(k) use only. */
  unsigned int padding : 25;
  OriginatorList * originators; /* for LANE_TRACING */
  OriginatorList * transitors;  /* for phase 2 of LANE_TRACING */
};


struct ConflictNode {
  int state;
  SymbolTblNode * lookahead;
  int r;  // reduce, neg, and is always the smaller one.
  int s;  // reduce or shift, neg or pos.
  int decision;  // final decision.
  struct ConflictNode * next;  
};
typedef struct ConflictNode Conflict;

int ss_count; // total count of shift/shift conflicts.
int rr_count; // total count of reduce/reduce conflicts.
int rs_count; // total count of shift/shift conflicts.

int expected_sr_conflict; // expected shift/reduce conflicts.


struct _StateList {
  struct StateNode ** state_list;
  int count;
  int size;
};
typedef struct _StateList StateList;


struct StateNode {
  Configuration ** config;
  int config_max_count;
  int config_count;
  int core_config_count;

  int state_no;
  SymbolNode * trans_symbol;

  StateList * parents_list;

  struct StateNode ** successor_list;
  int successor_count;
  int successor_max_count;

  struct StateNode * next;

  /* for lane-tracing */
  unsigned int ON_LANE : 1; 
  unsigned int COMPLETE : 1; 
  unsigned int PASS_THRU : 1; /* for phase 2 */
  unsigned int REGENERATED : 1; /* for phase 2 regeneration */
  unsigned int padding : 28;
};
typedef struct StateNode State;


/*
 * Note: a state collection is implemented as a linked list.
 */
typedef struct {
  State * states_head;
  State * states_tail;
  int state_count;
} State_collection;


/*
 * Note: a vanish symbol must occur on the
 * LHS of a production, so must be a non-terminal.
 */
typedef struct {
  Production ** rules;
  int rule_max_count;
  int rule_count;
  SymbolNode * goal_symbol;
  SymbolList terminal_list;
  int terminal_count;
  SymbolList non_terminal_list;
  int non_terminal_count;
  SymbolList vanish_symbol_list;
  int vanish_symbol_count;
} Grammar;


Grammar grammar; /* Used by the entire program. */

State_collection * states_new;

/* for indexed access of states_new states */
typedef struct {
  State ** state_list;

  // each cell is for a state, index is state_no.
  Conflict ** conflict_list; 
  int * rs_count;     // shift/reduce conflicts count.
  int * rr_count;     // reduce/reduce conflicts count.

  int state_count;
  int size;
} State_array;

State_array * states_new_array;


/* Variables for parsing table. */

#define CONST_ACC -10000000  /* for ACC in parsing table */

int PARSING_TABLE_SIZE;  /* Default to STATE_COLLECTION_SIZE. */
int * ParsingTable;
int ParsingTblCols;
int ParsingTblRows;
/*
 * For parsing table column header names.
 * Value = terminal_count + non_terminal_count + 1 columns.
 */
SymbolTblNode ** ParsingTblColHdr;
/*
 * For final parsing table.
 */
SymbolList F_ParsingTblColHdr;
int F_ParsingTblCols;


/*  
 * Used by step 4 of remove unit production in y.c,
 * and by get_yy_arrays() in gen_compiler.c.
 */
int * states_reachable;
int states_reachable_count;


/*
 * For condensed parsing table after removing unit productions.
 * Used by function getActualState() in y.c and gen_compiler.c.
 */
int * actual_state_no;
int actual_state_no_ct;


/* Statistical values. */
int n_symbol;
int n_rule;
int n_rule_opt;
int n_state_opt1;
int n_state_opt12;
int n_state_opt123;

/* defined in hyacc_path.c, used in gen_compiler.c */
char * HYACC_PATH;


/*************************
 * Function headers.
 *************************/

/* Functions from grammars.c */
extern void useGrammar(int grammar_index);

/* functions in y.c */
extern Context * createContext();
extern Production * createProduction(
         char * lhs, char * rhs[], int rhs_count);
//extern Configuration * createConfig();
extern Configuration * createConfig(
         int ruleID, int marker, int isCoreConfig);
extern State * createState();
extern BOOL isGoalSymbol(SymbolTblNode * snode);
extern int getActualState(int virtual_state);
extern void getAction(int symbol_type, int col, int row,
               char * action, int * state_dest);
//extern BOOL isVanishSymbol(SymbolTblNode * n);
extern BOOL isParentSymbol(SymbolTblNode * s);
extern BOOL isReachableState(int state);
extern void getNonTerminals(Grammar * g);
extern void getGoalSymbol(Grammar * g);
extern void getTerminals(Grammar * g);
extern void getVanishSymbols(Grammar * g);
extern int getOptRuleCount(Grammar * g);
extern void destroyState(State * s);
extern BOOL isSameState(State * s1, State * s2);
extern BOOL isCompatibleStates(State * s1, State * s2);
extern BOOL combineCompatibleStates(State * s_dest, State * s_src);
extern BOOL isUnitProduction(int rule_no);

extern BOOL isFinalConfiguration(Configuration * c);
extern BOOL isEmptyProduction(Configuration * c);
extern BOOL addSymbol2Context(SymbolTblNode * snode, Context * c);
extern BOOL isNonTerminal(SymbolTblNode * s);
extern int isCompatibleSuccessorConfig(State * s, int ruleID);
extern void insertAction(SymbolTblNode * lookahead, int row, int state_dest);
extern void insertReductionToParsingTable(
       Configuration * c, int state_no);
extern State * addState2Collection(State_collection * c, State * new_state);
extern void copyConfig(Configuration * c_dest, Configuration * c_src);
extern void addCoreConfig2State(State * s, Configuration * new_config);
extern BOOL addTransitionStates2New (
       State_collection * coll, State * src_state);
extern void addStateToStateArray(State_array * a, State * s);
extern void addSuccessor(State * s, State * n);
extern void expandParsingTable();
extern SymbolNode * getTHeads(SymbolNode * str);
extern void showTHeads(SymbolList alpha, SymbolList theads);
extern Configuration * findSimilarCoreConfig(State * t, Configuration * c,
                                      int * config_index);
extern void copyContext(Context * dest, Context * src);
extern BOOL isTerminal(SymbolTblNode * s);
extern void mandatoryUpdateAction(
              SymbolTblNode * lookahead, int row, int state_dest);
extern void destroyConflictNode(Conflict * c);
extern void destroyConflictList(Conflict * a);
extern void freeContext(Context * c);
extern void getReachableStates(int cur_state, int states_reachable[],
    int * states_count);
extern void printParsingTableNote(); 
extern int getGrammarRuleCount();
extern SymbolNode * insertSymbolList_unique_inc(
         SymbolList list, SymbolTblNode * snode, int * exist);
extern void printParsingTable(); // for DEBUG use.

extern void getClosure(State * s);
extern void transition(State * s);
extern BOOL hasCommonCore(State * s1, State * s2);
extern BOOL combineContext(Context * c_dest, Context * c_src);
extern void clearContext(Context * c);
extern void getContext(Configuration * cfg, Context * context);
extern void updateStateParsingTblEntry(State * s);
extern void propagateContextChange(State * s);

extern void insert_state_to_PM(State * s);

extern StateList * StateList_clone(StateList * L0);

/*
 * Given a symbol, returns which column it locates in
 * the parsing table.
 *
 * The column is arranged this way:
 * strEnd, terminals, non-terminals.
 * Use macro so it's inline and faster.
 *
 * Used in y.c and upe.c.
 */
#define getCol(n) (n)->seq

/*
 * Given a SymbolTblNode, returns whether it is vanishable.
 */
#define isVanishSymbol(n) (n)->vanishable

/*
 * Update the destination state of a entry in the parsing table.
 * Used by updateRepeatedRow(), remove_unit_production_step3() and
 * remove_unit_production_step1and2().
 * Type of symbol is SymbolTblNode *.
 * Just copy the value of state_dest.
 *
 * Used in y.c and upe.c.
 */
#define updateAction(col, row, state_dest) \
{ ParsingTable[(row) * ParsingTblCols + (col)] = state_dest; }


/* Defined in upe.c */
extern void writeActualStateArray();
extern void remove_unit_production();
extern void printFinalParsingTable();
extern void furtherOptimization();
extern void get_actual_state_no();
extern void printCondensedFinalParsingTable();

/* Defined in get_yacc_grammar.c */
extern void getYaccGrammar(char * infile);

/* Defined in version.c */
extern void print_version();

/* Defined in hyacc_path.c */
extern void show_manpage();

/* Defined in get_options.c */
extern int get_options(int argc, char ** argv);

/* Defined in gen_compiler.c */
int * final_state_list; // for final states.
extern void generate_compiler(char * infile);

/* functions in symbol_table.c */
extern void hashTbl_init();
extern SymbolTblNode * hashTbl_insert(char * symbol);
extern SymbolTblNode * hashTbl_find(char * symbol);
extern void hashTbl_dump();
extern void hashTbl_destroy();
extern SymbolNode * createSymbolNode(SymbolTblNode * sn);
extern SymbolNode * findInSymbolList(SymbolList a, SymbolTblNode * s);
extern SymbolNode * findInIncSymbolList(SymbolList a, SymbolTblNode * s);
extern SymbolList cloneSymbolList(const SymbolList a);
extern void freeSymbolNode(SymbolNode * n);
extern void freeSymbolNodeList(SymbolNode * a);
extern RuleIDNode * createRuleIDNode(int ruleID);
extern void writeSymbolList(SymbolList a, char * name);
extern SymbolList removeFromSymbolList(
         SymbolList a, SymbolTblNode * s, int * exist);
extern int getSymbolListLen(SymbolList a);
extern SymbolNode * combineIncSymbolList(SymbolList a, SymbolList b);
extern SymbolNode * insertIncSymbolList(SymbolList a, SymbolTblNode * n);


/* functions in state_hash_table.c */
extern void initStateHashTbl();
extern State * searchStateHashTbl(State * s, int * is_compatible);
extern State * searchSameStateHashTbl(State * s);
extern void StateHashTbl_dump();


/*
 * For use by get_yacc_grammar.c and gen_compiler.c
 */
SymbolList tokens;
SymbolNode * tokens_tail;
int tokens_max_ct;
int tokens_ct;

extern void writeTokens();

/*
 * Define the state of yacc input file section 2.
 */
typedef enum {LHS, LHS_COMMENT, COLON, COLON_COMMENT,
  RHS, TERMINAL, CODE,
  CODE_SINGLE_QUOTE, CODE_DOUBLE_QUOTE,
  CODE_COMMENT, CODE_COMMENT2, COMMENT, COMMENT2} YACC_STATE;

extern char * strAccept;
extern char * strPlaceHolder;
extern char * strEnd;
extern char * strEmpty;
extern char * strError; 


/* function in gen_graphviz.c */
extern void gen_graphviz_input();  /* For O0, O1 */
extern void gen_graphviz_input2(); /* For O2, O3 */

/* functions in lr0.c */
extern void generate_LR0_parsing_machine();
extern SymbolTblNode * getScannedSymbol(Configuration * c); // in y.c
extern State_collection * createStateCollection();
extern State * findStateForScannedSymbol(
        State_collection * c, SymbolTblNode * symbol);
extern void updateParsingTable_LR0();
extern void outputParsingTable_LALR();

/* list of inadequate states for lane-tracing. */

typedef struct {
  int * states; /* list of state_no */
  //SymbolNode ** conflictSymbolList; // conflict symbols for each state 
  int count;
  int count_unresolved; /* unresolved after lane tracing phase 1. */
  int size;
} StateNoArray;

StateNoArray * states_inadequate;

/* functions in lane_tracing.c */
extern OriginatorList * createOriginatorList();
extern BOOL insertOriginatorList(Configuration * c, 
              Configuration * originator, int cycle);
extern StateNoArray * createStateNoArray();
extern void addStateNoArray(StateNoArray * sa, int state_no);
extern void writeConfigOriginators(Configuration * c);
extern void writeConfigTransitors(Configuration * c);
extern void lane_tracing();
extern void stdout_writeConfig(Configuration * c);
extern BOOL is_inadequate_state(int state_no);
/* used by both originator list and transitor list */
extern void expandOriginatorList(OriginatorList * o);
extern void lane_tracing_reduction(Configuration * c);

/* these are used in lane_tracing.c only. */
extern void my_writeState(State * s);
extern void my_showTHeads(SymbolList alpha, SymbolList theads);
extern BOOL testA(SymbolNode * n);
extern SymbolList getCONTEXTS_GENERATED(SymbolList list,
                                 int * null_possible);
extern SymbolNode * combineContextList(
             SymbolList list, SymbolList new_list);
extern void writeConflictingContext(int state_no); // for debug.


/*
 * When this is true, transition occurs only on conflicting contexts.
 */
extern BOOL in_lanetracing;

#endif

