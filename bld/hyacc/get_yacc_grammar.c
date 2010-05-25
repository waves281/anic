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
 * get_yacc_grammar.c
 *
 * Parse Yacc input file, extract the grammar and feed to y.c.
 *
 * @Author: Xin Chen
 * @Created on: 9/28/2006
 * @Last modified: 3/21/2007
 * @Copyright (C) 2007
 */

#include "y.h"

#define DEBUG_YACC_INPUT_PARSER 0


//////////////////////////////////////////////////////////////////
// Basically, there are 3 sections in a yacc input file.
// Call these 3 states. State 1 is definition section,
// state 2 is grammar section, state 3 is for code section.
//////////////////////////////////////////////////////////////////

/* Special symbols used by the entire program */
char * strAccept = "$accept";
char * strPlaceHolder = "$placeholder";
char * strEnd = "$end";
char * strEmpty = "";
char * strError = "error"; // reserved word.

static char * ysymbol; // token symbol.
static int ysymbol_pt;
static int ysymbol_size;

static SymbolTblNode * curLHS;

static YACC_STATE yacc_sec2_state;
static int CODE_level;

static FILE * fp; // The yacc input file.
static SymbolTblNode * start_symbol;

static int n_line; // count the line number of yacc input file.
static int n_col;  // count the current column number of yacc input.

static int precedence;
static int IS_PREC; // used for %prec


/*
 * %prec is in section2. See output_nonterminal() function
 * in gen_compiler.c. To be implemented.
 *
 * In section 1, %left, %right, %union, %nonassoc,
 * %type, %expect, %pure_passer are to be implemented.
 */
typedef enum {IS_NONE, IS_CODE, IS_TOKEN, IS_DIRECTIVE,
              IS_START, IS_LEFT, IS_RIGHT, IS_UNION, IS_TOKEN_TYPE,
              IS_NONASSOC, IS_TYPE, IS_EXPECT, IS_QUOTED_TERMINAL,
              IS_PURE_PARSER, IS_COMMENT, IS_UNKNOWN} 
yacc_section1_state;


/*
 * Functions declarations.
 */
void addRHSSymbol(SymbolTblNode * symbol);


/*
 * case-insensitive string compare.
 * Return:
 *   - 1 if a > b
 *   - -1 if a < b
 *   - 0 if a == b
 *
 * Note: strcasecmp() is not an ANSI C function.
 * So use my own to be ANSI C compliant.
 */
int y_strcasecmp(const char * a, const char * b) {
  int len_a, len_b, len, i, cmp_val;
  len_a = strlen(a);
  len_b = strlen(b);

  len = len_a;
  if (len > len_b) len = len_b;

  for (i = 0; i < len; i ++) {
    cmp_val = tolower(a[i]) - tolower(b[i]);
    if (cmp_val > 0) return 1;
    if (cmp_val < 0) return -1;
  }

  cmp_val = len_a - len_b;
  if (cmp_val > 0) return 1;
  if (cmp_val < 0) return -1;

  return 0;
}


void init_terminal_property(SymbolTblNode * n) {
  HYY_NEW(n->TP, struct TerminalProperty, 1);
  n->TP->precedence = 0;
  n->TP->assoc = _NONASSOC;
  n->TP->is_quoted = FALSE;
}


/*
 *  A valid identifier: [alph|_](alph|digit|_)*.
 *  where alph is [a-zA-Z], digit is [0-9].
 */
BOOL validate_identifier(char * s) {
  int i;
  int len = strlen(s);

  if (len == 0) return FALSE;

  if (! (isalpha(s[0]) || s[0] == '_')) return FALSE;

  for (i = 1; i < len; i++) {
    if (! (isalnum(s[i]) || s[i] == '_')) return FALSE;
  } 
  return TRUE;
}


/*
 * Assumption: n is a terminal, and n->TP != NULL.
 */
void get_terminal_precedence(SymbolTblNode * n, 
                             yacc_section1_state state) {
  if (n->type != _TERMINAL || n->TP == NULL) return;

  if (state == IS_LEFT) {
    n->TP->assoc = _LEFT;
    n->TP->precedence = precedence;
  } else if (state == IS_RIGHT) {
    n->TP->assoc = _RIGHT;
    n->TP->precedence = precedence;
  } else if (state == IS_NONASSOC) {
    n->TP->assoc = _NONASSOC;
    n->TP->precedence = precedence;
  }
}


/* 
 * Add a token to the tokens list.
 */
void addToken(SymbolTblNode * n) {
  if (tokens_tail == NULL) {
    tokens_tail = tokens = createSymbolNode(n);
  } else {
    tokens_tail->next = createSymbolNode(n);
    tokens_tail = tokens_tail->next;
  }

  tokens_ct ++;
}



/*
 * Used when process section 1 of the grammar file.
 *
 * The token added here is a terminal.
 * These are declared in the first section of yacc input file.
 * empty string is not allowed.
 */
void output_terminal(yacc_section1_state state,
                     yacc_section1_state prev_state) {
  SymbolTblNode * n;
  if (ysymbol_pt == 0) {
    if (state == IS_QUOTED_TERMINAL) {
      printf("error [line %d, col %d]: empty token is not allowed\n",
             n_line, n_col);
      exit(1);
    }
    return;
  }

  ysymbol[ysymbol_pt] = 0;
  n = hashTbl_insert(ysymbol);
  ysymbol_pt = 0;

  n->type = _TERMINAL;

  // get property of this terminal.
  // Quote informatin is used to decide to print it to y.tab.h.
  init_terminal_property(n);

  if (state == IS_QUOTED_TERMINAL) {
    n->TP->is_quoted = TRUE;
    get_terminal_precedence(n, prev_state);
  } else {
    if (validate_identifier(ysymbol) == FALSE) {
      printf("error [line %d, col %d]: invalid identifier: %s\n",
             n_line, n_col, ysymbol);
      n->TP->is_quoted = FALSE;
      get_terminal_precedence(n, state);
      return;
      //exit(1);
    }
    n->TP->is_quoted = FALSE;
    get_terminal_precedence(n, state);
  }

  addToken(n);
}


void get_start_symbol() {
  if (ysymbol_pt == 0) return;

  ysymbol[ysymbol_pt] = 0;
  start_symbol = hashTbl_insert(ysymbol);
  ysymbol_pt = 0;

  start_symbol->type = _NONTERMINAL;
}


void get_type_symbol() {
  SymbolTblNode * n;
  if (ysymbol_pt == 0) return;

  ysymbol[ysymbol_pt] = 0;
  n = hashTbl_insert(ysymbol);
  ysymbol_pt = 0;

  n->type = _NONTERMINAL;
}


/*
 * If more than one %expect value, use the first one.
 *
 * Note that atoi() function returns 0 if error occurs,
 * like when the ysymbol is a string "abc" and not a number.
 */
void get_expect_sr_conflict() {
  if (ysymbol_pt == 0) return;

  ysymbol[ysymbol_pt] = 0;

  if (expected_sr_conflict > 0) { // already got it.
    printf("warning [%d, %d]: more than one %%expect value: %s\n",
            n_line, n_col, ysymbol);
    ysymbol_pt = 0;
    return;
  }

  expected_sr_conflict = atoi(ysymbol);
  if (expected_sr_conflict < 0) {
    printf("error [%d, %d]: %%expect value %s is not positive\n", 
           n_line, n_col, ysymbol);
    exit(1);
  } else if (expected_sr_conflict == 0) {
    printf("error [%d, %d]: invalid %%expect value: %s\n", 
           n_line, n_col, ysymbol);
    exit(1);
  }

  //printf("expect: %d\n", expected_sr_conflict);
  ysymbol_pt = 0;
}


/*
 * Note: At this time, don't worry about whether the first
 * char of a symbol should be a letter.
 */
void addCharToSymbol(char c) {
  if (ysymbol_pt >= ysymbol_size - 1) { // one more for '\0'.
    ysymbol_size *= 2;

    if (ysymbol_size >= SYMBOL_MAX_SIZE + 1) {
      printf("[line %d, col %d] ", n_line, n_col);
      printf("addCharToSymbol Error: symbol max size %d reached\n",
              SYMBOL_MAX_SIZE);
      ysymbol[ysymbol_pt - 1] = 0;
      printf("symbol is: %s\n", ysymbol);
      exit(0);
    }

    HYY_EXPAND(ysymbol, char, ysymbol_size);
    printf("symbol size expanded to %d\n", ysymbol_size);
  }

  ysymbol[ysymbol_pt ++] = c;
}


/*
 * Every occurence of "%left" or "%right" increases
 * the precedence by 1.
 */
yacc_section1_state get_section1_state() {
  if (y_strcasecmp(ysymbol, "token") == 0) {
    return IS_TOKEN;
  } else if (y_strcasecmp(ysymbol, "start") == 0) {
    return IS_START;
  } else if (y_strcasecmp(ysymbol, "left") == 0) {
    precedence ++;
    return IS_LEFT;
  } else if (y_strcasecmp(ysymbol, "right") == 0) {
    precedence ++;
    return IS_RIGHT;
  } else if (y_strcasecmp(ysymbol, "nonassoc") == 0) {
    return IS_NONASSOC;
  } else if (y_strcasecmp(ysymbol, "union") == 0) {
    return IS_UNION;
  } else if (y_strcasecmp(ysymbol, "type") == 0) {
    return IS_TYPE;
  } else if (y_strcasecmp(ysymbol, "expect") == 0) {
    return IS_EXPECT;
  } else if (y_strcasecmp(ysymbol, "pure_parser") == 0) {
    return IS_PURE_PARSER;
  } else {
    printf("error [line %d, col %d]: unknown directive %%%s\n",
           n_line, n_col, ysymbol);
    //exit(1);
    return IS_UNKNOWN;
  }
}


static void my_perror(char * msg, char c) {
  printf("\nerror [line %d, col %d]: invalid char '%c'. %s\n",
         n_line, n_col, c, msg);
  exit(0);
}


/* 
 * Processes section 1 (declaration section)
 * of yacc input file.
 *
 * Currently this only gets the "%start" line if there is one.
 */
void processYaccFileInput_section1() {
  char c, last_c = '\n', last_last_c;
  yacc_section1_state state = IS_NONE, prev_state = -1;

  ysymbol_pt = 0;
  tokens = tokens_tail = NULL;

  tokens_ct = 0;
  n_line = 1;
  n_col =  1;

  while ((c = getc(fp)) != EOF) {

    if (state != IS_COMMENT && last_c == '\n' && c == '%') {
      // do nothing.
      state = IS_NONE;

    } else if (state != IS_COMMENT && 
               last_last_c == '\n' && last_c == '%') {
      if (c == '%') {
        break; // end of section1.
      } else if (c == '{') {
        state = IS_CODE;
      } else if (c == '}') {
        state = IS_NONE;
      } else if (isalpha(c)) { // %token, %left, ... etc.
        state = IS_DIRECTIVE;
        addCharToSymbol(c);
      } else {
        printf("error [line %d, col %d]: wrong directive name: %%%c",
               n_line, n_col, c);
        exit(1);
      }

    } else if (state == IS_CODE) {
      // do nothing.

    } else if (state == IS_COMMENT) {
      if (last_c == '*' && c == '/') {
        state = prev_state;
      }

    } else if (last_c == '/' && c == '*' && state != IS_QUOTED_TERMINAL) {
      prev_state = state;
      state = IS_COMMENT;

    } else if (c == '/' && state != IS_QUOTED_TERMINAL) { 
      // '/' is not a valid char in a unquoted token.
      if (state == IS_TOKEN && ysymbol_pt > 0) {
        output_terminal(state, prev_state);
      }

    } else if (state == IS_DIRECTIVE) {
      if (isspace(c)) {
        if (ysymbol_pt == 0) {
          my_perror("invalid char after %%", c);
        }
        addCharToSymbol(0);
        state = get_section1_state();
        ysymbol_pt = 0;

      } else {
        addCharToSymbol(c);
      }

    } else if(state == IS_QUOTED_TERMINAL) {
        //putchar(c);
        // avoid '\'' and '\\'.
        if (c == '\'' && (last_c != '\\' ||
            ysymbol_pt == 2 && ysymbol[0] == '\\')) {
          //printf("] terminal ends\n");
          output_terminal(state, prev_state); // output quoted terminal.
          state = prev_state;
        } else if (! isspace(c)) {
          addCharToSymbol(c);
        }

    } else if (state == IS_TOKEN_TYPE) {
      if (isspace(c)) {
        // do nothing, ignore space. 
        // if there are spaces between the words in token type,
        // these words will be combined. However that's not valid
        // and should not happen.
      } else if (c == '>') {
        // process token type. to be implemented.
        ysymbol[ysymbol_pt] = 0;
        //printf("token type [%d, %d]: %s\n", n_line, n_col, ysymbol);
        ysymbol_pt = 0;
        state = prev_state;
      } else {
        addCharToSymbol(c);
      }

    } else if (state == IS_TOKEN || state == IS_LEFT ||
               state == IS_RIGHT || state == IS_NONASSOC) {

      if (isspace(c) && ysymbol_pt == 0) {
        // do nothing, ignore space
      } else if (isspace(c)) { // output another token
        output_terminal(state, prev_state);
      } else if (c == '\'') {
        //printf("terminal starts: ['");
        prev_state = state;
        state = IS_QUOTED_TERMINAL;
      } else if (c == '<') { // start of <token_type>.
        prev_state = state;
        state = IS_TOKEN_TYPE;
      } else { // add char to token string
        addCharToSymbol(c);
      }

   } else if (state == IS_TYPE) { // %type declares non-terminals.
      if (isspace(c) && ysymbol_pt == 0) {
        // do nothing, ignore space
      } else if (isspace(c)) { // output another non-terminal.
        get_type_symbol();
      } else if (c == '<') { // start of <token_type>.
        prev_state = state;
        state = IS_TOKEN_TYPE;
      } else { // add char to token string
        addCharToSymbol(c);
      }

    } else if (state == IS_START) {
      // else, do nothing, ignore this line.
      if (isspace(c) && ysymbol_pt == 0) {
        // do nothing, ignore space.
      } else if (isspace(c)) { // output start token
        get_start_symbol(); // start symbol is a non-terminal.
      } else { // add char to token string.
        addCharToSymbol(c);
      }

    } else if (state == IS_UNION) {
      if (c == '{') { // union starts.

      } else if (c == '}') { // union ends.
        state = IS_NONE;
      } else { // get union content.

      }

    } else if (state == IS_EXPECT) {
      // else, do nothing, ignore this line.
      if (isspace(c) && ysymbol_pt == 0) {
        // ignore white space.
      } else if (isspace(c)) {
        get_expect_sr_conflict();
      } else {
        addCharToSymbol(c);
      }

    } else if (state == IS_PURE_PARSER) {
      if (c == '\n') state = IS_NONE;
      // else, do nothing, ignore this line.
    } else if (state == IS_UNKNOWN) {
      // do nothing.
    }

    last_last_c = last_c;
    last_c = c;

    n_col ++;
    if (c == '\n') { n_line ++; n_col = 1; }
  }

  //writeTokens();
}



/////////////////////////////////////////////////////////////
// Functions to add rules to grammar. Start.
/////////////////////////////////////////////////////////////


/*
 * Used by function createNewRule().
 */
Production * createEmptyProduction() {
  Production * p = (Production *) malloc(sizeof(Production));
  if (p == NULL) {
    printf("createProduction error: output memory\n");
    exit(0);
  }
  p->nLHS = createSymbolNode(hashTbl_find(""));

  p->RHS_count = 0;
  p->isUnitProduction = 0;
  p->hasCode = 0;
  p->nLHS = p->nRHS_head = p->nRHS_tail = NULL;
  p->lastTerminal = NULL;
  return p;
}


Production * createNewRule() {
  if (grammar.rule_count >= grammar.rule_max_count - 1) {
    grammar.rule_max_count *= 2;
    HYY_EXPAND(grammar.rules, Production *, grammar.rule_max_count);
    //printf("grammar.rule_max_count expanded to %d\n",
    //        grammar.rule_max_count);
  }

  //printf("--new rule:");
  grammar.rules[grammar.rule_count] = 
    (Production *) createEmptyProduction();
  grammar.rule_count ++;

  return grammar.rules[grammar.rule_count - 1];
}


/*
 * Add a rule for mid-production action.
 *
 * @return: the new created non-terminal, which will
 *          be inserted to the position of the mid-production action.
 */
static void insert_mid_prod_rule(int ct) {
  int ruleID;
  Production * p;
  char nameLHS[20];
  SymbolTblNode * n;

  createNewRule();

  // switch the postion of the last two rules.
  ruleID = grammar.rule_count - 1; // last rule's pointer.
  p = grammar.rules[ruleID];
  grammar.rules[ruleID] = grammar.rules[ruleID - 1];
  grammar.rules[ruleID - 1] = p;

  // now fill the value of the new added rule.
  sprintf(nameLHS, "$$%d_@%d", ruleID - 1, ct);
  n = hashTbl_insert(nameLHS);
  n->type = _NONTERMINAL;
  p->nLHS = createSymbolNode(n); 
  p->hasCode = 1;

  addRHSSymbol(n);
}


void addLHSSymbol(SymbolTblNode * symbol) {
  //printf("\n==add LHS symbol: %s\n", symbol);
  Production * p = grammar.rules[grammar.rule_count - 1];
  p->nLHS = createSymbolNode(symbol);
}


void addRHSSymbol(SymbolTblNode * symbol) {
  //printf("\n==add RHS symbol: %s\n", symbol);
  SymbolNode * s;
  Production * p = grammar.rules[grammar.rule_count - 1];
  p->RHS_count ++;

  s = createSymbolNode(symbol);
  if (p->nRHS_head == NULL) {
    p->nRHS_head = p->nRHS_tail = s;
  } else {
    p->nRHS_tail->next = s; 
    p->nRHS_tail = s;
  }

  // if s is a terminal, it's the last terminal of p's RHS.
  if (s->snode->type == _TERMINAL && 
      s->snode->TP != NULL) p->lastTerminal = s->snode; 
}


/*
 * Assumption: symbol is the one after %prec in the RHS of p.
 */
void getRHSPrecSymbol(char * symbol) {
  Production * p;
  SymbolTblNode * n = hashTbl_find(symbol);
  if (n == NULL) {
    printf("error [line %d, col %d]: ", n_line, n_col);
    printf("%%prec symbol %s should be declared.\n", symbol);
    exit(1);
  }
  p = grammar.rules[grammar.rule_count - 1];
  if (n->TP != NULL && n->TP->precedence > 0)
    p->lastTerminal = n;
}


void setHasCode() {
  grammar.rules[grammar.rule_count - 1]->hasCode = 1;
}


/////////////////////////////////////////////////////////////
// Functions to add rules to grammar. End.
/////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////
// Functions to get grammar parameters. START.
/////////////////////////////////////////////////////////////


static BOOL isInVanishSymbolList(SymbolTblNode * n) {
  SymbolNode * a;

  //printf("isInVanishSymbolList input: %s\n", symbol);
  if (strlen(n->symbol) == 0) { return TRUE; }

  for (a = grammar.vanish_symbol_list; a != NULL; a = a->next) {
    if (n == a->snode) return TRUE;
  }

  return FALSE;
}


/*
 * Used by the algorithm to find vanish symbol(s) of a grammar.
 */
BOOL flagY(Production * p) {
  SymbolNode * a;
  for (a = p->nRHS_head; a != NULL; a = a->next) {
    if (isInVanishSymbolList(a->snode) == FALSE) return FALSE;
  }
  return TRUE; // x1,...,xn are all vanish symbols. Flag y.
}


/*
 * Called when creating a grammar.
 *
 * Define epsilon as empty string "".
 * Then epsilon production is one whose
 * RHS_count = 1 AND RHS[0] == "".
 *
 * NOTE: a symbol that can vanish must occur
 * on the left side of a production, thus a
 * non-terminal symbol.
 *
 * Algorithm:
 *   1. Flag all symbols which occur in epsilon-productions,
 *      i.e., all symbols y which occur in productions of
 *      the form y -> epsilon.
 *   2. Go thru the grammar and for each produciton
 *      y -> x1...xn, if x1, ..., xn are all flagged,
 *      then flag y.
 *   3. If any new symbols were flagged in step 2, go
 *      back to step 2.
 */
void getVanishSymbols(Grammar * g) {
  SymbolNode * tail = NULL;
  int i, new_vanish_symbol_found;
  g->vanish_symbol_count = 0;

  // find vanish symbols that occur in epsilon-productions.
  for (i = 0; i < g->rule_count; i ++) {
    if (flagY(g->rules[i]) == TRUE) {

      if (tail == NULL) {
        tail = g->vanish_symbol_list =
                  createSymbolNode(g->rules[i]->nLHS->snode);
      } else {
        tail->next = createSymbolNode(g->rules[i]->nLHS->snode);
        tail = tail->next;
      }

      g->rules[i]->nLHS->snode->vanishable = 1; //12-6-2008

      g->vanish_symbol_count ++;
    }
  }

  // if so far no vanish symbol, then no epsilon-production.
  // then no more vanish symbols.
  if (g->vanish_symbol_count == 0) return;

  while(1) {
    new_vanish_symbol_found = 0;
    for (i = 0; i < g->rule_count; i ++) {
      if (isInVanishSymbolList(g->rules[i]->nLHS->snode) == FALSE) {
        // y is not yet a vanish symbol, then:
        if (flagY(g->rules[i]) == TRUE) {

          // we know tail != NULL
          tail->next = createSymbolNode(g->rules[i]->nLHS->snode);
          tail = tail->next;

          g->rules[i]->nLHS->snode->vanishable = 1; //12-6-2008

          g->vanish_symbol_count ++;
          new_vanish_symbol_found = 1;
        } // end if
      } // end if
    } // end for
    if (new_vanish_symbol_found == 0) break;
  } // end while
}


/*
 * Given a grammar's rules, get non-terminals.
 *
 * Assumption: all non-terminals are used as LHS of
 * some rules. If not, such invalid non-terminal will
 * be reported after getSymbolRuleIDList().
 */
void getNonTerminals(Grammar * g) {
  BOOL has_error = FALSE;
  int i, index = 0;
  SymbolNode * tail = NULL;

  g->non_terminal_count = 0;

  // First scan LHS of all rules.
  for (i = 0; i < g->rule_count; i ++) {
    if (findInSymbolList(g->non_terminal_list,
          g->rules[i]->nLHS->snode) != NULL) continue;

    if (tail == NULL) {
      tail = g->non_terminal_list =
               createSymbolNode(g->rules[i]->nLHS->snode);
    } else {
      tail->next = createSymbolNode(g->rules[i]->nLHS->snode);
      tail = tail->next;
    }

    if (strcmp(tail->snode->symbol, strAccept) != 0)
      tail->snode->value = (-1) * (++ index);

    g->non_terminal_count ++;
  }

  // Next scan RHS of all rules.
  // no extra non-terminal should appear, since otherwise
  // it's not used as the LHS of any rule.
  for (i = 0; i < g->rule_count; i ++) {
    for (tail = g->rules[i]->nRHS_head; tail != NULL; tail = tail->next) {
      if (tail->snode->type != _NONTERMINAL) continue;

      if (findInSymbolList(g->non_terminal_list,
                           tail->snode) == NULL) {
        printf("error: non-termnal '%s' ", tail->snode->symbol);
        printf("is not used as the LHS of any rule\n");
        has_error = TRUE;
      }

    }
  }
  //if (has_error == TRUE) exit(1);
}


/*
 * Assumption:
 * This function is called after getNonTerminals().
 * empty string is not included as terminal.
 */
void getTerminals(Grammar * g) {
  int i, j;
  char * symbol;
  SymbolNode * s, * tail = NULL;
  g->terminal_count = 0;

  for (i = 0; i < g->rule_count; i ++) {
    s = g->rules[i]->nRHS_head;
    for (j = 0; j < g->rules[i]->RHS_count; j ++) {
      if (j > 0) s = s->next; // s: g->rules[i]->RHS[j].
      symbol = s->snode->symbol;

      if (s->snode->type != _TERMINAL) continue;

      // is an empty string.
      if (strlen(s->snode->symbol) == 0) continue;

      if (findInSymbolList(g->terminal_list,
                           s->snode) != NULL) continue;

      if (tail == NULL) {
        tail = g->terminal_list =
                  createSymbolNode(s->snode);
      } else {
        tail->next = createSymbolNode(s->snode);
        tail = tail->next;
      }
      s->snode->type = _TERMINAL;
      g->terminal_count ++;
    } // end of for
  } // end of for
}


/*
 * ref: page 38. The C programming language.
 */
char get_escape_char(char c) {
  switch (c) {
    case 'a': return '\a'; break;
    case 'b': return '\b'; break;
    case 'f': return '\f'; break;
    case 'n': return '\n'; break;
    case 'r': return '\r'; break;
    case 't': return '\t'; break;
    case 'v': return '\v'; break;
    case  92: return '\\'; break;
    case '?': return '\?'; break;
    case  34: return '\"'; break;
    case  39: return '\''; break;
    default:  return 0;    break;
  }
}


/*
 * Used by getTokensValue() only.
 */
int get_token_value(char * s, int * index) {
  int val;

  if (strlen(s) == 1) return s[0]; // single letter.

  if (strlen(s) == 2 && s[0] == '\\') { // escaped sequence.
    val = get_escape_char(s[1]);
    if (val != 0) return val;
  }

  if (strcmp(s, strError) == 0) return 256;
  if (strcmp(s, strEnd) == 0) return 0;

  val = 257 + (* index);
  (* index) ++;
  return val;
}


/*
 * This function gets the values of all terminals,
 * Including those after %prec in the rule section,
 * and including strEnd, strError.
 *
 * Symbol can be:
 *   - a token, value is 257 + index in token list (in un-quoted ones).
 *   - "error", value is 256.
 *   - a char or escaped char, value is its ascii code.
 *   - strEnd, value is 0.
 *   - a non-terminal symbol, value is -1 * index in non-terminal list.
 *
 * The values of non-terminals are calculated in
 * getNonTerminals().
 *
 * strEmpty has no value. strAccept is a non-terminal,
 * whose value is default to 0. But the value of these
 * two are never used, so doesn't matter.
 * 
 * The values of tokens are used for y.tab.h, for yytoks[]
 * and for yytbltok[] in y.tab.c.
 *
 * Note that the terminal list is a subset of tokens list.
 * The tokens list can include those after %prec in the
 * rule section, which terminal list does not include.
 */
void getTokensValue(Grammar * g) {
  int index = 0;
  SymbolNode * a;
  for (a = tokens; a != NULL; a = a->next) {
    a->snode->value = get_token_value(a->snode->symbol, & index);
  }
}


void getGoalSymbol(Grammar * g) {
  g->goal_symbol = createSymbolNode(g->rules[0]->nLHS->snode);
}


/*
 * Get the column index in the parsing table for 
 * each symbol.
 * This makes it easy to find the address for a symbol
 * in the parsing table.
 *
 * This can be done when calling getNonTerminals() and 
 * getTerminals(), but doing it here separately makes
 * it easy to check, debug and change.
 * Since there are not many symbols, this will take
 * only very little time.
 */
void getSymbolParsingTblCol(Grammar * g) {
  int i;
  SymbolTblNode * n;
  SymbolNode * a;

  n = hashTbl_find(strEnd);
  n->seq = 0;

  for (i = 1, a = g->terminal_list; a != NULL; a = a->next, i ++) {
    n = hashTbl_find(a->snode->symbol);
    n->seq = i;
  }
  
  for (i = 1, a = g->non_terminal_list; a != NULL; a = a->next, i ++) {
    n = hashTbl_find(a->snode->symbol);
    n->seq = g->terminal_count + i;
  }
}


/*
 *  Get the list of Parsing table column header symbols.
 *  This plus the getSymbolParsingTblCol() function make it
 *  easy to refer between column number and column symbol.
 */
void getParsingTblColHdr(Grammar * g) {
  SymbolNode * a;

  HYY_NEW(ParsingTblColHdr, SymbolTblNode *, 
          1 + g->terminal_count + g->non_terminal_count);

  ParsingTblColHdr[0] = hashTbl_find(strEnd);
  ParsingTblCols = 1;

  for (a = grammar.terminal_list; a != NULL; a = a->next) {
    ParsingTblColHdr[ParsingTblCols ++] = a->snode;
  }

  for (a = g->non_terminal_list; a != NULL; a = a->next) {
    ParsingTblColHdr[ParsingTblCols ++] = a->snode;
  }
}


/*
 * For each non-terminal, get a list of indexes of rules
 * for which the non-terminal is the LHS.
 *
 * If a non-terminal is not the LHS of any rule,
 * it's an error and should be reported.
 */
void getSymbolRuleIDList(Grammar * g) {
  SymbolNode * a;
  SymbolTblNode * n;
  RuleIDNode * r, * tail;
  int i;

  for (a = g->non_terminal_list; a != NULL; a = a->next) {
    n = a->snode;
    tail = NULL;
    for (i = 0; i < g->rule_count; i ++) {
      if (n == g->rules[i]->nLHS->snode) {
        r = createRuleIDNode(i);
        if (tail == NULL) {
          n->ruleIDList = tail = r;
        } else {
          tail->next = r; // insert at head.
          tail = tail->next; 
        }
      }
    }
  }
  //writeNonTerminalRuleIDList();
}


void getGrammarUnitProductions(Grammar * g) {
  int i;
  for (i = 0; i < g->rule_count; i ++) {
    if (grammar.rules[i]->RHS_count == 1 &&
      strlen(grammar.rules[i]->nRHS_head->snode->symbol) > 0) {
      g->rules[i]->isUnitProduction = TRUE;
    }
  }
}


void getGrammarParams() {
  getNonTerminals(& grammar);
  getTerminals(& grammar);

  getTokensValue(& grammar);

  getGoalSymbol(& grammar);
  getVanishSymbols(& grammar);

  getSymbolParsingTblCol(& grammar);
  getParsingTblColHdr(& grammar);

  getSymbolRuleIDList(& grammar);
  getGrammarUnitProductions(& grammar);

  //writeSymbolList(grammar.non_terminal_list, "NT list");
  //writeSymbolList(grammar.terminal_list, "T list");
  //writeSymbolList(tokens, "tokens");
}


/////////////////////////////////////////////////////////////
// Functions to get grammar parameters. START.
/////////////////////////////////////////////////////////////


/*
 * Outputs a new symbol and inserts it to the LHS or RHS
 * of a rule of the grammar.
 *
 * Note that a construct like "%prec UNARY" indicates the
 * precedence of this rule. Both "%prec" and "UNARY" are 
 * not true terminals, and should not be inserted into the
 * terminal symbol list. The detail  of handling precedence
 * is to be implemented.
 *
 * If a new symbol is on the LHS, it's a non-terminal.
 * If it is on the RHS and not found in the symbol table,
 * and not quoted by '', then it's a non-terminal.
 * Note that all terminals not quoted by '' should already
 * be delcared in section 1.
 *
 * Empty string is not allowed.
 */
SymbolTblNode * output_nonterminal(YACC_STATE state) {
  SymbolTblNode * n;

  if (ysymbol_pt == 0) { 
    if (state == TERMINAL) {
      printf("error [line %d, col %d]: empty token is not allowed\n",
             n_line, n_col);
      exit(1);
    }
    return NULL; 
  }

  ysymbol[ysymbol_pt] = 0;

  if (y_strcasecmp(ysymbol, "%prec") == 0) {
    IS_PREC = 1;
    ysymbol_pt = 0;
    return NULL;
  } else if (IS_PREC == 1) {
    // is a ficticious terminal, no token actually.
    getRHSPrecSymbol(ysymbol); // ysymbol should exist.
    //printf("%%prec on symbol %s\n", ysymbol);
    IS_PREC = 0;
    ysymbol_pt = 0;
    return NULL;
  }
  //printf("anoterh symbol: %s\n", ysymbol);

#if DEBUG_YACC_INPUT_PARSER
  printf("%s ", ysymbol);
#endif

  n = hashTbl_find(ysymbol);

  if (yacc_sec2_state == LHS) {
    if (n == NULL) {
      n = hashTbl_insert(ysymbol);
      n->type = _NONTERMINAL;
    } else if (n->type == _TERMINAL) {
      printf("error [line %d, col %d]: ", n_line, n_col);
      printf("symbol %s is declared as terminal but used as non-terminal\n",
             ysymbol);
      exit(1);
    }
    createNewRule();       // CREATE NEW RULE HERE.
    addLHSSymbol(n);

  } else { // RHS.
    if (n == NULL) {
      n = hashTbl_insert(ysymbol);

      // if quoted by '', is terminal, otherwise is non-terminal.
      // "error" is a reserved word, a terminal.
      if (state == TERMINAL || 
          strcmp(ysymbol, strError) == 0) {
        n->type = _TERMINAL;
        init_terminal_property(n);
        n->TP->is_quoted = TRUE;

        // add this to the tokens list.
        addToken(n);
      } else {
        n->type = _NONTERMINAL;
      }
    }

    addRHSSymbol(n);
  }

  ysymbol_pt = 0;
  return n;
}


/*
 * Stores LHS of current rule.
 * To be used by the next rule with the same LHS in cases like:
 *   LHS : RHS1 | RHS2 ...
 */
void getCurLHS(SymbolTblNode * n) {
  if (n == NULL) {
    printf("error [line %d, col %d]: LHS symbol is empty.\n",
            n_line, n_col);
    exit(1);
  }
  curLHS = n;
}


/*
 * Processes section 2 (grammar section)
 * of a yacc input file.
 */
void processYaccFileInput_section2() {
  // for mid-production actions.
  int mid_prod_code_ct = 0;
  BOOL END_OF_CODE = FALSE;

  char c, last_c = 0;
  yacc_sec2_state = LHS;
  ysymbol_pt = 0;

  while((c = getc(fp)) != EOF) {
    if (last_c == '%' && c == '%') return;

    switch(yacc_sec2_state) {
      case LHS:
        if (isspace(c) && ysymbol_pt == 0) {
          // Ignore empty spaces before LHS symbol.
        } else if (c == ':') { 
          getCurLHS( output_nonterminal(LHS) ); // OUTPUT LHS SYMBOL.
#if DEBUG_YACC_INPUT_PARSER
          printf("-> ");
#endif
          yacc_sec2_state = RHS;
        } else if (isspace(c)) {
          getCurLHS( output_nonterminal(LHS) ); // OUTPUT LHS SYMBOL.
#if DEBUG_YACC_INPUT_PARSER
          printf("-> ");
#endif
          yacc_sec2_state = COLON;
        } else if (last_c == '/' && c == '*') {
          yacc_sec2_state = LHS_COMMENT;
          ysymbol_pt = 0;
        } else if (c == '/') { 
          // do nothing. '/' is not a valid char for a symbol.
        } else if (c == ';') {
          // do nothing. a rule without anything.
        } else if (! isspace(c)) { 
          // when encountering the '%%' starting section 3,
          // the first '%' will be inserted. But since it won't
          // call output_nonterminal(), a new rule won't be created. 
          // So no problem will occur here.
          addCharToSymbol(c);
        }
        break;
      case LHS_COMMENT:
        if (last_c == '*' && c == '/') {
         yacc_sec2_state = LHS;
        }
        break;
      case COLON:
        if (c == ':') {
          yacc_sec2_state = RHS;
        } else if (c == '/') { 
          // do nothing
        } else if (last_c == '/' && c == '*') {
          yacc_sec2_state = COLON_COMMENT;
        } else if (! isspace(c)) {
          my_perror("in state COLON", c);
        } 
        break;
      case COLON_COMMENT:
        if (last_c == '*' && c == '/') {
          yacc_sec2_state = COLON;
        }
        break;
      case RHS:
        if (isspace(c)) {
          if (ysymbol_pt != 0) {
            output_nonterminal(RHS); // OUTPUT NEXT RHS SYMBOL.
          }
          // else, ignore empty space.
        } else if (c == '\'') {
          //printf("terminal starts(line %d): [%c", n_line, c);
          if (ysymbol_pt != 0) {
            output_nonterminal(RHS); // OUTPUT NEXT RHS SYMBOL.
          }
          yacc_sec2_state = TERMINAL;
          if (END_OF_CODE == TRUE) { // for mid-prod action.
            mid_prod_code_ct ++;
            insert_mid_prod_rule(mid_prod_code_ct);
            END_OF_CODE = FALSE;
          }
        } else if (c == ';') {
          END_OF_CODE = FALSE; // for mid-prod action.
          if (ysymbol_pt == 0 && IS_PREC == 1) {
            printf("error [line %d, col %d]: ", n_line, n_col);
            printf("forgot the symbol after %%prec?\n");
            exit(1);
          }
          if (ysymbol_pt != 0)
            output_nonterminal(RHS); // OUTPUT NEXT RHS SYMBOL.
#if DEBUG_YACC_INPUT_PARSER
          printf("\n");
#endif
          yacc_sec2_state = LHS; 
        } else if (c == '|') { // another rule with same LHS.
          END_OF_CODE = FALSE; // for mid-prod action.
          if (ysymbol_pt == 0 && IS_PREC == 1) {
            printf("error [line %d, col %d]: ", n_line, n_col);
            printf("forgot the symbol after %%prec?\n");
            exit(1);
          }
          if (ysymbol_pt != 0) 
            output_nonterminal(RHS); // OUTPUT NEXT RHS SYMBOL.
#if DEBUG_YACC_INPUT_PARSER
          printf("\n%s -> ", curLHS->symbol);
#endif
          //printf("\n");
          createNewRule();     // CREATE NEW RULE HERE.
          addLHSSymbol(curLHS);
        } else if (c == '{') {
          setHasCode(); // has associated code.
///printf("start of code at rule %d:\n{", grammar.rule_count);
          yacc_sec2_state = CODE;
          CODE_level = 1;
        } else if (last_c == '/' && c == '*') {
          yacc_sec2_state = COMMENT; // the format "/* ... */"
        } else if (last_c == '/' && c =='/') {
          yacc_sec2_state = COMMENT2; // the format "// ..."
        } else if (c == '/') { 
          // do nothing.
        } else if (c == ':') {
          my_perror("A ';' is missed in the last rule?", c);
        } else {
          if (END_OF_CODE == TRUE) { // for mid-prod action.
            mid_prod_code_ct ++;
            insert_mid_prod_rule(mid_prod_code_ct);
            END_OF_CODE = FALSE;
          }
          addCharToSymbol(c);
        }
        break;
      case TERMINAL:
        //putchar(c); 
        // avoid '\'' and '\\'.
        if (c == '\'' && (last_c != '\\' ||
            ysymbol_pt == 2 && ysymbol[0] == '\\')) { 
          //printf("] terminal ends\n");
          yacc_sec2_state = RHS;
          output_nonterminal(TERMINAL); // OUTPUT NEXT RHS SYMBOL. is terminal.
        } else if (! isspace(c)) {
          addCharToSymbol(c);
        }
        break;
      case CODE:
        if (c == '\"') {
          yacc_sec2_state = CODE_DOUBLE_QUOTE;
        } else if (c == '\'') {
          yacc_sec2_state = CODE_SINGLE_QUOTE;
        } else if (c == '*' && last_c == '/') {
          yacc_sec2_state = CODE_COMMENT;
        } else if (c == '/' && last_c == '/') {
          yacc_sec2_state = CODE_COMMENT2;
        } else if (c == '}' && CODE_level == 1) {
          yacc_sec2_state = RHS;
///printf("end of code\n");
          END_OF_CODE = TRUE; // for mid-prod action.
        } else if (c == '{') {
          CODE_level ++;
        } else if (c == '}') {
          CODE_level --;
        }
        break;
      case CODE_DOUBLE_QUOTE:
        if (c == '\"' && last_c != '\\') yacc_sec2_state = CODE;
        break;
      case CODE_SINGLE_QUOTE:
        if (c == '\'') yacc_sec2_state = CODE;
        break;
      case CODE_COMMENT:
        if (c == '/' && last_c == '*') yacc_sec2_state = CODE;
        break;
      case CODE_COMMENT2:
        if (c == '\n') yacc_sec2_state = CODE;
        break;
      case COMMENT:
        if (last_c == '*' && c == '/') yacc_sec2_state = RHS;
        break;
      case COMMENT2:
        if (c == '\n') yacc_sec2_state = RHS;
        break;
      default:
        break;
    } // end switch

    //putchar(c);
    last_c = c;

    n_col ++;
    if (c == '\n') { n_line ++; n_col = 1; }

  } // end while
}


/*
 * If there is a "%start ..." in the declaration section,
 * copy the start symbol value to RHS of goal production.
 * Otherwise, use the LHS of the first user rule as the
 * RHS of goal production.
 */
void getGoalRuleRHS() {
  if (grammar.rule_count > 1) {
    if (start_symbol != NULL) {
      grammar.rules[0]->nRHS_head = grammar.rules[0]->nRHS_tail =
        createSymbolNode(start_symbol);
    } else {
      grammar.rules[0]->nRHS_head = grammar.rules[0]->nRHS_tail =
        createSymbolNode(grammar.rules[1]->nLHS->snode);
    }
    grammar.rules[0]->RHS_count = 1;
  } else {
    printf("getGoalRuleRHS() error: ");
    printf("there is no user rule.\n");
    exit(1);
  }
}


void getGoalRuleLHS() {
  SymbolTblNode * n = hashTbl_insert(strAccept);
  createNewRule(); // goal production rule.
  grammar.rules[0]->nLHS = createSymbolNode(n);
  n->type = _NONTERMINAL;
}


/*
 * This modification is to preserve those unit productions
 * that have associated code. In such case add a place holder
 * nonterminal to the end of each such unit production, to
 * convert them to non-unit productions. This place holder
 * nonterminal will reduce to empty string.
 */
void post_modification(Grammar * g) {
  int count = 0, i;
  Production * p;
  SymbolTblNode * n;

  //printf("calling post_modification\n");
  if (USE_REMOVE_UNIT_PRODUCTION == FALSE) return;
  if (PRESERVE_UNIT_PROD_WITH_CODE == TRUE) return;

  n = hashTbl_insert(strPlaceHolder);
  n->type = _NONTERMINAL;

  for (i = 0; i < g->rule_count; i ++) {
    if (g->rules[i]->RHS_count == 1 &&
        g->rules[i]->hasCode == 1) {
      //printf("rule %d is a unit production with code\n", i);
      count ++;
      p = g->rules[i];
      p->RHS_count ++;
      p->isUnitProduction = 0;
      // add one more symbol to the end of RHS:
      p->nRHS_tail->next = createSymbolNode(n);
      p->nRHS_tail = p->nRHS_tail->next;
    }
  }

  if (count > 0) {
    p = createNewRule(); // $PlaceHolder -> epsilon
    p->nLHS = createSymbolNode(n);

    p->RHS_count = 0;
    p->isUnitProduction = 0;
    p->hasCode = 0;
    p->nRHS_head = p->nRHS_tail = NULL;
  }
}


void get_yacc_grammar_init() {
  // insert special symbols to hash table.
  SymbolTblNode * n = hashTbl_insert(strEnd); // end marker of production.
  n->type = _TERMINAL;

  hashTbl_insert(strAccept);
  hashTbl_insert(strEnd);
  n = hashTbl_insert(strEmpty);
  n->type = _TERMINAL;
  n->vanishable = 1;

  grammar.rule_max_count = GRAMMAR_RULE_INIT_MAX_COUNT;
  HYY_NEW(grammar.rules, Production *, grammar.rule_max_count);

  ysymbol_size = SYMBOL_INIT_SIZE;
  HYY_NEW(ysymbol, char, ysymbol_size);

  start_symbol = NULL;

  precedence = 0;
  IS_PREC = 0;
  expected_sr_conflict = 0;
}


/*
 * The main function of this file.
 * Gets grammar from a yacc input file.
 *
 * Called by function main() in y.c.
 */
void getYaccGrammar(char * infile) {
#if DEBUG_YACC_INPUT_PARSER
  printf("input file: %s\n", infile);
#endif

  if ((fp = fopen(infile, "r")) == NULL) {
    printf("can't open file %s\n", infile);
    exit(1);
  }

  get_yacc_grammar_init();

  if (ADD_GOAL_RULE) { getGoalRuleLHS(); }

  processYaccFileInput_section1(); 
  processYaccFileInput_section2(); 

  fclose(fp);

  if (ADD_GOAL_RULE) { getGoalRuleRHS(); }
  post_modification(& grammar);

  getGrammarParams();

  n_rule = grammar.rule_count;
  n_symbol = grammar.terminal_count + grammar.non_terminal_count;
  n_rule_opt = getOptRuleCount(& grammar);
  //writeGrammar(& grammar); exit(0);

}

