TARGET = anic
MAN_PAGE = $(TARGET).1
INSTALL_SCRIPT = $(TARGET)-install
UNINSTALL_SCRIPT = $(TARGET)-uninstall

INSTALL_PATH = /usr/local/bin

VERSION_STRING = "0.65"
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

test: start $(TARGET) bld/runTests.sh
	@chmod +x bld/runTests.sh
	@./bld/runTests.sh $(TARGET) -v $(TEST_FILES)

install: start $(TARGET) man $(INSTALL_SCRIPT) bld/authenticatedInstall.sh
	@chmod +x bld/authenticatedInstall.sh
	@./bld/authenticatedInstall.sh $(INSTALL_SCRIPT)

uninstall: start $(UNINSTALL_SCRIPT)
	@./$(UNINSTALL_SCRIPT)

man: start $(MAN_PAGE).gz

dist: start $(TARGET) $(MAN_PAGE).gz $(INSTALL_SCRIPT) $(UNINSTALL_SCRIPT) bld/genDist.sh
	@chmod +x bld/genDist.sh
	@./bld/genDist.sh $(TARGET) $(VERSION_STRING) $(MAN_PAGE) $(INSTALL_SCRIPT) $(UNINSTALL_SCRIPT)

clean: start
	@echo Cleaning build output...
	@rm -f $(TARGET)
	@rm -f $(MAN_PAGE).gz
	@rm -f $(INSTALL_SCRIPT)
	@rm -f $(UNINSTALL_SCRIPT)
	@rm -f *.gz
	@rm -f tmp/version
	@rm -f tmp/parserTable.txt
	@rm -f tmp/{lexer,parser}StructGen
	@rm -f tmp/{lexer,parser}Struct.{h,cpp,o}
	@rm -f -R tmp
	@$(MAKE_PROGRAM) -C bld/hyacc -f makefile clean -s

reset: start clean
	@echo Resetting build variables...
	@rm -f var/versionStamp.txt
	@rm -f var/testCertificate.dat
	@rm -R -f var

purge: start uninstall reset



### SHORTCUT ALIASES

a: all

v: version

t: test

i: install

u: uninstall

m: man

d: dist

c: clean

r: reset

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

var/versionStamp.txt: ./bld/getChecksumProgram.sh $(CORE_DEPENDENCIES) tmp/version
	@echo Stamping version...
	@mkdir -p var
	@chmod +x bld/getChecksumProgram.sh
	@./tmp/version $(VERSION_STRING) var/versionStamp.txt "`date | \` ./bld/getChecksumProgram.sh \` | awk -f bld/hexTruncate.awk`"
	$(PRINT_VERSION) $(VERSION_STRING)."`cat var/versionStamp.txt`"

# INSTALLER

$(INSTALL_SCRIPT): Makefile var/versionStamp.txt
	@echo Generating installater script...
	@echo -e "#!/bin/sh\n\n\
### $(INSTALL_SCRIPT) -- generated script for installing $(TARGET) ANI Compiler v.[$(VERSION_STRING).`cat var/versionStamp.txt`]\n\n\
echo Installing binary...\n\
cp -f $(TARGET) $(INSTALL_PATH)/$(TARGET)\n\
echo Installing manpage...\n\
cp -f $(MAN_PAGE).gz /usr/share/man/man1\n\
echo Installation complete.\n\
exit" > $(INSTALL_SCRIPT)
	@chmod +x $(INSTALL_SCRIPT)

$(UNINSTALL_SCRIPT): Makefile
	@echo Generating uninstallater script...
	@echo -e "#!/bin/sh\n\n\
### $(UNINSTALL_SCRIPT) -- generated script for uninstalling $(TARGET) ANI Compiler\n\n\
echo Uninstalling man page...\n\
rm -f /usr/share/man/man1/anic.1.gz\n\
echo Uninstalling binary...\n\
rm -f $(INSTALL_PATH)/$(TARGET)\n\
exit" > $(UNINSTALL_SCRIPT)
	@chmod +x $(UNINSTALL_SCRIPT)

# MANPAGE

$(MAN_PAGE).gz: man/$(MAN_PAGE)
	@echo Packaging man page...
	@gzip -9 man/$(MAN_PAGE) -c > $(MAN_PAGE).gz



# LEXER

tmp/lexerStruct.h tmp/lexerStruct.o: tmp/lexerStructGen src/lexerTable.txt src/lexer.h
	@echo Generating lexer structures...
	@mkdir -p var
	@./tmp/lexerStructGen
	@echo Compiling lexer structure object...
	@mkdir -p tmp
	@g++ tmp/lexerStruct.cpp $(CFLAGS) -c -o tmp/lexerStruct.o

tmp/lexerStructGen: bld/lexerStructGen.cpp
	@echo Building lexer structure generator...
	@mkdir -p tmp
	@g++ bld/lexerStructGen.cpp -o tmp/lexerStructGen

# PARSER

tmp/parserStruct.h tmp/parserStruct.o: tmp/parserStructGen tmp/parserTable.txt src/parser.h
	@echo Generating parser structures...
	@mkdir -p var
	@./tmp/parserStructGen
	@echo Compiling parser structure object...
	@mkdir -p tmp
	@g++ tmp/parserStruct.cpp $(CFLAGS) -O1 -c -o tmp/parserStruct.o

tmp/parserStructGen: bld/parserStructGen.cpp
	@echo Building parser structure generator...
	@mkdir -p tmp
	@g++ bld/parserStructGen.cpp -o tmp/parserStructGen

tmp/hyacc: bld/hyacc/makefile
	@echo Building parser table generator...
	@$(MAKE_PROGRAM) --directory=bld/hyacc --makefile=makefile -s

tmp/parserTable.txt: tmp/hyacc src/parserGrammar.y
	@echo Constructing parser table...
	@./tmp/hyacc -c -v -D1 -D2 -O1 -Q src/parserGrammar.y
	@mv y.output tmp/parserTable.txt



### CORE APPLICATION

$(TARGET): var/versionStamp.txt
	@echo Building main executable...
	@rm -f var/testCertificate.dat
	@g++ src/core.cpp src/system.cpp src/customOperators.cpp tmp/lexerStruct.o tmp/parserStruct.o src/lexer.cpp src/parser.cpp src/semmer.cpp \
	-D VERSION_STAMP="\"`cat var/versionStamp.txt`\"" \
	$(CFLAGS) \
	-o $(TARGET)
	@echo Done building main executable.
