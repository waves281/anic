#ifndef _CONSTANT_DEFS_H_
#define _CONSTANT_DEFS_H_

#define PROGRAM_STRING "anic"
#define VERSION_STRING "0.05"
#define VERSION_YEAR "2009"

#define HEADER_LITERAL PROGRAM_STRING<<" -- ANI v.["<<VERSION_STRING<<"."<<BUILD_NUMBER_MAIN<<"."<<BUILD_NUMBER_SUB<<"] Compiler (©) "<<VERSION_YEAR<<" Kajetan Adrian Biedrzykci\n" // compiler header
#define USAGE_LITERAL "\tusage: "<<PROGRAM_STRING<<" [<sourceFile>]* [-o <outputFile>] [-p <optimizationLevel>] [-h] [-s] [-v]\n" // info literal

#define VERBOSE_OUTPUT_DEFAULT 0

#define OPTIMIZATION_LEVEL_DEFAULT 0
#define MIN_OPTIMIZATION_LEVEL 0
#define MAX_OPTIMIZATION_LEVEL 5

#define MAX_STRING_LENGTH sizeof(char)*1024
#define MALLOC_STRING ((char *)malloc(MAX_STRING_LENGTH))

#define VERBOSE(s) if (verboseOutput) print(s)

#endif
