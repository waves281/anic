#include "mainDefs.h"
#include "constantDefs.h"
#include "system.h"
#include "customOperators.h"

#include "lexer.h"
#include "parser.h"
#include "semmer.h"

// global variables

bool verboseOutput = VERBOSE_OUTPUT_DEFAULT;
bool silentMode = SILENT_MODE_DEFAULT;
int optimizationLevel = DEFAULT_OPTIMIZATION_LEVEL;
bool eventuallyGiveUp = EVENTUALLY_GIVE_UP_DEFAULT;

// core helper functions

int containsString(vector<const char *>inFileNames, const char *s) {
	for (unsigned int i=0; i<inFileNames.size(); i++) { // scan the vector for matches
		if (strcmp(inFileNames[i], s) == 0) { // if we have a match at this index, return true
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
	vector<const char *> inFileNames; // source file name vector
	char outFileName[MAX_STRING_LENGTH]; // output file name
	strcpy(outFileName, OUTPUT_FILE_DEFAULT); // initialize output file name
	// handled flags for each option
	bool oHandled = false;
	bool pHandled = false;
	bool sHandled = false;
	bool vHandled = false;
	bool eHandled = false;
	for (int i=1; i<argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] != '\0') { // option argument
			if (argv[i][1] == 'o' && !oHandled) { // output file name
				if (++i >= argc) { // jump to the next argument, test if it doesn't exist
					printError("-o expected file name argument");
					die();
				}
				strncpy(outFileName, argv[i], strlen(argv[i]) + 1);
				// flag this option as handled
				oHandled = true;
			} else if (argv[i][1] == 'p' && !pHandled) { // optimization level option
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
			} else if (argv[i][1] == 's' && !vHandled && !sHandled) { // silent compilation option
				silentMode = true;
				// flag this option as handled
				sHandled = true;
			} else if (argv[i][1] == 'v' && !vHandled && !sHandled) { // verbose output option
				verboseOutput = true;
				VERBOSE (
					printNotice("verbose output enabled");
					print("");
				)
				// flag this option as handled
				vHandled = true;
			} else if (argv[i][1] == 'e' && !eHandled && !sHandled) {
				eventuallyGiveUp = false;
				// flag this option as handled
				eHandled = true;
			} else if (argv[i][1] == 'h' && argc == 2) {
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
			const char *fileName = argv[i];
			if (argv[i][0] == '-') {
				fileName = STD_IN_FILE_NAME;
			}
			if (containsString(inFileNames, fileName)) {
				printWarning("including file '" << fileName << "' multiple times");
				continue;
			}
			ifstream *inFile = (strcmp(fileName,STD_IN_FILE_NAME) == 0) ? NULL : new ifstream(fileName); // create a stream for this file
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
		const char *fileName = inFileNames[i];
		if (strcmp(fileName,"-") == 0) {
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
	// now, check if lexing failed and kill the system as appropriate
	if (lexerError) {
		die(1);
	}

	// parse lexemes
	int parserError = 0; // error flag
	unsigned int fileIndex = 0; // file name index
	vector<Tree *> parseme[NUM_LABELS]; // per-file vector of the parsemes that the parser is about to generate
	for (vector<vector<Token> *>::iterator lexemeIter = lexemes.begin(); lexemeIter != lexemes.end(); lexemeIter++) {
		const char *fileName = inFileNames[fileIndex];
		if (strcmp(fileName,"-") == 0) {
			fileName = STD_IN_FILE_NAME;
		}
		VERBOSE(printNotice("parsing file \'" << fileName << "\'...");)
		// do the actual parsing
		int thisParseError = parse(*lexemeIter, parseme, fileName);
		if (thisParseError) { // if parsing failed with an error, log the error condition
			VERBOSE(
				printNotice("failed to parse file \'" << fileName << "\'");
				print(""); // new line
			)
		} else { // else if parsing was successful, log the parseme to the vector
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
	// now, check if parsing failed and kill the system as appropriate
	if (parserError) {
		die(1);
	}

	VERBOSE(printNotice("Merging parse trees...");)

	// concatenate the generated trees into one
	// log the tree root as the first program parseme (which is guaranteed to exist)
	Tree *treeRoot = parseme[TOKEN_Program][0]; // Program (there can only be one root node)
	// set treeCur to the initial Program node
	Tree *treeCur = treeRoot; // Program
	for (unsigned int i=1; i < parseme[TOKEN_Program].size(); i++) {
		// log the tree we're about to concatenate on this iteration
		Tree *treeToAdd = parseme[TOKEN_Program][i]; // Program (there can only be one root node)
		// link in this tree
		// to the right
		*treeCur += treeToAdd;
		// to the left
		*treeToAdd -= treeCur;
		// now, advance treeCur to the new tail
		treeCur = treeCur->next;
	}

	VERBOSE(
		printNotice("Parse trees merged successfully");
		print(""); // new line
	)

	// perform semantic analysis

	VERBOSE(printNotice("Mapping semantics...");)

	// allocate symbol table root (will be filled by user-level definitions during parsing)
	SymbolTable *stRoot;

	int semmerErrorCode = sem(treeRoot, parseme, stRoot);
	// now, check if semming failed and kill the system as appropriate
	if (semmerErrorCode) {
		VERBOSE(
			printNotice("Semantic mapping generated inconsistencies");
			print(""); // new line
		)
	} else {
		VERBOSE(
			printNotice("Semantics successfully mapped");
			print(""); // new line
		)
	}
	// now, check if semming failed and kill the system as appropriate
	if (semmerErrorCode) {
		die(1);
	}

	// open output file for writing
	ofstream *outFile = new ofstream(outFileName);
	if (!outFile->good()) {
		printError("cannot open output file '" << outFileName << "'");
		die();
	}

	// terminate the program successfully
	return 0;
}
