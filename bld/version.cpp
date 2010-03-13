#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../src/constantDefs.h"
#include "../src/mainDefs.h"

// prints the buildnumber to stdout
int main(int argc, char **argv) {
	if (argc != 4) {
		printf("version controller: expects 3 arguments\n");
		return 1;
	}

	unsigned long buildNumber;
	char *dateHash = argv[3];

	char *versionString = argv[1];
	FILE *f;
	if ((f = fopen("var/version.cfg","r")) == NULL) { // if no version file exists, create one
		f = fopen("var/version.cfg","w");
		buildNumber = 1;
		fprintf(f,"%s %lu\n", versionString, buildNumber);
		fclose(f);
	} else { // a version file exists, so use it
		char version[MAX_STRING_LENGTH];
		int retVal = fscanf(f, "%s %lu", version, &buildNumber);
		if (retVal) {
			cerr << "Error reading var/version.cfg" << endl;
			exit(1);
		}
		fclose(f);
		f = fopen("var/version.cfg","w");
		if (strcmp(version, versionString) == 0) { // if we're still on the same version
			buildNumber++;
		} else { // else if we've moved on to a new version
			buildNumber = 1;
		}
		fprintf(f,"%s %lu\n", versionString, buildNumber);
		fclose(f);
	}

	f = fopen("var/versionStamp.txt","w");
	fprintf(f, "%lu.%s", buildNumber, dateHash);
	fclose(f);
	return 0;
}
