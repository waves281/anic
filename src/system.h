#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "mainDefs.h"
#include "constantDefs.h"

int printHeader(void);
int printUsage(void);
int printHelp(void);
void die(int errorCode);
void die(void);

#define print(s) cout << s << "\n";
#define printLabel(s) cout << "\n" << s << "\n";
#define printNotice(s) cout << PROGRAM_STRING << ": " << s << ".\n";
#define printError(s) cerr << PROGRAM_STRING << ": ERROR: " << s << ".\n"
#define printWarning(s) cerr << PROGRAM_STRING << ": warning: " << s << ".\n"

#define printLexerError(fn,r,c,str) cerr << PROGRAM_STRING << ": ERROR: LEXER: " << fn << ":" << r << ":" << c << ": " << str << ".\n"; \
	lexerErrorCode++; \
	if (eventuallyGiveUp && lexerErrorCode >= TOLERABLE_ERROR_LIMIT) { printError("too many errors, giving up"); return NULL; }
#define printParserError(fn,r,c,str) cerr << PROGRAM_STRING << ": ERROR: PARSER: " << fn << ":" << r << ":" << c << ": " << str << ".\n"; \
	parserErrorCode++; \
	if (eventuallyGiveUp && parserErrorCode >= TOLERABLE_ERROR_LIMIT) { printError("too many errors, giving up"); return NULL; }

#endif
