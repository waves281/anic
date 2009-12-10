#include <stdio.h>

#include "../src/constantDefs.h"

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
	fprintf(out, "static ParserNode parserNode[1024][NUM_TOKENS]; \\\n");
	fprintf(out, "static int parserNodesInitialized = 0; \\\n");
	fprintf(out, "if (!parserNodesInitialized) { \\\n");
	// now, process the input file
	// data buffers
	char lineBuf[MAX_STRING_LENGTH];
	int fromState;
	int tokenType;
	int action;
	int n;
	// read the lexer data
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the line, break out of the loop
			break;
		}
		// the line was valid, so now try parsing the data out of it
		int retVal2 = sscanf(lineBuf, "%d %d %d %d", &fromState, &tokenType, &action, &n);
		if (retVal2 >= 0 && retVal2 <= 3) { // if it was a blank/incomplete line, skip it
			continue;
		} else if (retVal2 == 4) { // else if it was a valid data line, process it normally
			fprintf(out, "\tparserNode[%d][%d] = (ParserNode){ %d, %d }; \\\n", fromState, tokenType, action, n);
		}
	}
	// print out the epilogue
	fprintf(out, "\tparserNodesInitialized = 1; \\\n");
	fprintf(out, "} \n");
	fprintf(out, "#endif\n");
	// finally, return normally
	return 0;
}
