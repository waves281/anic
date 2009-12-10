#include <stdio.h>
#include <string.h>
#include <map>
using std::map;
#include <string>
using std::string;

#include "../src/mainDefs.h"
#include "../src/constantDefs.h"

#define NUM_RULES 1024

// parses the generated parse table into an includable .h file with the appropriate struct representation
int main() {
	// input file
	FILE *in;
	in = fopen("./var/parserTable.txt","r");
	if (in == NULL) { // if file open failed, return an error
		return -1;
	}
	// output file
	FILE *out;
	out = fopen("./var/parserStruct.h","w");
	if (out == NULL) { // if file open failed, return an error
		return -1;
	}
	// print the necessary prologue into the output file
	fprintf(out, "#ifndef _PARSER_STRUCT_H_\n");
	fprintf(out, "#define _PARSER_STRUCT_H_\n");
	fprintf(out, "#include \"../src/parser.h\"\n\n");

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
	string token; // temportart token string
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
			fprintf(out, "#define %s MAX_TOKENS + %d\n", token.c_str(), nonTermCount);
			nonTermCount++;
		}
		// push the string to the token ordering map
		tokenOrder.insert( make_pair(tokenOrder.size(), token) );
	}
	fprintf(out, "\n");

	// print struct header
	fprintf(out, "#define PARSER_STRUCT \\\n");
	fprintf(out, "static int ruleLength[%d]; \\\n", NUM_RULES);
	fprintf(out, "static ParserNode parserNode[%d][NUM_TOKENS]; \\\n", NUM_RULES);
	fprintf(out, "static int parserNodesInitialized = 0; \\\n");
	fprintf(out, "if (!parserNodesInitialized) { \\\n");

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
	for (unsigned int i=0; true; i++) { // oer-rule line loop
		// first, discard the junk before the rule's RHS
		char junk[MAX_STRING_LENGTH];
		fscanf(in, "%s", junk); // scan the first token of the line
		if (junk[0] == 'N') { // break if we've reached the end of the rule set
			break;
		}
		fscanf(in, "%s %s", junk, junk); // scan and throw away the next 2 tokens in the line
		// now, count the number of elements on the RHS
		int rhsElements = 0;
		while (fscanf(in, "%s", junk) && junk[0] != '(') {
			rhsElements++;
		}
		// then, log the size of the rhs in the static array
		fprintf(out, "\truleLength[%d] = %d; \\\n", i, rhsElements);
		// finally, throw away the rest of the line
		fgets(lineBuf, MAX_STRING_LENGTH, in);
	}
	fprintf(out, "\t\\\n");

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
		lbCur += strlen(junk)+1; // advance past the state number
		// parse out the state number from the string
		fromState = atoi(junk);
		// now, read all of the transitions for this state
		for(unsigned int i=0; i<tokenOrder.size(); i++) {
			if (sscanf(lbCur, "%s", junk) < 1) { // read a transition
				break;
			}
			lbCur += strlen(junk)+1;
			// branch based on the type of transition action it is
			if (junk[0] == 's') { // shift action
				fprintf(out, "\tparserNode[%d][%s] = (ParserNode){ %s, %s }; \\\n", fromState, tokenOrder[i].c_str(), "ACTION_SHIFT", (junk+1) );
			} else if (junk[0] == 'r') { // reduce action
				fprintf(out, "\tparserNode[%d][%s] = (ParserNode){ %s, %s }; \\\n", fromState, tokenOrder[i].c_str(), "ACTION_REDUCE", (junk+1) );
			} else if (junk[0] == 'a') { // accept action
				fprintf(out, "\tparserNode[%d][%s] = (ParserNode){ %s, %s }; \\\n", fromState, tokenOrder[i].c_str(), "ACTION_ACCEPT", "0" );
			} else if (junk[0] == 'g') { // goto action
				fprintf(out, "\tparserNode[%d][%s] = (ParserNode){ %s, %s }; \\\n", fromState, tokenOrder[i].c_str(), "ACTION_GOTO", (junk+1) );
			} else if (junk[0] == '0') { // error action
				fprintf(out, "\tparserNode[%d][%s] = (ParserNode){ %s, %s }; \\\n", fromState, tokenOrder[i].c_str(), "ACTION_ERROR", "0" );
			}
		}
	}
	// print out the epilogue
	fprintf(out, "\t\\\n");
	fprintf(out, "\tparserNodesInitialized = 1; \\\n");
	fprintf(out, "} \n\n");
	fprintf(out, "#endif\n");
	// finally, return normally
	return 0;
}
