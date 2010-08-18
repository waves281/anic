#ifndef _MAIN_DEFS_H_
#define _MAIN_DEFS_H_

// standard includes

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <stack>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::ifstream;
using std::ofstream;
using std::ostream;
using std::string;
using std::vector;
using std::map;
using std::stack;
using std::pair;
using std::make_pair;

// class forward declarations

class Tree;
class SymbolTree;
class Type;
class TypeList;
class StdType;
class FilterType;
class StructorListResult;
class StructorList;
class MemberListResult;
class MemberList;
class ObjectType;
class ErrorType;
class TypeStatus;
class RepTree;
class IRTree;
	class LabelTree;
	class SeqTree;
	class DataTree;
		class WordTree;
		class ArrayTree;
		class TempTree;
		class ReadTree;
	class OpTree;
		class UnOpTree;
		class BinOpTree;
		class ConvTree;
	class CodeTree;
		class LockTree;
		class UnlockTree;
		class CondTree;
		class JumpTree;
		class WriteTree;
		class CopyTree;
		class SchedTree;

// global variable linkage specifiers

extern int optimizationLevel;
extern bool verboseOutput;
extern bool silentMode;
extern int tabModulus;
extern bool eventuallyGiveUp;

extern int lexerErrorCode;
extern int parserErrorCode;
extern int semmerErrorCode;
extern int gennerErrorCode;

extern Type *nullType;
extern Type *errType;
class StdType;
extern StdType *stdBoolType;
extern StdType *stdIntType;
extern StdType *stdFloatType;
extern StdType *stdCharType;
extern StdType *stdStringType;
extern StdType *stdBoolLitType;

extern ObjectType *stringerType;
extern ObjectType *boolUnOpType;
extern ObjectType *intUnOpType;
extern ObjectType *boolBinOpType;
extern ObjectType *intBinOpType;
extern ObjectType *floatBinOpType;
extern ObjectType *boolCompOpType;
extern ObjectType *intCompOpType;
extern ObjectType *floatCompOpType;
extern ObjectType *charCompOpType;
extern ObjectType *stringCompOpType;

// global function forward declarations

unsigned int getUniqueInt();
string getUniqueId();

#endif
