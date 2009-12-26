#ifndef _CUSTOM_OPERATORS_H_
#define _CUSTOM_OPERATORS_H_

#include "mainDefs.h"

#include "lexer.h"
#include "../var/lexerStruct.h"

#include "parser.h"
#include "../var/parserStruct.h"

#include "semmer.h"

// string vector streaming operator
ostream &operator<< (ostream &os, Token &t);
ostream &operator<< (ostream &os, SymbolTable *&st);

#endif
