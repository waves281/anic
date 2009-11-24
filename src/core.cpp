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
	// handled flags for each option
	int vHandled = 0;
	int pHandled = 0;
	for (int i=1; i<argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] != '\0') { // option argument
			if (strcmp((argv[i] + 1),"v") == 0 && !vHandled) { // verbose output option
				verboseOutput = 1;
				VERBOSE(print("verbose output enabled");)
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
			} else { // else if file open succeeded, add the file to the vector
				inFiles.push_back(inFile);
			}
		}
	}
	// terminate if there were no input files
	if (inFiles.empty()) {
		printError("no input files");
		die();
	}

	// now, being with lexing the input files
	for (unsigned int i=0; i<inFiles.size(); i++) {
		VERBOSE(print("lexing file '" << argv[i+1] << "'...");)
		vector<Token> *tl = lex(inFiles[i], argv[i+1]);
		if (tl == NULL) { // check if lexing failed with an error
			die(1);
		}
	}

	// terminate the program successfully
	return 0;
}
