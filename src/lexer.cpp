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


vector<Token> *lex(ifstream *in, char *fileName) {
	// initialize lexer structure
	// LexerNode lexerNode[fromState][charSeen] is hereby defined and usable
	LEXER_STRUCT
	// declare output vector
	vector<Token> *outputVector = new vector<Token>();
	// input character buffers
	unsigned char c;
	unsigned char carryOver = '\0';
	// output character buffer
	char *s = MALLOC_STRING;
	int sSize = 0;
	// state variables
	int state = 0;
	char *tokenType = "";
	// the position in the file we're currently in
	int row = 1;
	int col = 0;
	// loop flags
	int done = 0;
	for(;;) { // per-character loop

lexerLoopTop: ;

		// get a new character
		if (carryOver != '\0') { // if there is a character to carry over, use it
			c = carryOver;
			carryOver = '\0';
		} else { // otherwise, grab a character from the input
			if ( !(*in >> c) ) { // if getting a character fails, flag the fact that we're done now
				if (done) { // if this is the second time we're trying to read EOF, we're done
					break;
				}
				done = 1;
				// pretend there is a newline at the end of the file so the last token can be processed
				c = '\n';
			}
			// either way, we just got a character, so advance the column count
			col++;
		}
		// now, process the character we just got
		// first, check it it was a special character
		if (isWhiteSpace(c)) { // whitespace?
			if (strcmp(tokenType,"ERROR") == 0) { // if we got whitespace space while in error mode,
				printLexerError("token corrupted by '"<<c<<"' at "<<fileName<<":"<<row<<":"<<col);
				// throw away this token and continue parsing
				sSize = 0;
				state = 0;
				tokenType = "";
				carryOver = c;
				goto lexerLoopTop;
			} else if (!(strcmp(tokenType,"") == 0)) { // else if we were in a commitable state, commit this token to the output vector
				goto commitToken;
			}
			if (isNewLine(c)) { // newline?
				// bump up the row count and carriage return the column
				row++;
				col = 0;
			}
		} else { // else if it was a non-whitepace character, check if there is a valid transition for this state
			LexerNode transition = lexerNode[state][c];
			if (transition.valid) { // if the transition is valid, simply log the character
				if (sSize < MAX_TOKEN_LENGTH) { // if there is room in the buffer for this character, log it
					s[sSize] = c;
					sSize++;
					tokenType = transition.tokenType;
					state = transition.toState;
				} else { // else if there is no more room in the buffer for this character, discard the token with an error
					printLexerError("token overflow at "<<fileName<<":"<<row<<":"<<col);
					// also, reset state and scan to the end of this token
					sSize = 0;
					state = 0;
					tokenType = "";
					for(;;) {
						bool retVal = (cin >> c);
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
			} else { // else if the transition isn't valid
				if (strcmp(tokenType,"") == 0) { // if there were no valid characters before this junk
					printLexerError("unexpected character '"<<c<<"' at "<<fileName<<":"<<row<<":"<<col);
					// now, reset the state and try to recover by eating up characters until we hit whitespace or EOF
					// reset state
					sSize = 0;
					state = 0;
					tokenType = "";
					for(;;) {
						bool retVal = (cin >> c);
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
				} else { // else if there is a valid commit pending, do it and carry over this character for the next round
					goto commitToken;
				}
			}
		}
	}
	goto done;

commitToken:
	// first, terminate s
	s[sSize] = '\0';
	// then, build up the token
	Token t;
	t.tokenType = tokenType;
	strcpy(t.s, s);
	t.row = row;
	t.col = col;
	// now, commit it to the output vector
	outputVector->push_back(t);
	// finally, reset our state back to the default
	sSize = 0;
	state = 0;
	tokenType = "";
	// carry over the current character to the next round
	carryOver = c;
	goto lexerLoopTop;

done: ;
	// delete the output character buffer
	delete s;
	// finally, return the output vector we just created
	return outputVector;
}
