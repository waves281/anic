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
 * hyacc_path_dos.c
 *
 * Retrieve files hyaccpar and hyaccmanpage.
 * This is used by DOS/Windows version only.
 *
 * 1) If set USE_CUR_DIR to 0:
 *
 * After compilation, put hyacc.exe, hyaccmanpage and hyaccpar
 * to c:\windows\. Since this folder by default is in the system
 * path variable, hyacc becomes a command that you can use 
 * wherever you are in windows.
 *
 * You can also change the path here, and put hyacc and hyaccmanpage
 * and hyaccpar elsewhere, but then you may need to add the path
 * to system searchable path, so the system can find it.
 *
 * You also can remove the c:\\windows from the path here, then
 * hyacc will search in the current directory for them.
 *
 * 2) Otherwise, if set USE_CUR_DIR to 1:
 *
 * Then hyacc searches in the current directory.
 *
 * @Author: Xin Chen
 * @Date started: February 8, 2007
 * @Last modified: March 21, 2007
 * @Copyright (C) 2007.
 */

#include "y.h"

#define USE_CUR_DIR 1

#if USE_CUR_DIR

void show_manpage() {
   system("more hyaccmanpage");
}

char * HYACC_PATH = "hyaccpar";

#else

void show_manpage() {
   system("more c:\\windows\\hyaccmanpage");
}

char * HYACC_PATH = "c:\\windows\\hyaccpar";

#endif


