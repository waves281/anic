#ifndef _MAIN_DEFS_H_
#define _MAIN_DEFS_H_

// standard includes

#include<iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <stack>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::ifstream;
using std::ofstream;
using std::ostream;
using std::string;
using std::vector;
using std::map;
using std::stack;

// global variable linkage specifiers

extern bool verboseOutput;
extern bool silentMode;
extern int optimizationLevel;
extern int tabModulus;
extern bool eventuallyGiveUp;

#endif
