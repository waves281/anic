#ifndef _CONSTANT_DEFS_H_
#define _CONSTANT_DEFS_H_

#define VERSION_STRING "0.04"
#define VERSION_YEAR "2009"

#define HEADER_LITERAL "ani -- ANI v.["<<VERSION_STRING<<"."<<BUILD_NUMBER_MAIN<<"."<<BUILD_NUMBER_SUB<<"] Compiler (©) "<<VERSION_YEAR<<" Kajetan Adrian Biedrzykci\n" // compiler header
#define USAGE_LITERAL "\tusage: ani [<sourceFile>]* [-o <outputFile>] [-p <optimizationLevel>] [-h] [-s] [-v]\n" // info literal

#define VERBOSE_OUTPUT_DEFAULT 0

#define OPTIMIZATION_LEVEL_DEFAULT 0
#define MIN_OPTIMIZATION_LEVEL 0
#define MAX_OPTIMIZATION_LEVEL 5

#endif
