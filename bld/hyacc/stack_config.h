/* 
   This file is part of Hyacc, a LR(0)/LALR(1)/LR(1) parser generator.
   Copyright (C) 2007, 2008, 2009 Xin Chen. chenx@hawaii.edu

   Hyacc is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   Hyacc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Hyacc; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _STACK_CONFIG_H_
#define _STACK_CONFIG_H_

/* 
 * stack_config.h
 *
 * An expandable integer stack, implemented as an array.
 *
 * @Author: Xin Chen
 * @Created on: 2/27/2006
 * @Last modified: 6/19/2007
 * @Copyright (C) 2007, 2008, 2009
 */

#include "y.h"

#define DEBUG_STACK 0
#define STACK_INIT_SIZE 256
#define USE_WARNING 0

typedef struct {
  Configuration ** array;
  int pt;
  int capacity;
} stack;

stack * stack_create();
stack * stack_create2(int init_capacity); 
void stack_push(stack * s, Configuration * n); 
Configuration * stack_pop(stack * s);
Configuration * stack_top(stack * s); 
int stack_count(stack * s); /* number of elements in stack */
void stack_destroy(stack * s); 
void stack_dump(stack * s); 

#endif

