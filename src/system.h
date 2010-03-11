#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "mainDefs.h"
#include "constantDefs.h"

void printHeader(void);
void printUsage(void);
void printSeeAlso(void);
void printHelp(void);
void printLink(void);
void die(int errorCode);
void die(void);

#define print(s) if (!silentMode) { cout << s << "\n"; }
#define printLabel(s) if (!silentMode) { cout << "\n" << s << "\n"; }
#define printNotice(s) if (!silentMode) { cout << PROGRAM_STRING << ": " << s << ".\n"; }
#define printError(s) if (!silentMode) { cerr << PROGRAM_STRING << ": ERROR: " << s << ".\n"; }
#define printWarning(s) if (!silentMode) { cerr << PROGRAM_STRING << ": warning: " << s << ".\n"; }

#define lexerError(fn,r,c,str) if (!silentMode) { cerr << PROGRAM_STRING << ": ERROR: LEXER: " << fn << ":" << r << ":" << c << ": " << str << ".\n"; } \
	lexerErrorCode++; \
	if (lexerEventuallyGiveUp && lexerErrorCode >= TOLERABLE_ERROR_LIMIT) { printError("too many errors, giving up"); return NULL; }

#define parserError(fn,r,c,str) if (!silentMode) { cerr << PROGRAM_STRING << ": ERROR: PARSER: " << fn << ":" << r << ":" << c << ": " << str << ".\n"; } \
	parserErrorCode++;

#define semmerError(fn,r,c,str) if (!silentMode) { cerr << PROGRAM_STRING << ": ERROR: SEMMER: " << fn << ":" << r << ":" << c << ": " << str << ".\n"; } \
	semmerErrorCode++;

#endif
