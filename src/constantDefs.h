#ifndef _CONSTANT_DEFS_H_
#define _CONSTANT_DEFS_H_

#define PROGRAM_STRING "anic"
#define LANGUAGE_STRING "ANI"
#define VERSION_STRING "0.55"
#define VERSION_YEAR "2009"

#define HEADER_LITERAL PROGRAM_STRING<<" -- "<<LANGUAGE_STRING<<" v.["<<VERSION_STRING<<"."<<BUILD_NUMBER_MAIN<<"."<<BUILD_NUMBER_SUB<<"] Compiler (©) "<<VERSION_YEAR<<" Kajetan Adrian Biedrzycki\n" // compiler header
#define USAGE_LITERAL "\tusage: "<<PROGRAM_STRING<<" {<sourceFile>} [-] [-o <outputFile>] [-p <optimizationLevel>] [-s] [-v] [-e] [-h]\n" // info literal

#define VERBOSE_OUTPUT_DEFAULT false
#define OPTIMIZATION_LEVEL_DEFAULT 0
#define EVENTUALLY_GIVE_UP_DEFAULT true

#define MIN_OPTIMIZATION_LEVEL 0
#define MAX_OPTIMIZATION_LEVEL 5

#define TOLERABLE_ERROR_LIMIT 256

#define MAX_STRING_LENGTH (sizeof(char)*4096)
#define MALLOC_STRING ((char *)malloc(MAX_STRING_LENGTH))

#define STD_IN "stdin"

#define STANDARD_LIBRARY_PREFIX "std"

#define ESCAPE_CHARACTER '\\'

#define MAX_TOKEN_LENGTH INT_MAX

#define VERBOSE(s) if (verboseOutput) {s}

#endif
