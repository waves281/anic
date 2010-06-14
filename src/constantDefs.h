#ifndef _CONSTANT_DEFS_H_
#define _CONSTANT_DEFS_H_

#define PROGRAM_STRING "anic"
#define LANGUAGE_STRING "ANI"
#define HOME_PAGE "http://anic.googlecode.com/"

#define HEADER_LITERAL PROGRAM_STRING<<" -- "<<LANGUAGE_STRING<<" Compiler v.["<<VERSION_STRING<<"."<<VERSION_STAMP<<"] (c) "<<VERSION_YEAR<<" Kajetan Adrian Biedrzycki\n" /* compiler header */
#define USAGE_LITERAL "\n\tusage:\t"<<PROGRAM_STRING<<" sourceFile... [-] [-o outputFile] [-p optimizationLevel]\n\t\t[-v] [-s] [-t tabWidth] [-e] [-h]\n" /* info literal */
#define SEE_ALSO_LITERAL "\n\tFor more information, type 'anic -h'.\n" /* see also literal */
#define LINK_LITERAL "\thome page: "<<HOME_PAGE<<"\n" /* link literal */

#define OUTPUT_FILE_DEFAULT "a.out"
#define VERBOSE_OUTPUT_DEFAULT false
#define SILENT_MODE_DEFAULT false
#define EVENTUALLY_GIVE_UP_DEFAULT true

#define MIN_OPTIMIZATION_LEVEL 0
#define MAX_OPTIMIZATION_LEVEL 3
#define DEFAULT_OPTIMIZATION_LEVEL 1

#define MIN_TAB_MODULUS 1
#define MAX_TAB_MODULUS 80
#define TAB_MODULUS_DEFAULT 4

#define TOLERABLE_ERROR_LIMIT 256

#define MAX_STRING_LENGTH (sizeof(char)*4096)

#define STD_IN_FILE_NAME "<stdin>"

#define MAX_TOKEN_LENGTH INT_MAX
#define ESCAPE_CHARACTER '\\'

#define STANDARD_LIBRARY_STRING "std"

#define BLOCK_NODE_STRING "{}"
#define OBJECT_NODE_STRING "{=}"
#define FILTER_NODE_STRING "[]"
#define INSTRUCTOR_NODE_STRING "=[]"
#define OUTSTRUCTOR_NODE_STRING "=[-->]"
#define FAKE_RECALL_NODE_PREFIX ".."
#define IMPORT_DECL_STRING "_UNRESOLVED_IMPORT_"
#define STANDARD_IMPORT_DECL_STRING "_STD_IMPORT_"

#define VERBOSE(s) if (verboseOutput) {s}

#endif
