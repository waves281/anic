#include "lexer.h"

#include "outputOperators.h"

// lexer-global variables

int lexerErrorCode;

// Token functions
Token::Token(int tokenType) : tokenType(tokenType), fileIndex(STANDARD_LIBRARY_FILE_INDEX), row(0), col(0) {}
Token::Token(int tokenType, const string &s, unsigned int fileIndex, int row, int col) : tokenType(tokenType), s(s), fileIndex(fileIndex), row(row), col(col) {}
Token::Token(const Token &otherToken) : tokenType(otherToken.tokenType), s(otherToken.s), fileIndex(otherToken.fileIndex), row(otherToken.row), col(otherToken.col) {}
Token::~Token() {}
Token &Token::operator=(Token &otherToken) {tokenType = otherToken.tokenType; s = otherToken.s; fileIndex = otherToken.fileIndex; row = otherToken.row; col = otherToken.col; return *this;}

// main lexing functions

int isWhiteSpace(unsigned char c) {
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

int isNewLine(unsigned char c) {
	return (c == '\n');
}

int isTab(unsigned char c) {
	return (c == '\t');
}

void resetState(string &s, int &state, int &tokenType) {
	// reset the state variables
	s.clear(); // clear the raw token buffer
	state = 0;
	tokenType = -1;
	// return normally
	return;
}

void commitToken(string &s, int &state, int &tokenType, unsigned int fileIndex, int rowStart, int colStart, vector<Token> *outputVector, char c) {
	// first, build up the token
	Token t(tokenType, s, fileIndex, rowStart, colStart);
	// now, commit it to the output vector
	outputVector->push_back(t);
	// finally, reset our state back to the default
	resetState(s, state, tokenType);
	// finally, return normally
	return;
}

string hex(unsigned char c) {
	string retVal;
	char hexDigits[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	retVal += (hexDigits[(c>>4)&0xF]);
	retVal += (hexDigits[c&0xF]);
	return retVal;
}

// discard input up until the end of the current token
void discardToken(ifstream *in, char c, int &row, int &col, bool &done) {
	for(;;) {
		bool retVal = (in == NULL ? cin.get(c) : in->get(c));
		// handle newline cursor logging properly
		if (isNewLine(c)) {
			row++;
			col = 0;
		} else if (isTab(c)) {
			col = (col - (col % tabModulus) + tabModulus);
		} else {
			col++;
		}
		if (!retVal) { // if we hit the end of the file, flag the fact that we're done and continue lexing
			done = true;
			return;
		} else if (isWhiteSpace(c)) { // else if it was whitespace, continue lexing normally
			return;
		}
	}
}

vector<Token> *lex(ifstream *in, unsigned int fileIndex) {

	// initialize error variables
	lexerErrorCode = 0;

	// declare and initialize lexer structure
#include "../tmp/lexerNodeRaw.h"
	// declare output vector
	vector<Token> *outputVector = new vector<Token>();
	// input character buffers
	char c;
	char carryOver = '\0';
	// output character buffer
	string s;
	// state variables
	int state = 0;
	int tokenType = -1;
	// the position in the file we're currently in
	int row = 1;
	int col = 0;
	int rowStart = -1;
	int colStart = -1;
	// loop flags
	bool done = false;
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
				done = true;
				// pretend that there's a newline at the end of the file so the last token can be processed nominally
				c = '\n';
			}
			// either way, we just got a character, so advance the column count
			if (isTab(c)) {
				col = (col - (col % tabModulus) + tabModulus);
			} else {
				col++;
			}
		}
		// now, process the character we just got
		// first, check it it was a special character
		if (isWhiteSpace(c)) { // whitespace?
			if (tokenType == TOKEN_ERROR) { // if we got whitespace space while in error mode,
				lexerError(fileIndex,rowStart,colStart,"whitespace-truncated token");
				// throw away this token and continue parsing
				resetState(s, state, tokenType);
				carryOver = c;
			} else if (tokenType != -1) { // else if we were in a commitable state, commit this token to the output vector
				commitToken(s, state, tokenType, fileIndex, rowStart, colStart, outputVector, c);
			}
			if (isNewLine(c)) { // newline?
				// bump up the row count and carriage return the column
				row++;
				col = 0;
			}
		} else { // else if it was a non-whitepace character, check if there is a valid transition for this state
			LexerNode transition = state == -1 ? lexerNode[255][(unsigned char)c] : lexerNode[state][(unsigned char)c];
			if (transition.tokenType != -1) { // if the transition is valid
				// first, set rowStart amd colStart if we're coming from the core state
				if (state == 0) {
					rowStart = row;
					colStart = col;
				}
				// second, check if we're jumping into a failure state
				if(transition.tokenType == TOKEN_FAIL) { // if it's a failure state, print an error, reset, and continue
					// print the error message
					lexerError(fileIndex,row,col,"token mangled by stray character 0x"<<hex(c));
					// also, reset state
					resetState(s, state, tokenType);
					// however, carry over the faulting character, as it might be useful for later debugging
					carryOver = c;
					// finally, continue from the top of the loop
					continue;
				}
				// now, branch based on the type of transition it was
				if (transition.tokenType == TOKEN_REGCOMMENT) { // if it's a transition into regular comment mode
					// first, reset our state
					resetState(s, state, tokenType);
					// finally, scan and discard characters up to and including the next newline
					for(;;) { // scan until we hit either EOF or a newline
						bool retVal = (in == NULL ? cin.get(c) : in->get(c));
						if (!retVal) { // if we hit EOF, flag the fact that we're done and jump to the top of the loop
							done = true;
							goto lexerLoopTop;
						} else if (isNewLine(c)) { // if we hit a newline, break out of this loop and continue normally
							row++;
							col = 0;
							break;
						} else if (isTab(c)) {
							col = (col - (col % tabModulus) + tabModulus);
						} else {
							col++;
						}
					}
				} else if (transition.tokenType == TOKEN_STARCOMMENT) { // else if it's a transition into star comment mode
					// first, reset our state
					resetState(s, state, tokenType);
					// next, scan and discard characters up to and including the next * /
					char lastChar = '\0';
					for(;;) { // scan until we hit either EOF or a * /
						bool retVal = (in == NULL ? cin.get(c) : in->get(c));
						if (isTab(c)) {
							col = (col - (col % tabModulus) + tabModulus);
						} else {
							col++;
						}
						if (!retVal) { // if we hit EOF, flag a critical comment truncation error and signal that we're done
							lexerError(fileIndex,rowStart,colStart,"/* comment truncated by EOF");
							done = true;
							goto lexerLoopTop;
						} else if (isNewLine(c)) { // if we hit a newline, update the row and col as necessary
							row++;
							col = 0;
						} else if (lastChar == '*' && c == '/') { // else if we've found the end of the comment, simply break out of the loop
							break;
						}
						lastChar = c;
					}
				} else if (transition.tokenType == TOKEN_CQUOTE || transition.tokenType == TOKEN_SQUOTE) { // else if it's a transition into a quoting mode
					// log the type of transition we're making
					tokenType = transition.tokenType;
					// pre-decide the terminal that should signal the end of the quote
					char termChar = (tokenType == TOKEN_CQUOTE) ? '\'' : '\"';
					// whether the last character seen was the escape character
					bool lastCharWasEsc = false;
					for(;;) { // scan until we hit either EOF or the termChar
						bool retVal = (in == NULL ? cin.get(c) : in->get(c));
						if (isTab(c)) {
							col = (col - (col % tabModulus) + tabModulus);
						} else {
							col++;
						}
						if (!retVal) { // if we hit EOF, flag a critical comment truncation error and signal that we're done
							if (termChar == '\'') {
								lexerError(fileIndex,rowStart,colStart,"character literal truncated by EOF");
							} else {
								lexerError(fileIndex,rowStart,colStart,"string literal truncated by EOF");
							}
							done = true;
							goto lexerLoopTop;
						}
						// escape character handling
						if (c == ESCAPE_CHARACTER && !lastCharWasEsc) { // if it's the escape character and not a double, log this and wait for the next character
							lastCharWasEsc = true;
							// continue so the character isn't logged, and the condition is unflagged
							continue;
						} else if (lastCharWasEsc) { // else if the last character was an escape character, specially handle this one
							// then, branch based on the type of escape it is
							if (c == 'a') { // bell character
								c = '\a';
							} else if (c == 'b') { // backspace
								c = '\b';
							} else if (c == 't') { // tab
								c = '\t';
							} else if (c == 'n') { // tab
								c = '\n';
							} else if (c == 'v') { // vertical tab
								c = '\v';
							} else if (c == 'f') { // from feed
								c = '\f';
							} else if (c == 'r') { // carriage return
								c = '\r';
							} else if (c == '0') { // null character
								c = '\0';
							} else if (c == '\'') { // c-quote
								// nothing, the below code will properly process it
							} else if (c == '\"') { // s-quote
								// nothing, the below code will properly process it
							} else if (c == ESCAPE_CHARACTER) { // escape-escape
								// nothing, the below code will properly process it
							} else if (c == '\n') { // newline escape
								row++;
								col = 0;
								// unflag the condition
								lastCharWasEsc = false;
								// continue so the character isn't logged
								continue;
							} else { // else if it's an unrecognized escape sequence, throw an error and discard the character
								lexerError(fileIndex,row,col-1,"unrecognized escape sequence "<<ESCAPE_CHARACTER<<"0x"<<hex(c));
								// unflag the condition
								lastCharWasEsc = false;
								// continue so the character isn't logged
								continue;
							}
						}
						// termination detection
						if (!lastCharWasEsc) { // if we don't need special forced commiting of this character due to escaping
							if (isNewLine(c)) { // if we hit a newline, throw a quote truncation error
								if (termChar == '\'') {
									lexerError(fileIndex,rowStart,colStart,"character literal truncated by end of line");
								} else {
									lexerError(fileIndex,rowStart,colStart,"string literal truncated by end of line");
								}
								row++;
								col = 0;
								goto lexerLoopTop;
							} else if (c == termChar) { // else if we've found the end of the quote
								if (termChar == '\'' && s.size() > 1) { // if this is an overflowing CQUOTE, throw a CQUOTE overflow error
									lexerError(fileIndex,rowStart,colStart,"character literal overflow");
								}
								// either way, commit the token and continue with processing
								commitToken(s, state, tokenType, fileIndex, rowStart, colStart, outputVector, c);
								break;
							}
						} else { // else if we *do* need to force the character to commit due to escaping
							// unflag the escape character condition now, since we've passed all of its dependencies
							lastCharWasEsc = false;
						}

						// character logging
						if (s.size() < (MAX_TOKEN_LENGTH-1)) { // else if there is room in the buffer for this character, log it
							s += c;
						} else { // else if there is no more room in the buffer for this character, discard the token with an error
							lexerError(fileIndex,rowStart,colStart,"quoted literal overflow");
							// also, reset state and scan to the end of this token
							resetState(s, state, tokenType);
							discardToken(in, c, row, col, done);
							// finally, break out of the quote loop
							break;
						} // if there is room in the buffer
					} // for (;;)
				} else { // else if it's any other regular valid transition
					if (s.size() < (MAX_TOKEN_LENGTH-1)) { // if there is room in the buffer for this character, log it
						s += c;
						tokenType = transition.tokenType;
						state = transition.toState;
					} else { // else if there is no more room in the buffer for this character, discard the token with an error
						lexerError(fileIndex,rowStart,colStart,"token overflow");
						// also, reset state and scan to the end of this token
						resetState(s, state, tokenType);
						discardToken(in, c, row, col, done);
					}
				}
			} else { // else if the transition isn't valid
				if (tokenType == -1) { // if there were no valid characters before this junk
					lexerError(fileIndex,row,col,"stray character 0x"<<hex(c));
					// now, reset the state and try to recover by eating up characters until we hit whitespace or EOF
					// reset state
					resetState(s, state, tokenType);
					discardToken(in, c, row, col, done);
				} else if (tokenType == TOKEN_ERROR) { // else if it's an invalid transition from an error state, flag it
					// print the error message
					lexerError(fileIndex,row,col,"token truncated by stray character 0x"<<hex(c));
					// also, reset state
					resetState(s, state, tokenType);
					// however, carry over the faulting character, as it might be useful for later debugging
					carryOver = c;
				} else { // else if there is a valid commit pending, do it and carry over this character for the next round
					commitToken(s, state, tokenType, fileIndex, rowStart, colStart, outputVector, c);
					// also, carry over the current character to the next round
					carryOver = c;
				}
			}
		}
	}

	// per-character loop is done now

	// finally, test the error code to see if we should propagate it up the chain or return normally
	if (lexerErrorCode) {
		// deallocate the output vector, since we're just going to return null
		delete outputVector;
		return NULL;
	} else {
		// augment the vector with the end token
		string eofString("EOF");
		Token termToken(TOKEN_END, eofString, fileIndex, 0, 0);
		outputVector->push_back(termToken);
		// print out the lexeme if we're in verbose mode
		VERBOSE(
			for (unsigned int tokenIndex = 0; tokenIndex < outputVector->size(); tokenIndex++) {
				cout << (*outputVector)[tokenIndex] << " " ;
			} // per-token loop
			cout << "\n";
		)
		// finally, return the vector to the caller
		return outputVector;
	}
}
