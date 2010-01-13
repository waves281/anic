#ifndef _CONSTANT_DEFS_H_
#define _CONSTANT_DEFS_H_

#define PROGRAM_STRING "anic"
#define LANGUAGE_STRING "ANI"
#define HOME_PAGE "http://anic.googlecode.com/"

#define HEADER_LITERAL PROGRAM_STRING<<" -- "<<LANGUAGE_STRING<<" v.["<<VERSION_STRING<<"."<<VERSION_STAMP<<"] Compiler (c) "<<VERSION_YEAR<<" Kajetan Adrian Biedrzycki\n" /* compiler header */
#define USAGE_LITERAL "\tusage: "<<PROGRAM_STRING<<" {<sourceFile>} [-] [-o <outputFile>] [-p <optimizationLevel>] [-s] [-v] [-e] [-h]\n" /* info literal */
#define LINK_LITERAL "\thome page: "<<HOME_PAGE<<"\n" /* link literal */

#define VERBOSE_OUTPUT_DEFAULT false
#define EVENTUALLY_GIVE_UP_DEFAULT true

#define MIN_OPTIMIZATION_LEVEL 0
#define MAX_OPTIMIZATION_LEVEL 3
#define DEFAULT_OPTIMIZATION_LEVEL 1

#define TOLERABLE_ERROR_LIMIT 256

#define MAX_STRING_LENGTH (sizeof(char)*4096)
#define MALLOC_STRING ((char *)malloc(MAX_STRING_LENGTH))

#define STD_IN_FILE_NAME "stdin"

#define MAX_TOKEN_LENGTH INT_MAX
#define ESCAPE_CHARACTER '\\'

#define STANDARD_LIBRARY_STRING "std"

#define BLOCK_NODE_STRING "_BLOCK_"
#define IMPORT_DECL_STRING "_UNRESOLVED_IMPORT_"
#define STANDARD_IMPORT_DECL_STRING "_STD_IMPORT_"

#define VERBOSE(s) if (verboseOutput) {s}

#endif
