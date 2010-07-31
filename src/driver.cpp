#include "driver.h"

#include "globalDefs.h"
#include "constantDefs.h"

#include "outputOperators.h"

#include "lexer.h"
#include "parser.h"
#include "semmer.h"
#include "genner.h"

// global variables

int optimizationLevel = DEFAULT_OPTIMIZATION_LEVEL;
bool verboseOutput = VERBOSE_OUTPUT_DEFAULT;
bool silentMode = SILENT_MODE_DEFAULT;
int tabModulus = TAB_MODULUS_DEFAULT;
bool eventuallyGiveUp = EVENTUALLY_GIVE_UP_DEFAULT;

// header print functions

void printHeader(void) {
	cerr << HEADER_LITERAL;
}

void printUsage(void) {
	cerr << USAGE_LITERAL;
}

void printSeeAlso(void) {
	cerr << SEE_ALSO_LITERAL;
}

void printLink(void) {
	cerr << LINK_LITERAL;
}

void printHelp(void) {
	printHeader();
	printUsage();
	printSeeAlso();
	printLink();
}

// system termination functions

void die(int errorCode) {
	if (errorCode) {
		printError("fatal error code " << errorCode << " -- stop");
	}
	exit(errorCode);
}

void die(void) {
	die(0);
}

// core helper functions

unsigned int nextUniqueInt = 0;
unsigned int getUniqueInt() {
	return (nextUniqueInt++);
}

int containsString(vector<string> inFileNames, string &s) {
	for (unsigned int i=0; i < inFileNames.size(); i++) { // scan the vector for matches
		if (inFileNames[i] == s) { // if we have a match at this index, return true
			return 1;
		}
	}
	// else if we did not find any matches, return false
	return 0;
}

// main driver function

int main(int argc, char **argv) {
	// verify arguments
	if (argc == 1) {
		printHelp();
		die();
	}
	// now, parse the command-line arguments
	vector<ifstream *> inFiles; // source file vector
	vector<string> inFileNames; // source file name vector
	string outFileName(OUTPUT_FILE_DEFAULT); // initialize the output file name
	// handled flags for each option
	bool oHandled = false;
	bool pHandled = false;
	bool vHandled = false;
	bool sHandled = false;
	bool tHandled = false;
	bool eHandled = false;
	for (int i=1; i<argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] != '\0') { // option argument
			if (argv[i][1] == 'o' && argv[i][2] == '\0' && !oHandled) { // output file name
				if (++i >= argc) { // jump to the next argument, test if it doesn't exist
					printError("-o expected file name argument");
					die();
				}
				outFileName = argv[i];
				// flag this option as handled
				oHandled = true;
			} else if (argv[i][1] == 'p' && argv[i][2] == '\0' && !pHandled) { // optimization level option
				if (++i >= argc) { // jump to the next argument, test if it doesn't exist
					printError("-p expected optimization level argument");
					die();
				}
				int n;
				if (sscanf(argv[i], "%d", &n) < 1) { // unsuccessful attempt to extract a number out of the argument
					printError("-p got illegal optimization level '" << argv[i] << "'");
					die();
				} else { // else attempt was successful
					if (n >= MIN_OPTIMIZATION_LEVEL && n <= MAX_OPTIMIZATION_LEVEL) {
						optimizationLevel = n;
					} else {
						printError("-p got out-of-bounds optimization level " << n);
						die();
					}
				}
				// flag this option as handled
				pHandled = true;
			} else if (argv[i][1] == 'v' && argv[i][2] == '\0' && !vHandled && !sHandled) { // verbose output option
				verboseOutput = true;
				VERBOSE (
					printNotice("verbose output enabled");
					print("");
				)
				// flag this option as handled
				vHandled = true;
			} else if (argv[i][1] == 's' && argv[i][2] == '\0' && !vHandled && !sHandled) { // silent compilation option
				silentMode = true;
				// flag this option as handled
				sHandled = true;
			} else if (argv[i][1] == 't' && argv[i][2] == '\0' && !tHandled) {
				if (++i >= argc) { // jump to the next argument, test if it doesn't exist
					printError("-t expected tab modulus argument");
					die();
				}
				int n;
				if (sscanf(argv[i], "%d", &n) < 1) { // unsuccessful attempt to extract a number out of the argument
					printError("-t got illegal tab modulus '" << argv[i] << "'");
					die();
				} else { // else attempt was successful
					if (n >= MIN_TAB_MODULUS && n <= MAX_TAB_MODULUS) {
						tabModulus = n;
					} else {
						printError("-t got out-of-bounds tab modulus " << n);
						die();
					}
				}
				// flag this option as handled
				tHandled = true;
			} else if (argv[i][1] == 'e' && argv[i][2] == '\0' && !eHandled && !sHandled) {
				eventuallyGiveUp = false;
				// flag this option as handled
				eHandled = true;
			} else if (argv[i][1] == 'h' && argv[i][2] == '\0' && argc == 2) {
				// test to see if a command interpreter is available
				int systemRetVal = system(NULL);
				if (!systemRetVal) {
					printError("cannot display manual page: no command interpreter available");
					die(1);
				}
				// invoke the program's man page
				systemRetVal = system("man anic 2> /dev/null");
				// test if displaying the manual page failed
				if (systemRetVal) {
					printError("cannot display manual page: executing manual driver failed");
					die(1);
				}
				die(0);
			} else {
				printWarning("confused by option '" << argv[i] << "', skipping");
			}

		} else { // default case; assume regular file argument
			string fileName;
			if (argv[i][0] == '-') {
				fileName = STD_IN_FILE_NAME;
			} else {
				fileName = argv[i];
			}
			if (containsString(inFileNames, fileName)) {
				printWarning("including file '" << fileName << "' multiple times");
				continue;
			}
			ifstream *inFile = (fileName == STD_IN_FILE_NAME) ? NULL : new ifstream(fileName.c_str()); // create a stream for this file
			if (inFile != NULL && !inFile->good()) { // if file open failed
				printError("cannot open input file '" << fileName << "'");
				die();
			} else { // else if file open succeeded, add the file and its name to the appropriate vectors
				inFiles.push_back(inFile);
				inFileNames.push_back(fileName);
			}
		}
	}

	// terminate if there were no input files
	if (inFiles.empty()) {
		printError("no input files");
		die();
	}

	// lex files
	int lexerError = 0; // error flag
	vector<vector<Token> *> lexemes; // per-file vector of the lexemes that the lexer is about to generate
	for (unsigned int i=0; i<inFiles.size(); i++) {
		// check file arguments
		string fileName(inFileNames[i]);
		if (fileName == "-") {
			fileName = STD_IN_FILE_NAME;
		}
		VERBOSE(
			printNotice("lexing file \'" << fileName << "\'...");
		)
		// do the actual lexing
		int thisLexError = 0; // one-shot error flag
		vector<Token> *lexeme = lex(inFiles[i], fileName);
		if (lexeme == NULL) { // if lexing failed with an error, log the error condition
			thisLexError = 1;
		} else { // else if lexing was successful, log the lexeme to the vector
			lexemes.push_back(lexeme);
		}
		// print out the tokens if we're in verbose mode
		VERBOSE(
			if (!thisLexError) {
				printNotice("successfully lexed file \'" << fileName << "\'");
			} else {
				printNotice("failed to lex file \'" << fileName << "\'");
			}
			print(""); // new line
		)
		// log the highest error code that occured
		if (thisLexError > lexerError) {
			lexerError = thisLexError;
		}
	}
	// now, check if lexing failed and if so, kill the system as appropriate
	if (lexerError) {
		die(1);
	}

	// parse lexemes
	int parserError = 0; // error flag
	unsigned int fileIndex = 0; // file name index
	Tree *treeRoot = NULL; // the root parseme of the parse tree
	Tree *treeCur = NULL; // the tail of the linked list of parsemes
	for (vector<vector<Token> *>::iterator lexemeIter = lexemes.begin(); lexemeIter != lexemes.end(); lexemeIter++) {
		string fileName(inFileNames[fileIndex]);
		if (fileName == "-") {
			fileName = STD_IN_FILE_NAME;
		}
		VERBOSE(printNotice("parsing file \'" << fileName << "\'...");)
		// do the actual parsing
		Tree *thisParseme;
		int thisParseError = parse(*lexemeIter, thisParseme, fileName);
		if (thisParseError) { // if parsing failed with an error, log the error condition
			VERBOSE(
				printNotice("failed to parse file \'" << fileName << "\'");
				print(""); // new line
			)
		} else { // else if parsing was successful, latch the parseme into the tree trunk
			if (treeRoot != NULL) { // if this is not the first parseme
				// link in this tree
				*treeCur += thisParseme;
				// to the right
				// to the left
				*thisParseme -= treeCur;
				// advance treeCur to the new tail
				treeCur = treeCur->next;
			} else { // else if this is the first parseme
				treeRoot = treeCur = thisParseme;
			}
			VERBOSE(
				printNotice("successfully parsed file \'" << fileName << "\'");
				print(""); // new line
			)
		}
		// log the highest error code that occured
		if (thisParseError > parserError) {
			parserError = thisParseError;
		}
		// advance file name index
		fileIndex++;
	}
	// now, check if parsing failed and if so, kill the system as appropriate
	if (parserError) {
		die(1);
	}

	// perform semantic analysis

	VERBOSE(printNotice("mapping semantics...");)

	// allocate symbol table root (will be filled with user-level definitions during parsing)
	SymbolTable *stRoot;
	// allocate the intermediate representation tree root (will be bound at the end of parsing)
	IRTree *codeRoot;

	int semmerErrorCode = sem(treeRoot, stRoot, codeRoot);
	// now, check if semming failed and kill the system as appropriate
	if (semmerErrorCode) {
		VERBOSE(
			printNotice("failed to map semantics");
			print(""); // new line
		)
	} else {
		VERBOSE(
			printNotice("successfully mapped semantics");
			print(""); // new line
		)
	}
	// now, check if semming failed and if so, kill the system as appropriate
	if (semmerErrorCode) {
		die(1);
	}

	// generate the intermediate code tree

	VERBOSE(printNotice("generating code dump...");)
	
	// allocate the string buffer into which we will dump the resulting assembly code
	string asmString;

	// generate the actual assembly code
	int gennerErrorCode = gen(codeRoot, asmString);
	// now, check if genning failed and kill the system as appropriate
	if (gennerErrorCode) {
		VERBOSE(
			printNotice("failed to generate code dump");
			print(""); // new line
		)
	} else {
		VERBOSE(
			printNotice("successfully generated code dump");
			print(""); // new line
		)
	}
	// now, check if genning failed and if so, kill the system as appropriate
	if (gennerErrorCode) {
		die(1);
	}
	
	// open output file for writing
	ofstream *outFile = new ofstream(outFileName.c_str());
	if (!outFile->good()) {
		printError("cannot open output file '" << outFileName << "'");
		die();
	}

	// terminate the program successfully
	return 0;
}
