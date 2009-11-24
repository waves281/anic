#include "mainDefs.h"
#include "system.h"

#include "lexer.h"
#include "../var/lexerStruct.h"


int isWhiteSpace(unsigned char c) {
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

int isNewLine(unsigned char c) {
	return (c == '\n' || c == '\r');
}

void resetState(int &sSize, int &state, char *&tokenType) {
	// reset the state variables
	sSize = 0;
	state = 0;
	tokenType = "";
	// return normally
	return;
}

void commitToken(char *s, int &sSize, int &state, char *&tokenType, int rowStart, int colStart, vector<Token> *outputVector, char c, char &carryOver) {
	// first, terminate s
	s[sSize] = '\0';
	// then, build up the token
	Token t;
	t.tokenType = tokenType;
	strcpy(t.s, s);
	t.row = rowStart;
	t.col = colStart;
	// now, commit it to the output vector
	outputVector->push_back(t);
	// finally, reset our state back to the default
	resetState(sSize, state, tokenType);
	// also, carry over the current character to the next round
	carryOver = c;
	// finally, return normally
	return;
}

vector<Token> *lex(ifstream *in, char *fileName) {
	// local error code
	int lexerErrorCode = 0;
	// initialize lexer structure
	// LexerNode lexerNode[fromState][charSeen] is hereby defined and usable
	LEXER_STRUCT
	// declare output vector
	vector<Token> *outputVector = new vector<Token>();
	// input character buffers
	char c;
	char carryOver = '\0';
	// output character buffer
	char *s = MALLOC_STRING;
	int sSize = 0;
	// state variables
	int state = 0;
	char *tokenType = "";
	// the position in the file we're currently in
	int row = 1;
	int col = 0;
	int rowStart = -1;
	int colStart = -1;
	// loop flags
	int done = 0;
	for(;;) { // per-character loop

lexerLoopTop: ;
		// get a new character
		if (carryOver != '\0') { // if there is a character to carry over, use it
			c = carryOver;
			carryOver = '\0';
		} else { // otherwise, grab a character from the input
			if ( !(in == NULL ? cin.get(c) : in->get(c)) ) { // if getting a character fails, flag the fact that we're done now
				if (done) { // if this is the second time we're trying to read EOF, break out of the loop
					break;
				}
				done = 1;
				// pretend that there's a newline at the end of the file so the last token can be processed nominally
				c = '\n';
			}
			// either way, we just got a character, so advance the column count
			col++;
		}
		// now, process the character we just got
		// first, check it it was a special character
		if (isWhiteSpace(c)) { // whitespace?
			if (strcmp(tokenType,"ERROR") == 0) { // if we got whitespace space while in error mode,
				printLexerError(fileName,rowStart,colStart,"whitespace-truncated token");
				// throw away this token and continue parsing
				resetState(sSize, state, tokenType);
				carryOver = c;
			} else if (!(strcmp(tokenType,"") == 0)) { // else if we were in a commitable state, commit this token to the output vector
				commitToken(s, sSize, state, tokenType, rowStart, colStart, outputVector, c, carryOver);
			}
			if (isNewLine(c)) { // newline?
				// bump up the row count and carriage return the column
				row++;
				col = 0;
			}
		} else { // else if it was a non-whitepace character, check if there is a valid transition for this state
			LexerNode transition = state == -1 ? lexerNode[255][(unsigned char)c] : lexerNode[state][(unsigned char)c];
			if (transition.valid) { // if the transition is valid
				// first, set rowStart amd colStart if we're coming from the core state
				if (state == 0) {
					rowStart = row;
					colStart = col;
				}
				// second, check if we're jumping into a failure state
				if(strcmp(transition.tokenType,"FAIL") == 0) { // if it's a failure state, print an error, reset, and continue
					// print the error message
					printLexerError(fileName,row,col,"token mangled by \'"<<c<<"\'");
					// also, reset state
					resetState(sSize, state, tokenType);
					// however, carry over the faulting character, as it might be useful for later debugging
					carryOver = c;
					// finally, continue from the top of the loop
					continue;
				}
				// now, branch based on the type of transition it was
				if (strcmp(transition.tokenType,"REGCOMMENT") == 0) { // if it's a transition into regular comment mode
					// first, reset our state
					resetState(sSize, state, tokenType);
					// finally, scan and discard characters up to and including the next newline
					for(;;) { // scan until we hit either EOF or a newline
						bool retVal = (in == NULL ? cin.get(c) : in->get(c));
						if (!retVal) { // if we hit EOF, flag the fact that we're done and jump to the top of the loop
							done = 1;
							goto lexerLoopTop;
						} else if (isNewLine(c)) { // if we hit a newline, break out of this loop and continue normally
							row++;
							col = 0;
							break;
						} else {
							col++;
						}
					}
				} else if (strcmp(transition.tokenType,"STARCOMMENT") == 0) { // else if it's a transition into star comment mode
					// first, reset our state
					resetState(sSize, state, tokenType);
					// next, scan and discard characters up to and including the next * /
					char lastChar = '\0';
					for(;;) { // scan until we hit either EOF or a * /
						bool retVal = (in == NULL ? cin.get(c) : in->get(c));
						col++;
						if (!retVal) { // if we hit EOF, flag a critical comment truncation error and signal that we're done
							printLexerError(fileName,rowStart,colStart,"/* comment truncated by EOF");
							done = 1;
							goto lexerLoopTop;
						} else if (isNewLine(c)) { // if we hit a newline, update the row and col as necessary
							row++;
							col = 0;
						} else if (lastChar == '*' && c == '/') { // else if we've found the end of the comment, simply break out of the loop
							break;
						}
						lastChar = c;
					}
				} else if (strcmp(transition.tokenType,"CQUOTE") == 0 || strcmp(transition.tokenType,"SQUOTE") == 0) { // else if it's a transition into a quoting mode

				} else { // else if it's any other regular valid transition
					if (sSize < (MAX_TOKEN_LENGTH-1)) { // if there is room in the buffer for this character, log it
						s[sSize] = c;
						sSize++;
						tokenType = transition.tokenType;
						state = transition.toState;
					} else { // else if there is no more room in the buffer for this character, discard the token with an error
						printLexerError(fileName,rowStart,colStart,"token overflow");
						// also, reset state and scan to the end of this token
						resetState(sSize, state, tokenType);
						for(;;) {
							bool retVal = (in == NULL ? cin.get(c) : in->get(c));
							// handle newline cursor logging properly
							if (isNewLine(c)) {
								row++;
								col = 0;
							} else {
								col++;
							}
							if (!retVal) { // if we hit the end of the file, flag the fact that we're done and continue lexing
								done = 1;
								goto lexerLoopTop;
							} else if (isWhiteSpace(c)) { // else if it was whitespace, continue lexing normally
								goto lexerLoopTop;
							}
						}
					}
				}
			} else { // else if the transition isn't valid
				if (strcmp(tokenType,"") == 0) { // if there were no valid characters before this junk
					printLexerError(fileName,row,col,"stray character \'"<<c<<"\'");
					// now, reset the state and try to recover by eating up characters until we hit whitespace or EOF
					// reset state
					resetState(sSize, state, tokenType);
					for(;;) {
						bool retVal = (in == NULL ? cin.get(c) : in->get(c));
						// handle newline cursor logging properly
						if (isNewLine(c)) {
							row++;
							col = 0;
						} else {
							col++;
						}
						if (!retVal) { // if we hit the end of the file, flag the fact that we're done and continue lexing
							done = 1;
							goto lexerLoopTop;
						} else if (isWhiteSpace(c)) { // else if it was whitespace, continue lexing normally
							goto lexerLoopTop;
						}
					}
				} else if (strcmp(tokenType,"ERROR") == 0) { // else if it's an invalid transition from an error state, flag it
					// print the error message
					printLexerError(fileName,row,col,"token truncated by \'"<<c<<"\'");
					// also, reset state
					resetState(sSize, state, tokenType);
					// however, carry over the faulting character, as it might be useful for later debugging
					carryOver = c;
				} else { // else if there is a valid commit pending, do it and carry over this character for the next round
					commitToken(s, sSize, state, tokenType, rowStart, colStart, outputVector, c, carryOver);
				}
			}
		}
	}
	// per-character loop is done now
	// delete the output character buffer
	delete s;
	// finally, test the error code to see if we should propagate it up the chain or return normally
	if (lexerErrorCode) {
		// deallocate the output vector, since we're just going to return null
		delete outputVector;
		return NULL;
	} else {
		return outputVector;
	}
}
