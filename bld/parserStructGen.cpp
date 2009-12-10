#include <stdio.h>
#include <string.h>

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
	fprintf(out, "#define PARSER_STRUCT \\\n");
	fprintf(out, "static int ruleLength[%d]; \\\n", NUM_RULES);
	fprintf(out, "static ParserNode parserNode[%d][NUM_TOKENS]; \\\n", NUM_RULES);
	fprintf(out, "static int parserNodesInitialized = 0; \\\n");
	fprintf(out, "if (!parserNodesInitialized) { \\\n");
	// now, process the input file
	// data buffers
	char lineBuf[MAX_STRING_LENGTH];
	// read the lexer data
	// first, scan ahead to the rule declarations
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
	// now, scan ahead to the parser table
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

// LOLOL the rest isn't done yet!!!!!!!!!
	// now, extract the token ordering from the header row



	// get parse table actions
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the line, break out of the loop
			break;
		}
		// the line was valid, so now try parsing the data out of it
		int fromState;
		int tokenType;
		int action;
		int n;

		int retVal2 = sscanf(lineBuf, "%d %d %d %d", &fromState, &tokenType, &action, &n);
		if (retVal2 >= 0 && retVal2 <= 3) { // if it was a blank/incomplete line, skip it
			continue;
		} else if (retVal2 == 4) { // else if it was a valid data line, process it normally
			fprintf(out, "\tparserNode[%d][%d] = (ParserNode){ %d, %d }; \\\n", fromState, tokenType, action, n);
		}
	}
	// print out the epilogue
	fprintf(out, "\t\\\n");
	fprintf(out, "\tparserNodesInitialized = 1; \\\n");
	fprintf(out, "} \n");
	fprintf(out, "#endif\n");
	// finally, return normally
	return 0;
}
