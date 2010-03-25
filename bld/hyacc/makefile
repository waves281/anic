############################################################
# This file is part of Hyacc, a LR(1) parser generator.
# Copyright (C) 2007 Xin Chen. chenx@hawaii.edu
#
# Hyacc is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Hyacc is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Hyacc; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
############################################################
#
# makefile for hyacc
#
# These targets are used:
#   y, release, debug, clean install, uninstall, pack
# Here target y is the same as release.
#
# To use this makefile, the only configuration needed is
# to change the value of INSTALL_PATH below to the
# destination path you want to install hyacc.
#
# @Author: Xin Chen
# @Created on: Feb 10, 2007
# @Last modified: Oct 25, 2007
#############################################################

# NOTE: choose your installation directory here.
#INSTALL_PATH = /usr/bin
INSTALL_PATH = `pwd` # current directory

SRC_HEADER = y.h stack_config.h mrt.h lane_tracing.h
SRC = y.c get_yacc_grammar.c gen_compiler.c get_options.c \
      version.c hyacc_path.c symbol_table.c state_hash_table.c \
      queue.c gen_graphviz.c lr0.c lane_tracing.c stack_config.c \
      mrt.c upe.c
OBJS = y.o get_yacc_grammar.o gen_compiler.o get_options.o \
       version.o hyacc_path.o symbol_table.o state_hash_table.o \
       queue.o gen_graphviz.o lr0.o lane_tracing.o \
       stack_config.o mrt.o upe.o
PACK_SRC = $(SRC) $(SRC_HEADER) inst.c hyacc_path_dos.c makefile \
           hyaccpar hyaccmanpage hyaccmanpage.html \
           hyacc.1 GPL_license readme.pdf readme.txt calc.y
TARGET = ../../bin/hyacc
CC = gcc
DATE = `date '+%m-%d-%y'`
PACK_NAME = hyacc_$(DATE).tar
FFLAG = -c -g


$(TARGET) : $(OBJS) $(SRC_HEADER)
	@$(CC) -o $(TARGET) $(OBJS)

#
# all - the old default.
#
all : $(SRC) $(SRC_HEADER)
	@$(CC) -o $(TARGET) $(SRC)

release : $(SRC) $(SRC_HEADER)
	@make create_path_file
	@$(CC) -o $(TARGET) $(SRC)

debug : $(SRC) $(SRC_HEADER)
	@make create_path_file
	@$(CC) -g -o $(TARGET) $(SRC)

clean :
	@rm -f ./$(TARGET) ./$(OBJS)

install : create_path_file
	@make release
	@-cp -f ./$(TARGET) $(INSTALL_PATH)/.
	@-cp -f ./hyaccpar $(INSTALL_PATH)/.
	@-cp -f ./hyaccmanpage $(INSTALL_PATH)/.

create_path_file: inst.c
	@$(CC) inst.c -o inst
	@./inst $(INSTALL_PATH)
	@rm -f inst

uninstall:
	@echo to uninstall, manually remove the following files:
	@echo $(INSTALL_PATH)/$(TARGET)
	@echo $(INSTALL_PATH)/hyaccpar
	@echo $(INSTALL_PATH)/hyaccmanpage

dist:
	@-rm -f $(PACK_NAME).gz
	@tar cvf $(PACK_NAME) $(PACK_SRC)
	@gzip $(PACK_NAME)
	@ls $(PACK_NAME).gz

#
# for object files.
#

gen_compiler.o : gen_compiler.c y.h
	@$(CC) $(FFLAG) gen_compiler.c

gen_graphviz.o : gen_graphviz.c y.h
	@$(CC) $(FFLAG) gen_graphviz.c

get_options.o : get_options.c y.h
	@$(CC) $(FFLAG) get_options.c

get_yacc_grammar.o : get_yacc_grammar.c y.h
	@$(CC) $(FFLAG) get_yacc_grammar.c

hyacc_path.o : hyacc_path.c y.h
	@$(CC) $(FFLAG) hyacc_path.c

lane_tracing.o : lane_tracing.c lane_tracing.h y.h stack_config.h
	@$(CC) $(FFLAG) lane_tracing.c

lr0.o : lr0.c y.h
	@$(CC) $(FFLAG) lr0.c

mrt.o : mrt.c mrt.h y.h
	@$(CC) $(FFLAG) mrt.c

queue.o : queue.c y.h
	@$(CC) $(FFLAG) queue.c

stack_config.o : stack_config.c y.h
	@$(CC) $(FFLAG) stack_config.c

state_hash_table.o : state_hash_table.c y.h
	@$(CC) $(FFLAG) state_hash_table.c

symbol_table.o : symbol_table.c y.h
	@$(CC) $(FFLAG) symbol_table.c

upe.o : upe.c y.h mrt.h
	@$(CC) $(FFLAG) upe.c

version.o : version.c y.h
	@$(CC) $(FFLAG) version.c

y.o : y.c y.h lane_tracing.h
	@$(CC) $(FFLAG) y.c


