#ifndef _NAMER_H_
#define _NAMER_H_

#include "constantDefs.h"

#include "lexer.h"
#include "../var/lexerStruct.h"

#include "parser.h"
#include "../var/parserStruct.h"

class SymbolTable {

};

SymbolTable *name(Tree *rootParseme, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp);

#endif
