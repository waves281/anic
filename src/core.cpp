#include "mainDefs.h"
#include "constantDefs.h"
#include "globalVars.h"
#include "system.h"

#include "lexer.h"
#include "../var/lexerStruct.h"

int main(int argc, char **argv) {
	// verify arguments
	if (argc == 1) {
		printHelp();
		die();
	}
	// now, parse the command-line arguments
	vector<ifstream *> inFiles; // source file vector
	vector<char *> inFileNames; // source file name vector
	// handled flags for each option
	int vHandled = 0;
	int pHandled = 0;
	for (int i=1; i<argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] != '\0') { // option argument
			if (strcmp((argv[i] + 1),"v") == 0 && !vHandled) { // verbose output option
				verboseOutput = 1;
				VERBOSE(printNotice("verbose output enabled");)
				// flag this option as handled
				vHandled = 1;
			} else if  (strcmp((argv[i] + 1),"p") == 0 && !pHandled) { // optimization level option
				if (++i >= argc) { // jump to the next argument, test if it doesn't exist
					printError("-p expected optimization level");
					die();
				}
				int n;
				if (sscanf(argv[i], "%d", &n) < 1) { // unsuccessful attempt to extract a number out of the argument
					printError("-p got illegal optimization level '" << n << "'");
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
				pHandled = 1;
			} else {
				printWarning("confused by option '" << argv[i] << "', skipping");
			}

		} else { // default case; assume regular file argument
			char *fileName = argv[i];
			if (argv[i][0] == '-') {
				fileName = "stdin";
			}
			ifstream *inFile = (strcmp(fileName,"stdin") == 0) ? NULL : new ifstream(fileName); // create a stream for this file
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

	// now, being with lexing the input files
	int lexerError = 0; // error flag
	vector<vector<Token> *> lexemes; // per-file vector of the lexemes that the lexer is about to generate
	for (unsigned int i=0; i<inFiles.size(); i++) {
		// check file arguments
		char *fileName = inFileNames[i];
		if (strcmp(fileName,"-") == 0) {
			fileName = "stdin";
		}
		VERBOSE(printNotice("lexing file \'" << fileName << "\'..");)
		// do the actual lexing
		vector<Token> *lexeme = lex(inFiles[i], fileName);
		if (lexeme == NULL) { // if lexing failed with an error, log the error condition
			lexerError = 1;
		} else { // else if lexing was successful, log the lexeme to the vector
			lexemes.push_back(lexeme);
		}
	}
	// now, check if lexing failed and kill the system as appropriate
	if (lexerError) {
		die(1);
	}

	// print out the lexemes if we're in verbose mode
	VERBOSE(
		for (unsigned int fileIndex = 0; fileIndex < lexemes.size(); fileIndex++) {
			char *fileName = inFileNames[fileIndex];
			if (strcmp(fileName,"-") == 0) {
				fileName = "stdin";
			}
			printLabel(fileName << ":");
			vector<Token> *fileVector = lexemes[fileIndex];
			for (unsigned int tokenIndex = 0; tokenIndex < fileVector->size(); tokenIndex++) {
				Token tokenCur = (*fileVector)[tokenIndex];
				cout << "[" << tokenCur.tokenType << " " << tokenCur.s << " (" << tokenCur.row << "," << tokenCur.col << ")] " ;
			} // per-token loop
			cout << "\n";
		} // per-file loop
	) // VERBOSE

	// terminate the program successfully
	return 0;
}
