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

#define print(s) cout << s << "\n";
#define printLabel(s) cout << "\n" << s << "\n";
#define printNotice(s) cout << PROGRAM_STRING << ": " << s << ".\n";
#define printError(s) cerr << PROGRAM_STRING << ": ERROR: " << s << ".\n"
#define printWarning(s) cerr << PROGRAM_STRING << ": warning: " << s << ".\n"

#define lexerError(fn,r,c,str) cerr << PROGRAM_STRING << ": ERROR: LEXER: " << fn << ":" << r << ":" << c << ": " << str << ".\n"; \
	lexerErrorCode++; \
	if (lexerEventuallyGiveUp && lexerErrorCode >= TOLERABLE_ERROR_LIMIT) { printError("too many errors, giving up"); return NULL; }

#define parserError(fn,r,c,str) cerr << PROGRAM_STRING << ": ERROR: PARSER: " << fn << ":" << r << ":" << c << ": " << str << ".\n"; \
	parserErrorCode++; \
	if (parserEventuallyGiveUp && parserErrorCode >= TOLERABLE_ERROR_LIMIT) { printError("too many errors, giving up"); return parserErrorCode; }

#define semmerError(fn,r,c,str,rv) cerr << PROGRAM_STRING << ": ERROR: SEMMER: " << fn << ":" << r << ":" << c << ": " << str << ".\n"; \
	semmerErrorCode++; \
	if (semmerEventuallyGiveUp && semmerErrorCode >= TOLERABLE_ERROR_LIMIT) { printError("too many errors, giving up"); return rv; }

#endif
