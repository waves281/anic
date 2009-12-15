#include "mainDefs.h"
#include "constantDefs.h"
#include "globalVars.h"
#include "system.h"

#include "lexer.h"
#include "../var/lexerStruct.h"

#include "parser.h"
#include "../var/parserStruct.h"

int containsString(vector<char *>inFileNames, char *s) {
	for (unsigned int i=0; i<inFileNames.size(); i++) { // scan the vector for matches
		if (strcmp(inFileNames[i], s) == 0) { // if we have a match at this index, return true
			return 1;
		}
	}
	// else if we did not find any matches, return false
	return 0;
}

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
				fileName = STD_IN;
			}
			if (containsString(inFileNames, fileName)) {
				printWarning("including file '" << fileName << "' multiple times");
				continue;
			}
			ifstream *inFile = (strcmp(fileName,STD_IN) == 0) ? NULL : new ifstream(fileName); // create a stream for this file
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
		char *fileName = inFileNames[i];
		if (strcmp(fileName,"-") == 0) {
			fileName = "stdin";
		}
		VERBOSE(
			cout << "\n";
			printNotice("lexing file \'" << fileName << "\'..");
		)
		// do the actual lexing
		vector<Token> *lexeme = lex(inFiles[i], fileName, verboseOutput, optimizationLevel);
		if (lexeme == NULL) { // if lexing failed with an error, log the error condition
			lexerError = 1;
		} else { // else if lexing was successful, log the lexeme to the vector
			lexemes.push_back(lexeme);
		}
		// print out the tokens if we're in verbose mode
		VERBOSE(
			if (lexeme != NULL) {
				for (unsigned int tokenIndex = 0; tokenIndex < lexeme->size(); tokenIndex++) {
					Token tokenCur = (*lexeme)[tokenIndex];
					cout << "[" << tokenType2String(tokenCur.tokenType) << " " << tokenCur.s << " (" << tokenCur.row << "," << tokenCur.col << ")] " ;
				} // per-token loop
				cout << "\n";
			}
			if (!lexerError) {
				printNotice("successfully lexed file \'" << fileName << "\'");
			} else {
				printNotice("failed to lex file \'" << fileName << "\'");
			}
			print(""); // new line
		)
	}
	// now, check if lexing failed and kill the system as appropriate
	if (lexerError) {
		die(1);
	}

	// parse lexemes
	int parserError = 0; // error flag
	unsigned int fileIndex = 0; // file name index
	vector<Tree *> parsemes; // per-file vector of the parsemes that the parser is about to generate
	for (vector<vector<Token> *>::iterator lexemeIter = lexemes.begin(); lexemeIter != lexemes.end(); lexemeIter++) {
		char *fileName = inFileNames[fileIndex];
		if (strcmp(fileName,"-") == 0) {
			fileName = "stdin";
		}
		VERBOSE(printNotice("parsing file \'" << fileName << "\'..");)
		// do the actual parsing
		Tree *parseme = parse(*lexemeIter, fileName, verboseOutput, optimizationLevel);
		if (parseme == NULL) { // if parsing failed with an error, log the error condition
			parserError = 1;
		} else { // else if parsing was successful, log the parseme to the vector
			parsemes.push_back(parseme);
		}
		VERBOSE(
			if (parseme != NULL && parseme->t.tokenType == TOKEN_Program) {
				printNotice("successfully parsed file \'" << fileName << "\'");
			} else {
				printNotice("failed to parse file \'" << fileName << "\'");
			}
			print(""); // new line
		)
		// advance file name index
		fileIndex++;
	}
	// now, check if parsing failed and kill the system as appropriate
	if (parserError) {
		die(1);
	}

	// parsing is done now, so now merge the generated parsemes into a unified root
	vector<Tree *>::iterator parsemeIter = parsemes.begin();
	// log the root parseme as the Program node of the first parseme (which is guaranteed to exist)
	Tree *rootParseme = *parsemeIter; // Program
	// advance to the second parseme
	parsemeIter++;
	// set parsemeCur to the first Pipes node with a NULL child
	Tree *parsemeCur = rootParseme->child; // child of Program (Pipes)
	while (parsemeCur->child != NULL && parsemeCur->child->next != NULL) { // while the child of the Pipes node in non-NULL and we can go deeper
		parsemeCur = parsemeCur->child->next->next;
	}
	for (; parsemeIter != parsemes.end(); parsemeIter++) {
		Tree *thisTreeRoot = *parsemeIter; // the root of the next tree we're examining
		Tree *handle = thisTreeRoot->child; // the handle of the next tree we're examining
		// unlink the next tree's root, then delete it
		*thisTreeRoot *= 0;
		*handle &= 0;
		delete thisTreeRoot;
		// link the handle into the global parse tree
		if (parsemeCur->child == NULL) { // if the cursor's child is NULL, plug in the handle directly
			*parsemeCur *= handle;
			*handle &= parsemeCur;
		} else if (parsemeCur->child->next == NULL) { // else if the cursor is at a Pipes -> Pipe production, hackishly link in the new node
			Token t;
			t.tokenType = TOKEN_SEMICOLON;
			t.s = "";
			t.row = -1;
			t.col = -1;
			// create the linking SEMICOLON node
			Tree *semiColonNode = new Tree(t, handle, parsemeCur->child, NULL, NULL);
			// link in from the right
			*(parsemeCur->child) += semiColonNode;
			// link in from the left
			*handle -= semiColonNode;
		}
		// finally, advance the parseme cursor appropriately
		while (parsemeCur->child != NULL && parsemeCur->child->next != NULL) { // while the child of the Pipes node in non-NULL and we can go deeper
			parsemeCur = parsemeCur->child->next->next;
		}
	}

	// terminate the program successfully
	return 0;
}
