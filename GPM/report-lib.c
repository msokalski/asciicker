/*
 * report-lib.c: the exported version of gpm_report. used in Gpm_Open and co.
 *
 * Copyright (c) 2001        Nico Schottelius <nico@schottelius.org>
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
 */

#include <stdio.h>      /* NULL */
#include <stdarg.h>     /* va_arg/start/... */
#include <stdlib.h>     /* exit() */

#include "message.h"

void gpm_report(int line, const char *file, int stat, const char *text, ... )
{
   const char *string = NULL;
   int log_level;
   va_list ap;

   if (stat == GPM_STAT_DEBUG) return;

   va_start(ap,text);

   switch(stat) {
      case GPM_STAT_INFO : string = GPM_TEXT_INFO ;
                           log_level = LOG_INFO; break;
      case GPM_STAT_WARN : string = GPM_TEXT_WARN ;
                           log_level = LOG_WARNING; break;
      case GPM_STAT_ERR  : string = GPM_TEXT_ERR  ;
                           log_level = LOG_ERR; break;
      case GPM_STAT_DEBUG: string = GPM_TEXT_DEBUG;
                           log_level = LOG_DEBUG; break;
      case GPM_STAT_OOPS : string = GPM_TEXT_OOPS;
                           log_level = LOG_CRIT; break;
   }
#ifdef HAVE_VSYSLOG
   syslog(log_level, "%s", string);
   vsyslog(log_level, text, ap);
#else
   fprintf(stderr,"%s[%s(%d)]:\n",string,file,line);
   vfprintf(stderr,text,ap);
   fprintf(stderr,"\n");
#endif

   if(stat == GPM_STAT_OOPS) exit(1);  /* may a lib function call exit ???? */
}
