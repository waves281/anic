#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "constantDefs.h"

int printHeader(void);
int printUsage(void);
int printHelp(void);
void die(int errorCode);
void die(void);

#define print(s) cout << PROGRAM_STRING << ": " << s << ".\n";
#define printError(s) cerr << PROGRAM_STRING << ": ERROR: " << s << ".\n"
#define printWarning(s) cerr << PROGRAM_STRING << ": warning: " << s << ".\n"

#define printLexerError(s) cerr << PROGRAM_STRING << ": ERROR: LEXER: " << s << ".\n"; lexerErrorCode = 1

#endif
