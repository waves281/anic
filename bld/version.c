#include <stdlib.h>
#include <stdio.h>

#include "../src/constantDefs.h"

// prints the buildnumber to stdout
int main(int argc, char **argv) {
	if (argc != 2) {
		printf("version controller: expects 2 arguments\n");
		return 1;
	}
	unsigned long buildNumber;
	char *versionString = argv[1];
	FILE *f;
	if ((f = fopen("./var/version.cfg","r")) == NULL) { // if no version file exists, create one
		f = fopen("./var/version.cfg","w");
		buildNumber = 1;
		fprintf(f,"%s %d\n", versionString, buildNumber);
	} else { // a version file exists, so use it
		char *version = MALLOC_STRING;
		fscanf(f, "%s %lu", version, &buildNumber);
		fclose(f);
		f = fopen("./var/version.cfg","w");
		if (strcmp(version, versionString) == 0) { // if we're still on the same version
			buildNumber++;
		} else { // else if we've moved on to a new version
			buildNumber = 1;
		}
		fprintf(f,"%s %d\n", versionString, buildNumber);
	}
	printf("%lu", buildNumber);
	return 0;
}
