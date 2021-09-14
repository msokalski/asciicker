/*
 * tools.c - tools which are needed by client and server
 *
 * Copyright (c) 2001 	     Nico Schottelius <nico@schottelius.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 ********/

#include <stdio.h> /* NULL */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>  /* these three are */
#include <sys/stat.h>   /* needed for      */
#include <unistd.h>     /* stat() */

#include "gpmInt.h"   /* only used for some defines */
#include "message.h"

/*****************************************************************************
 * check, whether devfs is used or not.
 * See /usr/src/linux/Documentation/filesystems/devfs/ for details.
 * Returns: the name of the console (/dev/tty0 or /dev/vc/0)
 *****************************************************************************/
char *Gpm_get_console( void )
{

   char *back = NULL, *tmp = NULL;
   struct stat buf;

   /* first try the devfs device, because in the next time this will be
    * the preferred one. If that fails, take the old console */
   
   /* Check for open new console */
   if (stat(GPM_DEVFS_CONSOLE,&buf) == 0)
      tmp = GPM_DEVFS_CONSOLE;
  
   /* Failed, try OLD console */
   else if(stat(GPM_OLD_CONSOLE,&buf) == 0)
      tmp = GPM_OLD_CONSOLE;
  
   if(tmp != NULL)
      if((back = malloc(strlen(tmp) + sizeof(char)) ) != NULL)
         strcpy(back,tmp);

   return(back);
}

/* what's the english name for potenz ? */
int Gpm_x_high_y(int base, int pot_y)
{
   int val = 1;
   
   if(pot_y == 0) val = 1;
   else if(pot_y  < 0) val = 0;     /* ugly hack ;) */
   else while(pot_y > 0) {
      val = val * base;
      pot_y--;
   }   
   return val;
}   
      
/* return characters needed to display int */
int Gpm_cnt_digits(int number)
{
   /* 0-9 = 1        10^0 <-> (10^1)-1
    * 10 - 99 = 2    10^1 <-> (10^2)-1
    * 100 - 999 = 3  10^2 <-> (10^3)-1
    * 1000 - 9999 = 4 ...  */
   
   int ret = 0, num = 0;

   /* non negative, please */
   if(number < 0) number *= -1;
   else if(number == 0) ret = 1;
   else while(number > num) {
      ret++;
      num = (Gpm_x_high_y(10,ret) - 1);
   }   

   return(ret);
}      
