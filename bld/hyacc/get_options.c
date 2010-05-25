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
 * get_options.c
 *
 * Gets switch options from the command line.
 *
 * @Author: Xin Chen
 * @Created on: Febuary 8, 2007
 * @Last modified: 7/25/2007
 * @Copyright (C) 2007
 */

#include "y.h"

static int cmd_argc; /* argc passed from main(). */
static int argc_pt;  /* index of current cmd parameter. */

static BOOL ERR_NO_INPUT_FILE = FALSE;
static BOOL ERR_UNKNOWN_SWITCH_O = FALSE;
static BOOL ERR_UNKNOWN_SWITCH_O_USE_LR0 = FALSE;
static BOOL ERR_UNKNOWN_SWITCH_D = FALSE;
static BOOL ERR_NO_OUTFILE_NAME = FALSE;
static BOOL ERR_NO_FILENAME_PREFIX = FALSE;


void show_helpmsg_exit() {
  if (ERR_NO_INPUT_FILE) {
    printf("no input file\n");
  }

  if (ERR_UNKNOWN_SWITCH_O) {
    printf("unknown parameter to switch -O. %s",
           "choices are -O0, -O1, -O2, -O3.\n");
  }

  if (ERR_UNKNOWN_SWITCH_O_USE_LR0) {
    printf("switch -O cannot be used together with ");
    printf("-P (--lane-tracing-pgm), \n");
    printf("-Q (--lane-tracing-ltt), -R (--lalr1) or -S (--lr0).\n");
  }

  if (ERR_UNKNOWN_SWITCH_D) {
    printf("unknown parameter to switch -D. %s",
           "choices are -Di (i = 0 ~ 14).\n");
  }

  if (ERR_NO_OUTFILE_NAME) {
    printf("output file name is not specified by -o or --output-file==\n");
  }

  if (ERR_NO_FILENAME_PREFIX) {
    printf("file name prefix is not specified by -b or --file-prefix==\n");
  }

  printf("Usage: hyacc [-bcCdDghlmnOPQRStvV] inputfile\n");
  printf("Man page: hyacc -m\n");
  exit(0);
}


/*
 * Get output file name if -o or --outfile-name==
 * switches are used.
 */
void get_outfile_name(int len, char * name) {
  if (len <= 0 || strlen(name) == 0) {
    ERR_NO_OUTFILE_NAME = TRUE;
    show_helpmsg_exit();
  }
  //printf("len = %d, name = %s\n", len, name);
  y_tab_c = (char *) malloc(sizeof(char) * (len + 3));
  sprintf(y_tab_c, "%s.c", name);
  y_tab_h = (char *) malloc(sizeof(char) * (len + 3));
  sprintf(y_tab_h, "%s.h", name);
  y_output = (char *) malloc(sizeof(char) * (len + 8));
  sprintf(y_output, "%s.output", name);
  y_gviz = (char *) malloc(sizeof(char) * (len + 6));
  sprintf(y_gviz, "%s.gviz", name);
}


/*
 * Get output file name if -b or --file_prefix==
 * switches are used.
 */
void get_filename_prefix(int len, char * name) {
  if (len <= 0 || strlen(name) == 0) {
    ERR_NO_OUTFILE_NAME = TRUE;
    show_helpmsg_exit();
  }
  //printf("len = %d, name = %s\n", len, name);
  y_tab_c = (char *) malloc(sizeof(char) * (len + 7));
  sprintf(y_tab_c, "%s.tab.c", name);
  y_tab_h = (char *) malloc(sizeof(char) * (len + 7));
  sprintf(y_tab_h, "%s.tab.h", name);
  y_output = (char *) malloc(sizeof(char) * (len + 8));
  sprintf(y_output, "%s.output", name);
  y_gviz = (char *) malloc(sizeof(char) * (len + 6));
  sprintf(y_gviz, "%s.gviz", name);
}


void write_options(int infile_index, char ** argv) {
  //exit(0);
}


void init_options() {
  USE_COMBINE_COMPATIBLE_CONFIG = TRUE;

  // default: -O1
  USE_COMBINE_COMPATIBLE_STATES = TRUE;
  USE_REMOVE_UNIT_PRODUCTION = FALSE;
  USE_REMOVE_REPEATED_STATES = FALSE;

  SHOW_GRAMMAR = FALSE;
  SHOW_PARSING_TBL = FALSE;
  DEBUG_GEN_PARSING_MACHINE = FALSE;
  DEBUG_COMB_COMP_CONFIG = FALSE;
  DEBUG_BUILD_MULTIROOTED_TREE = FALSE;
  DEBUG_REMOVE_UP_STEP_1_2 = FALSE;
  DEBUG_REMOVE_UP_STEP_4 = FALSE;
  SHOW_TOTAL_PARSING_TBL_AFTER_RM_UP = FALSE;
  SHOW_THEADS = FALSE;
  USE_GENERATE_COMPILER = TRUE;
  PRESERVE_UNIT_PROD_WITH_CODE = FALSE;
  DEBUG_HASH_TBL = FALSE;
  SHOW_SS_CONFLICTS = FALSE;
  SHOW_STATE_TRANSITION_LIST = TRUE;
  SHOW_STATE_CONFIG_COUNT = FALSE;
  SHOW_ACTUAL_STATE_ARRAY = FALSE;

  USE_YYDEBUG = FALSE;
  USE_LINES = TRUE;
  USE_VERBOSE = FALSE; /* not implemented in code yet */
  y_tab_c = "y.tab.c";
  y_tab_h = "y.tab.h";
  y_output = "y.output";
  y_gviz = "y.gviz";
  USE_OUTPUT_FILENAME = FALSE;
  USE_FILENAME_PREFIX = FALSE;
  USE_HEADER_FILE = FALSE;
  USE_GRAPHVIZ = FALSE;

  USE_LR0 = FALSE;
  USE_LALR = FALSE;
  USE_LANE_TRACING = FALSE;

  SHOW_ORIGINATORS = FALSE;
}


void set_lalr1() {
  USE_LALR = TRUE;
  USE_LR0 = TRUE;
}


void set_lr0() {
  USE_LR0 = TRUE;
}


/*
 * Use PGM to combine states in the end.
 */
void set_lane_tracing_pgm() {
  USE_LANE_TRACING = TRUE;
  USE_LALR = TRUE;
  USE_LR0 = TRUE;
  USE_COMBINE_COMPATIBLE_STATES = TRUE;
}


/*
 * Use the other (lane-based) method to combine states in the end.
 */
void set_lane_tracing_ltt() {
  USE_LANE_TRACING = TRUE;
  USE_LALR = TRUE;
  USE_LR0 = TRUE;
  USE_COMBINE_COMPATIBLE_STATES = FALSE;
}


void get_single_letter_option(char * s, unsigned int pos) {
  int switch_param = -1;
  int c = s[pos];

  //printf("c = %c\n", c);
  switch(c) {
    case 'b':        /* file name prefix */
              if (argc_pt >= cmd_argc - 1) {
                ERR_NO_FILENAME_PREFIX = TRUE;
                show_helpmsg_exit();
              }
              USE_FILENAME_PREFIX = TRUE;
              break; 
    case 'c':
              USE_GENERATE_COMPILER = FALSE;
              break;
    case 'C':
              PRESERVE_UNIT_PROD_WITH_CODE = TRUE;
              break;
    case 'd':        /* define: create y.tab.h */
              USE_HEADER_FILE = TRUE;
              break; 
    case 'g':        /* create y.gviz */
              USE_GRAPHVIZ = TRUE;
              break;
    case 'h':
              show_helpmsg_exit();
              break;
    case '?':
              show_helpmsg_exit();
              break;
    case 'l':        /* no line directives in y.tab.c */
              USE_LINES = FALSE;
              break; 
    case 'm':        /* show man page */
              show_manpage(); /* in hyacc_path.c */
              exit(0);
              break;
    case 'o':        /* output file name */
              if (argc_pt >= cmd_argc - 1) {
                ERR_NO_OUTFILE_NAME = TRUE;
                show_helpmsg_exit();
              }
              USE_OUTPUT_FILENAME = TRUE;
              break;
    case 'O':        /* optimization. */
              if (strlen(s) <= pos + 1) {
                ERR_UNKNOWN_SWITCH_O = TRUE;
                show_helpmsg_exit();
              }
              if (USE_LR0 == TRUE) {
                ERR_UNKNOWN_SWITCH_O_USE_LR0 = TRUE;
                show_helpmsg_exit();
              }
              sscanf(s+pos+1, "%d", & switch_param);
              switch(switch_param) {
                case 0: 
                        USE_COMBINE_COMPATIBLE_STATES = FALSE;
                        USE_REMOVE_UNIT_PRODUCTION = FALSE;
                        USE_REMOVE_REPEATED_STATES = FALSE;
                        break;
                case 1: 
                        USE_COMBINE_COMPATIBLE_STATES = TRUE;
                        USE_REMOVE_UNIT_PRODUCTION = FALSE;
                        USE_REMOVE_REPEATED_STATES = FALSE;
                        break;
                case 2: 
                        USE_COMBINE_COMPATIBLE_STATES = TRUE;
                        USE_REMOVE_UNIT_PRODUCTION = TRUE;
                        USE_REMOVE_REPEATED_STATES = FALSE;
                        break;
                case 3: /* use all optimizations */
                        USE_COMBINE_COMPATIBLE_STATES = TRUE;
                        USE_REMOVE_UNIT_PRODUCTION = TRUE;
                        USE_REMOVE_REPEATED_STATES = TRUE;
                        break;
                default:
                        ERR_UNKNOWN_SWITCH_O = TRUE;
                        show_helpmsg_exit();
                        break;
              }
              break;
    //case 'p':        /* symbol name prefix */ // to be implemented.
    //          break; 
    case 'P':
              set_lane_tracing_pgm();
              break;
    case 'Q':
              set_lane_tracing_ltt();
              break;
    case 'R':
              set_lalr1();
              break;
    case 'S':
              set_lr0();
              break;
    case 't':        /* debug: output y.parse */
              USE_YYDEBUG = TRUE;
              break;
    case 'v':        /* verbose */
              USE_VERBOSE = TRUE;
              break; 
    case 'V':        /* show version information. */
              print_version();
              exit(0);
              break; 
    case 'D' :       /* debug print options. */
              if (strlen(s) <= pos + 1) {
                ERR_UNKNOWN_SWITCH_D = TRUE;
                show_helpmsg_exit();
              }
              sscanf(s+pos+1, "%d", & switch_param);
              switch(switch_param) {
                case 0:  SHOW_GRAMMAR = TRUE;
                         SHOW_PARSING_TBL = TRUE;
                         DEBUG_GEN_PARSING_MACHINE = TRUE;
                         DEBUG_COMB_COMP_CONFIG = TRUE;
                         DEBUG_BUILD_MULTIROOTED_TREE = TRUE;
                         DEBUG_REMOVE_UP_STEP_1_2 = TRUE;
                         DEBUG_REMOVE_UP_STEP_4 = TRUE;
                         SHOW_TOTAL_PARSING_TBL_AFTER_RM_UP = TRUE;
                         SHOW_THEADS = TRUE;
                         DEBUG_HASH_TBL = TRUE;
                         SHOW_SS_CONFLICTS = TRUE;
                         SHOW_STATE_TRANSITION_LIST = TRUE;
                         SHOW_STATE_CONFIG_COUNT = TRUE;
                         SHOW_ACTUAL_STATE_ARRAY = TRUE;
                         break;
                case 1:  SHOW_GRAMMAR = TRUE; 
                         break;
                case 2:  SHOW_PARSING_TBL = TRUE;
                         break;
                case 3:  DEBUG_GEN_PARSING_MACHINE = TRUE;
                         break;
                case 4:  DEBUG_COMB_COMP_CONFIG = TRUE;
                         break;
                case 5:  DEBUG_BUILD_MULTIROOTED_TREE = TRUE;
                         break;
                case 6:  DEBUG_REMOVE_UP_STEP_1_2 = TRUE;
                         break;
                case 7:  DEBUG_REMOVE_UP_STEP_4 = TRUE;
                         break;
                case 8:  SHOW_TOTAL_PARSING_TBL_AFTER_RM_UP = TRUE;
                         break;
                case 9:  SHOW_THEADS = TRUE;
                         break;
                case 10: DEBUG_HASH_TBL = TRUE; 
                         break;
                case 11: SHOW_SS_CONFLICTS = TRUE;
                         break;
                case 12: SHOW_STATE_TRANSITION_LIST = FALSE;
                         break;
                case 13: SHOW_STATE_CONFIG_COUNT = TRUE;
                         break;
                case 14: SHOW_ACTUAL_STATE_ARRAY = TRUE;
                         break;
                case 15: SHOW_ORIGINATORS = TRUE;
                         break;
                default:
                         ERR_UNKNOWN_SWITCH_D = TRUE;
                         show_helpmsg_exit();
                         break;
              }
              USE_VERBOSE = TRUE; /* allow write to y.output */
              break;
    default:         /* ignore other switches. */
              if (! (c >= '0' && c <= '9')) { /* not a digit */
                printf("unknown switch %c\n", c);
                exit(1);
              }
              break; 
  }
}


void get_mnemonic_long_option(char * s) {
  if (strcmp(s, "--debug") == 0) { // -t
    USE_YYDEBUG = TRUE;
  } else if (strcmp(s, "--defines") == 0) { // -d
    USE_HEADER_FILE = TRUE;
  } else if (strcmp(s, "--no-compiler") == 0) { // -c
    USE_GENERATE_COMPILER = FALSE;
  } else if (strcmp(s, "--keep-unit-production-with-action") == 0) { // -C
    PRESERVE_UNIT_PROD_WITH_CODE = FALSE;
  } else if (strncmp(s, "--file-prefix==", 15) == 0) { // -b
    // 15 for strlen("--file-prefix=="). 
    get_filename_prefix(strlen(s) - 15, s + 15);
  } else if (strcmp(s, "--graphviz") == 0) { // -g
    USE_GRAPHVIZ = TRUE;
  } else if (strcmp(s, "--help") == 0) { // -h
    show_helpmsg_exit();
  } else if (strcmp(s, "--lalr1") == 0) { // -a
    set_lalr1();
  } else if (strcmp(s, "--lane-tracing-pgm") == 0) { // -P
    //set_lane_tracing_no_pgm();
    set_lane_tracing_pgm();
  } else if (strcmp(s, "--lane-tracing-ltt") == 0) { // -Q
    set_lane_tracing_ltt();
  } else if (strcmp(s, "--lr0") == 0) { // -r
    set_lr0();
  } else if (strcmp(s, "--man-page") == 0) { // -m
    show_manpage(); /* in hyacc_path.c */
    exit(0);
  //} else if (strncmp(s, "--name-prefix==", 15) == 0) { // -p
  // to be implemented.
  } else if (strcmp(s, "--no-lines") == 0) { // -l
    USE_LINES = FALSE;
  } else if (strncmp(s, "--output-file==", 15) == 0) { // -o
    // 15 for strlen("--output-file=="). 
    get_outfile_name(strlen(s) - 15, s + 15);
  } else if (strcmp(s, "--verbose") == 0) { // -v
    USE_VERBOSE = TRUE;
  } else if (strcmp(s, "--version") == 0) { // -V
    print_version();
    exit(0);
  } else {
    printf("unknown switch: %s\n", s);
    exit(1);
  }
}


/*
 * Note:
 *   optimization 1: combine compatible states.
 *   optimization 2: remove unit productions after 1.
 *   optimization 3: further remove repeated states after 2.
 */
int get_options(int argc, char ** argv) {
  int i, pos, len;
  int infile_index = -1;

  if ((cmd_argc = argc) == 1) {
    ERR_NO_INPUT_FILE = TRUE; show_helpmsg_exit(); 
  }

  init_options();

  for (i = 1; i < argc; i ++) {
    argc_pt = i;
    //printf("%d, %s\n", i, argv[i]);
    len = strlen(argv[i]);

    if (USE_OUTPUT_FILENAME) { // get output file name.
      get_outfile_name(len, argv[i]);
      USE_OUTPUT_FILENAME = FALSE;
      continue;
    } 
    if (USE_FILENAME_PREFIX) { // -b
      get_filename_prefix(len, argv[i]);
      USE_FILENAME_PREFIX = FALSE;
      continue;
    }

    if (len >= 2 && argv[i][0] == '-' && argv[i][1] == '-') {
      get_mnemonic_long_option(argv[i]);
    } else if (argv[i][0] == '-') {
      //printf ("single letter switch %s\n", argv[i]);
      for (pos = 1; pos < len; pos ++) {
        get_single_letter_option(argv[i], pos);
      }
    } else {
      if (infile_index == -1) {
        infile_index = i;
        //printf("file to open: %s\n", argv[i]);
      }
    }
  }

  if (infile_index == -1) {
    ERR_NO_INPUT_FILE = TRUE;
    show_helpmsg_exit();
  }

  write_options(infile_index, argv);
  return infile_index;
}

