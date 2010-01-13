TARGET = anic
MAN_PAGE = $(TARGET).1
INSTALL_SCRIPT = $(TARGET)-install

INSTALL_PATH = /usr/local/bin

VERSION_STRING = "0.64"
VERSION_YEAR = "2010"

MAKE_PROGRAM = /usr/bin/make

PRINT_VERSION = @echo Version stamp is

CFLAGS = -D VERSION_STRING=$(VERSION_STRING) -D VERSION_YEAR=$(VERSION_YEAR) -O3 -fomit-frame-pointer -ffast-math -pipe -Wall

CORE_DEPENDENCIES = Makefile \
	tmp/version bld/hexTruncate.awk \
	src/mainDefs.h src/constantDefs.h src/system.h src/customOperators.h \
	src/lexer.h src/parser.h src/semmer.h \
	src/core.cpp src/system.cpp src/customOperators.cpp tmp/lexerStruct.o tmp/parserStruct.o src/lexer.cpp src/parser.cpp src/semmer.cpp

TEST_FILES = tst/test.ani



### BUILD TYPES

main: start $(TARGET)

all: start cleanout test install

version: start var/versionStamp.txt
	@$(PRINT_VERSION) $(VERSION_STRING)."`cat var/versionStamp.txt`"

test: start $(TARGET)
	@bld/runTests.sh $(TARGET) -v $(TEST_FILES)

install: start $(TARGET) man bld/authenticatedInstall.sh $(INSTALL_SCRIPT)
	@./bld/authenticatedInstall.sh $(INSTALL_SCRIPT)

uninstall: start
	@echo Uninstalling man page...
	@rm -f /usr/share/man/man1/anic.1.gz
	@echo Uninstalling binary...
	@rm -f $(INSTALL_PATH)/$(TARGET)

man: start $(MAN_PAGE).gz

dist: start $(TARGET) $(MAN_PAGE).gz $(INSTALL_SCRIPT)
	@echo Packing redistributable...
	@tar cf $(TARGET)-$(VERSION_STRING)."`cat var/versionStamp.txt`".tar $(TARGET) $(MAN_PAGE).gz $(INSTALL_SCRIPT)
	@gzip -f $(TARGET)-$(VERSION_STRING)."`cat var/versionStamp.txt`".tar
	@echo Done packing to $(TARGET)-$(VERSION_STRING)."`cat var/versionStamp.txt`".tar.gz

clean: start cleanout
	@echo Cleaning temporary files...
	@rm -R -f var
	@rm -R -f tmp

cleanout: start
	@echo Cleaning output...
	@rm -f $(TARGET)
	@rm -f *.gz
	@rm -f tmp/version
	@rm -f $(MAN_PAGE).gz
	@rm -f tmp/lexerStructGen
	@rm -f tmp/parserStructGen
	@rm -f $(INSTALL_SCRIPT)
	@rm -f var/versionStamp.txt
	@rm -f var/testCertificate.dat
	@$(MAKE_PROGRAM) -C bld/hyacc -f makefile clean -s

purge: start uninstall clean



### SHORTCUT ALIASES

a: all

v: version

t: test

i: install

u: uninstall

m: man

d: dist

c: clean

p: purge



### WRAPPER RULES

start: 
	@echo $(TARGET) ANI Compiler Makefile
	@echo



### BUILD AUXILIARIES

# VERSION CONTROLLER

tmp/version: bld/version.c
	@echo Building version controller...
	@mkdir -p var
	@mkdir -p tmp
	@gcc bld/version.c -o tmp/version

# VERSION STAMP

var/versionStamp.txt: $(CORE_DEPENDENCIES) tmp/version
	@echo Stamping version...
	@mkdir -p var
	@./tmp/version $(VERSION_STRING) var/versionStamp.txt "`date | \` ./bld/getChecksumProgram.sh \` | awk -f bld/hexTruncate.awk`"
	$(PRINT_VERSION) $(VERSION_STRING)."`cat var/versionStamp.txt`"

# INSTALLER

$(INSTALL_SCRIPT): Makefile var/versionStamp.txt
	@echo Generating installer script...
	@echo -e "#!/bin/sh\n\n\
### $(INSTALL_SCRIPT) -- generated script for installing $(TARGET) ANI Compiler v.[$(VERSION_STRING).`cat var/versionStamp.txt`]\n\n\
echo Installing binary...\n\
cp -f $(TARGET) $(INSTALL_PATH)/$(TARGET)\n\
echo Installing manpage...\n\
cp -f $(MAN_PAGE).gz /usr/share/man/man1\n\
echo Installation complete.\n\
exit" > $(INSTALL_SCRIPT)

# MANPAGE

$(MAN_PAGE).gz: man/$(MAN_PAGE)
	@echo Packaging man page...
	@gzip -9 man/$(MAN_PAGE) -c > $(MAN_PAGE).gz



# LEXER

var/lexerStruct.h tmp/lexerStruct.o: tmp/lexerStructGen src/lexerTable.txt src/lexer.h
	@echo Generating lexer structures...
	@mkdir -p var
	@./tmp/lexerStructGen
	@echo Compiling lexer structure object...
	@mkdir -p tmp
	@g++ var/lexerStruct.cpp $(CFLAGS) -c -o tmp/lexerStruct.o

tmp/lexerStructGen: bld/lexerStructGen.cpp
	@echo Building lexer structure generator...
	@mkdir -p tmp
	@g++ bld/lexerStructGen.cpp -o tmp/lexerStructGen

# PARSER

var/parserStruct.h tmp/parserStruct.o: tmp/parserStructGen var/parserTable.txt src/parser.h
	@echo Generating parser structures...
	@mkdir -p var
	@./tmp/parserStructGen
	@echo Compiling parser structure object...
	@mkdir -p tmp
	@g++ var/parserStruct.cpp $(CFLAGS) -O1 -c -o tmp/parserStruct.o

tmp/parserStructGen: bld/parserStructGen.cpp
	@echo Building parser structure generator...
	@mkdir -p tmp
	@g++ bld/parserStructGen.cpp -o tmp/parserStructGen

tmp/hyacc: bld/hyacc/makefile
	@echo Building parser table generator...
	@$(MAKE_PROGRAM) --directory=bld/hyacc --makefile=makefile -s

var/parserTable.txt: tmp/hyacc src/parserGrammar.y
	@echo Constructing parser table...
	@./tmp/hyacc -c -v -D1 -D2 -O1 -Q src/parserGrammar.y
	@mv y.output var/parserTable.txt



### CORE APPLICATION

$(TARGET): var/versionStamp.txt
	@echo Building main executable...
	@rm -f var/testCertificate.dat
	@g++ src/core.cpp src/system.cpp src/customOperators.cpp tmp/lexerStruct.o tmp/parserStruct.o src/lexer.cpp src/parser.cpp src/semmer.cpp \
	-D VERSION_STAMP="\"`cat var/versionStamp.txt`\"" \
	$(CFLAGS) \
	-o $(TARGET)
	@echo Done building main executable.
