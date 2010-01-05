#include <stdlib.h>
#include <stdio.h>

#include "../src/constantDefs.h"

// prints the buildnumber to stdout
int main() {
	unsigned long buildNumber;
	FILE *f;
	if ((f = fopen("./var/version.cfg","r")) == NULL) { // if no version file exists, create one
		f = fopen("./var/version.cfg","w");
		buildNumber = 1;
		fprintf(f,"%s %d\n",VERSION_STRING,buildNumber);
	} else { // a version file exists, so use it
		char *version = MALLOC_STRING;
		fscanf(f, "%s %lu",version,&buildNumber);
		fclose(f);
		f = fopen("./var/version.cfg","w");
		if (strcmp(version,VERSION_STRING) == 0) { // if we're still on the same version
			buildNumber++;
		} else { // else if we've moved on to a new version
			buildNumber = 1;
		}
		fprintf(f,"%s %d\n",VERSION_STRING,buildNumber);
	}
	printf("%lu",buildNumber);
	return 0;
}
