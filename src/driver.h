#ifndef _DRIVER_H_
#define _DRIVER_H_

#include "globalDefs.h"
#include "constantDefs.h"

void printHeader(void);
void printUsage(void);
void printSeeAlso(void);
void printHelp(void);
void printLink(void);
const string &getFileName(unsigned int fileIndex);
void die(int errorCode);
void die(void);

#define print(s) if (!silentMode) { cout << s << "\n"; }
#define printLabel(s) if (!silentMode) { cout << "\n" << s << "\n"; }
#define printNotice(s) if (!silentMode) { cout << PROGRAM_STRING << ": " << s << ".\n"; }
#define printError(s) if (!silentMode) { cerr << PROGRAM_STRING << ": ERROR: " << s << ".\n"; }
#define printWarning(s) if (!silentMode) { cerr << PROGRAM_STRING << ": warning: " << s << ".\n"; }

#define GET_FILE_NAME(fi) ((fi != STANDARD_LIBRARY_FILE_INDEX) ? inFileNames[fi] : STANDARD_LIBRARY_FILE_NAME)

#define lexerError(fi,r,c,str) if (!silentMode) { cerr << PROGRAM_STRING << ": ERROR: LEXER: " << GET_FILE_NAME(fi) << ":" << r << ":" << c << ": " << str << ".\n"; } \
	lexerErrorCode++; \
	if (eventuallyGiveUp && lexerErrorCode >= TOLERABLE_ERROR_LIMIT) { printError("too many errors, giving up"); return NULL; }

#define parserError(fi,r,c,str) if (!silentMode) { cerr << PROGRAM_STRING << ": ERROR: PARSER: " << GET_FILE_NAME(fi) << ":" << r << ":" << c << ": " << str << ".\n"; } \
	parserErrorCode++;

#define semmerError(fi,r,c,str) if (!silentMode) { cerr << PROGRAM_STRING << ": ERROR: SEMMER: " << GET_FILE_NAME(fi) << ":" << r << ":" << c << ": " << str << ".\n"; } \
	semmerErrorCode++;

#endif
