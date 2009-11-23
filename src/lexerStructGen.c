#include <stdio.h>
#include "constantDefs.h"

// parses the lexer table into an includable .h file with the appropriate struct representation
int main() {
	// input file
	FILE *in;
	in = fopen("./src/lexerTable.txt","r");
	if (in == NULL) { // if file open failed, return an error
		return -1;
	}
	// output file
	FILE *out;
	out = fopen("./var/lexerStruct.h","w");
	if (out == NULL) { // if file open failed, return an error
		return -1;
	}
	// print the necessary prologue into the output file
	fprintf(out, "#ifndef _LEXER_STRUCT_H_\n");
	fprintf(out, "#define _LEXER_STRUCT_H_\n");
	fprintf(out, "#include \"../src/lexer.h\"\n");
	fprintf(out, "#define LEXER_STRUCT \\\n");
	fprintf(out, "static LexerNode lexerNode[256][256]; \\\n");
	fprintf(out, "static int lexerNodesInitialized = 0; \\\n");
	fprintf(out, "if (!lexerNodesInitialized) { \\\n");
	fprintf(out, "\tfor (int i = 0; i<256; i++) { \\\n");
	fprintf(out, "\t\tfor (int j = 0; j<256; j++) { \\\n");
	fprintf(out, "\t\t\tlexerNode[i][j].valid = 0; \\\n");
	fprintf(out, "\t\t} \\\n");
	fprintf(out, "\t} \\\n");
	// now, process the input file
	// data buffers
	char lineBuf[MAX_STRING_LENGTH];
	int fromState;
	char c;
	char tokenType[MAX_STRING_LENGTH];
	int toState;
	// read the lexer data
	for(;;) {
		char *retVal = fgets(lineBuf, MAX_STRING_LENGTH, in);
		if (retVal == NULL) { // if we've reached the end of the line, break out of the loop
			break;
		}
		// the line was valid, so now try parsing the data out of it
		int retVal2 = sscanf(lineBuf, "%d %c %s %d", &fromState, &c, tokenType, &toState);
		if (retVal2 >= 0 && retVal2 <= 3) { // if it was a blank/incomplete line, skip it
			continue;
		} else if (retVal2 == 4) { // else if it was a valid data line, process it normally
			fprintf(out, "\tlexerNode[%d][%d] = (LexerNode){ 1, \"%s\", %d }; \\\n", fromState, c, tokenType, toState);
		}
	}
	// print out the epilogue
	fprintf(out, "\tlexerNodesInitialized = 1; \\\n");
	fprintf(out, "} \n");
	fprintf(out, "#endif\n");
	// finally, return normally
	return 0;
}
