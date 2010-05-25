/* 
   This file is part of Hyacc, a LR(0)/LALR(1)/LR(1) parser generator.
   Copyright (C) 2007 Xin Chen. chenx@hawaii.edu

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

/*
 * stack_config.c
 *
 * An expandable stack for Configuration pointer.
 *
 * @Author: Xin Chen 
 * @Created on: 2/27/2006 
 * @Last modified: 6/19/2007
 */

#include "stack_config.h"


stack * stack_create() {
  return stack_create2(STACK_INIT_SIZE);
}


stack * stack_create2(int size) {
  stack * s = (stack *) malloc(sizeof(stack));
  if (s == NULL) {
    puts("stack_create: out of memory");
    return NULL;
  }
  s->pt = 0;
  s->capacity = size;
  s->array = (Configuration **) 
             malloc(s->capacity * sizeof(Configuration *));
  if (s->array == NULL) {
    puts("stack_create: out of memory");
    return NULL;
  }
  return s;
}


void stack_push(stack * s, Configuration * n) {
  s->array[s->pt] = n;
  s->pt ++;
#if DEBUG_STACK
    printf("stack_push: %d\n", s->array[s->pt - 1]);
    stack_dump(s);
#endif

  if (s->pt == s->capacity) {
    s->array = (Configuration **) realloc(
      (void *) s->array, 2 * s->capacity * sizeof(Configuration *) );
    if (s->array == NULL) {
      puts("stack_push: out of memory");
      return; 
    }
    s->capacity *= 2;
#if DEBUG_STACK
    printf("stack_push: expand stack capacity to: %d\n", 
           s->capacity);
#endif
  }
}


Configuration * stack_pop(stack * s) {
  if (s->pt == 0) { 
    if (USE_WARNING)
      puts("stack_pop warning: underflow, return NULL"); 
    return NULL; 
  }
  -- s->pt;
#if DEBUG_STACK
  printf("stack_pop: %d\n", s->array[s->pt]);
  stack_dump(s);
#endif
  return s->array[s->pt];
}


Configuration * stack_top(stack * s) {
  if (s->pt == 0) { 
    if (USE_WARNING)
      puts("stack_top warning: underflow, return NULL");
    return NULL;
  }
#if DEBUG_STACK
  printf("stack_top: %d. ct: %d\n", s->array[s->pt - 1], 
         stack_count(s));
#endif
  return s->array[s->pt - 1];
}


/* 
 * @Return: number of elements in the stack.
 */
int stack_count(stack * s) {
  return s->pt;
}


void stack_destroy(stack * s) { 
  free(s->array);
  free(s); 
}


void stack_dump(stack * s) {
  int i;
  Configuration * c;
  printf("stack capacity: %d, count: %d\n", 
         s->capacity, stack_count(s));
  if (stack_count(s) == 0) return;

  for (i = 0; i < s->pt; i ++) {
    if ((i > 0) && (i % 10 == 0)) putchar('\n');
    c = s->array[i];
    printf("[%d] ", i);
    if (c == 0) {
      puts("0");
    } else if (c == (Configuration *) -1) {
      puts("*");
    } else {
      stdout_writeConfig(c);
    } 
  }
  putchar('\n');
}

