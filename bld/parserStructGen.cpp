#include "../src/mainDefs.h"
#include "../src/constantDefs.h"

#define NUM_RULES 1024

// parses the generated parse table into an includable .h file with the appropriate struct representation
int main() {
	// input file
	FILE *in;
	in = fopen("./tmp/parserTable.txt","r");
	if (in == NULL) { // if file open failed, return an error
		return -1;
	}
	// output files
	FILE *out;
	out = fopen("./tmp/parserStruct.h","w");
	if (out == NULL) { // if file open failed, return an error
		return -1;
	}
	FILE *out2;
		out2 = fopen("./tmp/parserStruct.cpp","w");
		if (out2 == NULL) { // if file open failed, return an error
			return -1;
	}
	// print the necessary prologue into the .h
	fprintf(out, "#ifndef _PARSER_STRUCT_H_\n");
	fprintf(out, "#define _PARSER_STRUCT_H_\n\n");
	fprintf(out, "#include \"../src/parser.h\"\n\n");
	fprintf(out, "#define NUM_RULES %d\n\n", NUM_RULES);

	// now, process the input file
	char lineBuf[MAX_STRING_LENGTH]; // line data buffer

	// first, scan ahead to the parser table
	bool tableFound = false;
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the file, break out of the loop
			break;
		}
		if (strcmp(lineBuf,"--Parsing Table--\n") == 0) { // if we've found the beginning of the parse table, break out of the loop
			tableFound = true;
			break;
		}
	}
	// if we couldn't find a parse table block, return with an error
	if (!tableFound) {
		return -1;
	}

	// now, extract the token ordering from the header row
	// read the header into the line buffer
	char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
	if (retVal == NULL) { // if we've reached the end of the file, return an error code
		return -1;
	}
	// discard the initial "State"
	char *lbCur = lineBuf;
	char junk[MAX_STRING_LENGTH];
	sscanf(lbCur, "%s", junk);
	lbCur += strlen(junk)+1; // scan past the "State"
	// read in the token ordering itself
	bool parsingNonTerms = false; // whether we have reached the nonterminal part yet
	unsigned int nonTermCount = 0;
	string token; // temportary token string
	map<unsigned int, string> tokenOrder;
	for(;;) {
		if (sscanf(lbCur, "%s", junk) < 1) {
			break;
		}
		lbCur += strlen(junk)+1;
		// allocate a string with this token in it
		token = "TOKEN_";
		// $end token special-casing
		if (strcmp(junk, "$end") == 0) {
			token += "END";
		} else {
			token += junk;
		}
		if (token == "TOKEN_Program") {
			parsingNonTerms = true;
		}
		if (parsingNonTerms) {
			fprintf(out, "#define %s NUM_TOKENS + %d\n", token.c_str(), (1 + nonTermCount));
			nonTermCount++;
		}
		// push the string to the token ordering map
		tokenOrder.insert( make_pair(tokenOrder.size(), token) );
	}
	// print out the definition of the number of nonterminals
	fprintf(out, "\n");
	fprintf(out, "#define NUM_NONTERMS %d\n\n", nonTermCount);
	// print out the definition for the total label count
	fprintf(out, "#define NUM_LABELS NUM_TOKENS + NUM_NONTERMS\n\n");
	// print out the epilogue into the .h
	fprintf(out, "void parserInit( unsigned int ruleRhsLength[NUM_RULES],\n");
	fprintf(out, "\t\tint ruleLhsTokenType[NUM_RULES],\n");
	fprintf(out, "\t\tconst char *ruleLhsTokenString[NUM_RULES],\n");
	fprintf(out, "\t\tParserNode parserNode[NUM_RULES][NUM_TOKENS + NUM_NONTERMS] );\n\n");
	fprintf(out, "#endif\n");

	// print the necessary prologue into the .cpp
	fprintf(out2, "#include \"parserStruct.h\"\n\n");
	// print out parserInit to the .cpp
	fprintf(out2, "void parserInit( unsigned int ruleRhsLength[NUM_RULES],\n");
	fprintf(out2, "\t\tint ruleLhsTokenType[NUM_RULES],\n");
	fprintf(out2, "\t\tconst char *ruleLhsTokenString[NUM_RULES],\n");
	fprintf(out2, "\t\tParserNode parserNode[NUM_RULES][NUM_TOKENS + NUM_NONTERMS] ) {\n\n");
	// initialize all parser nodes to error conditions
	fprintf(out2, "\tfor (unsigned int i=0; i < NUM_RULES; i++) {\n");
	fprintf(out2, "\t\tfor (unsigned int j=0; j < (NUM_TOKENS + NUM_NONTERMS); j++) {\n");
	fprintf(out2, "\t\t\tparserNode[i][j].action = ACTION_ERROR;\n");
	fprintf(out2, "\t\t}\n");
	fprintf(out2, "\t}\n");
	fprintf(out2, "\t\n");

	// now, back up in the file and scan ahead to the rule declarations
	fseek(in, 0, SEEK_SET);
	bool rulesFound = false;
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the file, break out of the loop
			break;
		}
		if (strcmp(lineBuf,"Rules: \n") == 0) { // if we've found the beginning of the rules, break out of the loop
			rulesFound = true;
			break;
		}
	}
	// if we couldn't find a rule block, return with an error
	if (!rulesFound) {
		return -1;
	}

	// get rule lengths
	for (unsigned int i=0; true; i++) { // per-rule line loop
		// read in a line
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the file, break out of the loop
			return -1;
		}
		char *lbCur = lineBuf;
		// discard the junk before the rule's lhs
		char junk[MAX_STRING_LENGTH];
		sscanf(lbCur, "%s", junk); // scan the first token of the line
		if (junk[0] == 'N') { // break if we've reached the end of the rule set
			break;
		}
		// scan over to the next token
		lbCur += strlen(junk);
		while (lbCur[0] == ' ' || lbCur[0] == 't') {
			lbCur++;
		}
		// scan in the lhs of the rule
		sscanf(lbCur, "%s", junk);
		string lhs(junk); // log the lhs in a string wrapper
		// scan over to the next token
		lbCur += strlen(junk);
		while (lbCur[0] == ' ' || lbCur[0] == 't') {
			lbCur++;
		}
		sscanf(lbCur, "%s", junk); // scan and throw away the next "->" token in the line
		// scan over to the next token
		lbCur += strlen(junk);
		while (lbCur[0] == ' ' || lbCur[0] == 't') {
			lbCur++;
		}
		// now, count the number of elements on the RHS
		int rhsElements = 0;
		for(;;) {
			if (sscanf(lbCur, "%s", junk) < 1 || junk[0] == '(') { // break wif we reach the end of the line
				break;
			}
			// scan over to the next token
			lbCur += strlen(junk);
			while (lbCur[0] == ' ' || lbCur[0] == 't') {
				lbCur++;
			}
			rhsElements++;
		}
		// then, log the lhs tokenType, lhs tokenString and rhs size of the rule in the corresponding arrays
		if (lhs != "$accept") {
			fprintf(out2, "\truleLhsTokenType[%d] = TOKEN_%s;\n", i, lhs.c_str());
			fprintf(out2, "\truleLhsTokenString[%d] = \"%s\";\n", i, lhs.c_str());
		}
		fprintf(out2, "\truleRhsLength[%d] = %d;\n", i, rhsElements);
	}
	fprintf(out2, "\n");

	// now, scan ahead to the parse table
	tableFound = false;
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the file, break out of the loop
			break;
		}
		if (strcmp(lineBuf,"--Parsing Table--\n") == 0) { // if we've found the beginning of the parse table, break out of the loop
			tableFound = true;
			break;
		}
	}
	if (!tableFound) { // if we couldn't find a parse table block, return with an error
		return -1;
	} else { // otherwise, eat the header line and return an error if this fails
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the file, break out of the loop
			return -1;
		}
	}

	// finally, process the raw parse table actions
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL || lineBuf[0] == 'N') { // if we've reached the end of the table, break out of the loop
			break;
		}
		// the line was valid, so now try parsing the data out of it
		int fromState;
		// first, read the state number
		char *lbCur = lineBuf;
		sscanf(lbCur, "%s", junk);
		// advance past the current token
		lbCur += strlen(junk);
		while(lbCur[0] == ' ' || lbCur[0] == '\t') {
			lbCur++;
		}
		// parse out the state number from the string
		fromState = atoi(junk);
		// now, read all of the transitions for this state
		for(unsigned int i=0; i<tokenOrder.size(); i++) {
			sscanf(lbCur, "%s", junk); // read a transition
			// advance past the current token
			lbCur += strlen(junk);
			while(lbCur[0] == ' ' || lbCur[0] == '\t') {
				lbCur++;
			}
			// branch based on the type of transition action it is
			if (junk[0] == 's') { // shift action
				fprintf(out2, "\tparserNode[%d][%s] = (ParserNode){ %s, %d };\n", fromState, tokenOrder[i].c_str(), "ACTION_SHIFT", atoi(junk+1) );
			} else if (junk[0] == 'r') { // reduce action
				fprintf(out2, "\tparserNode[%d][%s] = (ParserNode){ %s, %d };\n", fromState, tokenOrder[i].c_str(), "ACTION_REDUCE", atoi(junk+1) );
			} else if (junk[0] == 'a') { // accept action
				fprintf(out2, "\tparserNode[%d][%s] = (ParserNode){ %s, %d };\n", fromState, tokenOrder[i].c_str(), "ACTION_ACCEPT", 0 );
			} else if (junk[0] == 'g') { // goto action
				fprintf(out2, "\tparserNode[%d][%s] = (ParserNode){ %s, %d };\n", fromState, tokenOrder[i].c_str(), "ACTION_GOTO", atoi(junk+1) );
			}
			// do nothing on error states; all states have been defaulted to ACTION_ERROR already
		}
	}
	// print out the epilogue into the .cpp
	fprintf(out2, "}\n");
	// finally, return normally
	return 0;
}
