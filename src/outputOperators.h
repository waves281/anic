#ifndef _OUTPUT_OPERATORS_H_
#define _OUTPUT_OPERATORS_H_

#include "globalDefs.h"
#include "constantDefs.h"

#include "lexer.h"
#include "parser.h"
#include "semmer.h"

// string vector streaming operator overloads
ostream &operator<< (ostream &os, Token &t);
ostream &operator<< (ostream &os, Tree *&tree);
ostream &operator<< (ostream &os, SymbolTree *st);
ostream &operator<< (ostream &os, Type *type);
ostream &operator<< (ostream &os, const TypeStatus &status);

#endif
