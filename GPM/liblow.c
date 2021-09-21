/* 
 *
 * liblow.c - client library - low level (gpm)
 *
 * Copyright 1994,1995   rubini@linux.it (Alessandro Rubini)
 * Copyright (C) 1998    Ian Zimmerman <itz@rahul.net>
 * Copyright 2001-2008   Nico Schottelius (nico-gpm2008 at schottelius.org)
 * 
 * xterm management is mostly by jtklehto@stekt.oulu.fi (Janne Kukonlehto)
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

#ifdef USE_GPM

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>        /* strncmp */
#include <unistd.h>        /* select(); */
#include <errno.h>
#include <fcntl.h>         /* O_RDONLY */

#include <sys/time.h>      /* timeval */
#include <sys/types.h>     /* socket() */
#include <sys/socket.h>    /* socket() */
#include <sys/un.h>        /* struct sockaddr_un */
#include <sys/stat.h>      /* stat() */

#ifdef  SIGTSTP         /* true if BSD system */
#include <sys/file.h>
#include <sys/ioctl.h>
#endif

#include <signal.h> 
#include <linux/vt.h>      /* VT_GETSTATE */
#include <sys/kd.h>        /* KDGETMODE */
#include <termios.h>       /* winsize */

#include "gpmInt.h"
#include "message.h"

#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

/*....................................... Stack struct */
typedef struct Gpm_Stst {
   Gpm_Connect info;
   struct Gpm_Stst *next;
} Gpm_Stst;

/*....................................... Global variables */
int gpm_flag=0; /* almost unuseful now -- where was it used for ? can
                   we remove it now ? FIXME */
int gpm_tried=0;
int gpm_fd=-1;
int gpm_hflag=0;
Gpm_Stst *gpm_stack=NULL;
struct timeval gpm_timeout={10,0};
Gpm_Handler *gpm_handler=NULL;
void *gpm_data=NULL;
int gpm_zerobased=0;
int gpm_visiblepointer=0;
int gpm_mx, gpm_my; /* max x and y (1-based) to fit margins */

unsigned char    _gpm_buf[6*sizeof(short)];
unsigned short * _gpm_arg = (unsigned short *)_gpm_buf +1;

int gpm_consolefd=-1;  /* used to invoke ioctl() */
int gpm_morekeys=0;

int gpm_convert_event(unsigned char *mdata, Gpm_Event *ePtr);

/*----------------------------------------------------------------------------*
 * nice description
 *----------------------------------------------------------------------------*/
static inline int putdata(int where,  Gpm_Connect *what)
{
#ifdef GPM_USE_MAGIC
  static int magic=GPM_MAGIC;

  if (write(where,&magic,sizeof(int))!=sizeof(int)) {
      gpm_report(GPM_PR_ERR,GPM_MESS_WRITE_ERR,strerror(errno));
      return -1;
    }
#endif
  if (write(where,what,sizeof(Gpm_Connect))!=sizeof(Gpm_Connect))
    {
      gpm_report(GPM_PR_ERR,GPM_MESS_WRITE_ERR,strerror(errno));
      return -1;
    }
  return 0;
}

#if (defined(SIGWINCH))
/* itz Wed Mar 18 11:19:52 PST 1998 hook window change as well */

/* Old SIGWINCH handler. */

static struct sigaction gpm_saved_winch_hook;

/*----------------------------------------------------------------------------*
 * nice description
 *----------------------------------------------------------------------------*/
static void gpm_winch_hook (int signum)
{
  struct winsize win;

  if (SIG_IGN != gpm_saved_winch_hook.sa_handler &&
      SIG_DFL != gpm_saved_winch_hook.sa_handler) {
    gpm_saved_winch_hook.sa_handler(signum);
  } /*if*/
  if (ioctl(gpm_consolefd, TIOCGWINSZ, &win) == -1) {
    return;
  } /*if*/
  if (!win.ws_col || !win.ws_row) {
    win.ws_col=80; win.ws_row=25;
  } /*if*/
  gpm_mx = win.ws_col - gpm_zerobased;
  gpm_my = win.ws_row - gpm_zerobased;
}

#endif /* SIGWINCH */

#if (defined(SIGTSTP)) 
/* itz: support for SIGTSTP */

/* Old SIGTSTP handler. */

static struct sigaction gpm_saved_suspend_hook;

/*----------------------------------------------------------------------------*
 * nice description
 *----------------------------------------------------------------------------*/
static void gpm_suspend_hook (int signum)
{
  Gpm_Connect gpm_connect;
  sigset_t old_sigset;
  sigset_t new_sigset;
  struct sigaction sa;
  int success;

  sigemptyset (&new_sigset);
  sigaddset (&new_sigset, SIGTSTP);
  sigprocmask (SIG_BLOCK, &new_sigset, &old_sigset);

  /* Open a completely transparent gpm connection */
  gpm_connect.eventMask = 0;
  gpm_connect.defaultMask = ~0;
  gpm_connect.minMod = ~0;
  gpm_connect.maxMod = 0;
  /* cannot do this under xterm, tough */
  success = (Gpm_Open (&gpm_connect, 0) >= 0);

  /* take the default action, whatever it is (probably a stop :) */
  sigprocmask (SIG_SETMASK, &old_sigset, 0);
  sigaction (SIGTSTP, &gpm_saved_suspend_hook, 0);
  kill (getpid (), SIGTSTP);

  /* in bardo here */

  /* Reincarnation. Prepare for another death early. */
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = gpm_suspend_hook;
  sa.sa_flags = SA_NODEFER;
  sigaction (SIGTSTP, &sa, 0);

  /* Pop the gpm stack by closing the useless connection */
  /* but do it only when we know we opened one.. */
  if (success) {
    Gpm_Close ();
  } /*if*/
}
#endif /* SIGTSTP */

/*----------------------------------------------------------------------------*
 * nice description
 *----------------------------------------------------------------------------*/
int Gpm_Open(Gpm_Connect *conn, int flag)
{
   char *tty = NULL;
   char *term = NULL;
   int i;
   struct sockaddr_un addr;
   struct winsize win;
   Gpm_Stst *new = NULL;
   char* sock_name = 0;
   static char *consolename = NULL;
   int gpm_is_disabled = 0;

   /*....................................... First of all, check xterm */

   if ((term=(char *)getenv("TERM")) && !strncmp(term,"xterm",5)) {
      if(gpm_tried) return gpm_fd; /* no stack */
      gpm_fd=-2;
      GPM_XTERM_ON;
      gpm_flag=1;
      return gpm_fd;
   }
   /*....................................... No xterm, go on */

   /* check whether we know what name the console is: what's with the lib??? */
   if(!consolename) {
      consolename = Gpm_get_console();
	   if(!consolename) {
		   gpm_report(GPM_PR_ERR,"unable to open gpm console, check your /dev filesystem!\n");
		   goto err;
	   }
   }   

   /*
    * So I chose to use the current tty, instead of /dev/console, which
    * has permission problems. (I am fool, and my console is
    * readable/writeable by everybody.
    *
    * However, making this piece of code work has been a real hassle.
    */

   if (!gpm_flag && gpm_tried) return -1;
      gpm_tried=1; /* do or die */

   if ((new = (Gpm_Stst *) malloc(sizeof(Gpm_Stst))) == NULL)
      return -1;

   new->next=gpm_stack;
   gpm_stack=new;

   conn->pid=getpid(); /* fill obvious values */

   if (new->next)
      conn->vc=new->next->info.vc; /* inherit */
   else {
      conn->vc=0;                 /* default handler */
      if (flag > 0) {  /* forced vc number */
         conn->vc=flag;
         if((tty = malloc(strlen(consolename) + Gpm_cnt_digits(flag))) == NULL)
            gpm_report(GPM_PR_OOPS,GPM_MESS_NO_MEM);
         memcpy(tty, consolename, strlen(consolename)-1);
         sprintf(&tty[strlen(consolename) - 1], "%i", flag);
      } else if (flag==0) { /* use your current vc */ 
         if (isatty(0)) tty = ttyname(0);             /* stdin */
         if (!tty && isatty(1)) tty = ttyname(1);     /* stdout */
         if (!tty && isatty(2)) tty = ttyname(2);     /* stderr */
         if (tty == NULL) {
            gpm_report(GPM_PR_ERR,"checking tty name failed");
            goto err;
         }   
          
         conn->vc=atoi(&tty[strlen(consolename)-1]);
      } else /* a default handler -- use console */
        tty=strdup(consolename);

      if (gpm_consolefd == -1)
        gpm_consolefd = 0;
        /*
         if ((gpm_consolefd=open(tty,O_WRONLY)) < 0) {
            gpm_report(GPM_PR_ERR,GPM_MESS_DOUBLE_S,tty,strerror(errno));
            goto err;
         }
        */
   }

   new->info=*conn;

   /*....................................... Get screen dimensions */

   ioctl(gpm_consolefd, TIOCGWINSZ, &win);

   if (!win.ws_col || !win.ws_row) {
      fprintf(stderr, "libgpm: zero screen dimension, assuming 80x25.\n");
      win.ws_col=80; win.ws_row=25;
   }
   gpm_mx = win.ws_col - gpm_zerobased;
   gpm_my = win.ws_row - gpm_zerobased;

   /*....................................... Connect to the control socket */

   if (!(gpm_flag++)) {
      if ( (gpm_fd=socket(AF_UNIX,SOCK_STREAM,0))<0 ) {
         gpm_report(GPM_PR_ERR,GPM_MESS_SOCKET,strerror(errno));
         goto err;
      }

#ifndef SO_PEERCRED
      bzero((char *)&addr,sizeof(addr));
      addr.sun_family=AF_UNIX;
      if (!(sock_name = tempnam (0, "gpm"))) {
         gpm_report(GPM_PR_ERR,GPM_MESS_TEMPNAM,strerror(errno));
         goto err;
      }
      strncpy (addr.sun_path, sock_name, sizeof (addr.sun_path));
      if (bind (gpm_fd, (struct sockaddr*)&addr,
                sizeof (addr.sun_family) + strlen (addr.sun_path))==-1) {
         gpm_report(GPM_PR_ERR,GPM_MESS_DOUBLE_S, sock_name, strerror(errno));
         goto err;
      }
#endif

      bzero((char *)&addr,sizeof(addr));
      addr.sun_family=AF_UNIX;
      strcpy(addr.sun_path, GPM_NODE_CTL);
      i=sizeof(addr.sun_family)+strlen(GPM_NODE_CTL);

      if(connect(gpm_fd,(struct sockaddr *)(&addr),i)<0 ) {
         struct stat stbuf;

         if (errno == ENOENT) {
            gpm_report(GPM_PR_DEBUG,"cannot find %s; ignoring (gpm disabled?)",
                            GPM_NODE_CTL);
            gpm_is_disabled++;
         } else {
            gpm_report(GPM_PR_INFO,GPM_MESS_DOUBLE_S,GPM_NODE_CTL,
                            strerror(errno));
         }
          /*
           * Well, try to open a chr device called /dev/gpmctl. This should
           * be forward-compatible with a kernel server
           */
          close(gpm_fd); /* the socket */
          if ((gpm_fd=open(GPM_NODE_DEV,O_RDWR))==-1) {
            if (errno == ENOENT) {
               gpm_report(GPM_PR_DEBUG,"Cannot find %s; ignoring (gpm disabled?)",
                               GPM_NODE_DEV);
               gpm_is_disabled++;
            } else {
               gpm_report(GPM_PR_ERR,GPM_MESS_DOUBLE_S,GPM_NODE_DEV
                                                   ,strerror(errno));
            }
            goto err;
          }
         if (fstat(gpm_fd,&stbuf)==-1 || (stbuf.st_mode&S_IFMT)!=S_IFCHR) {
            goto err;
         }   
      }
   }
   /*....................................... Put your data */

   if (putdata(gpm_fd,conn)!=-1) {
      /* itz Wed Dec 16 23:22:16 PST 1998 use sigaction, the old
         code caused a signal loop under XEmacs */
      struct sigaction sa;
      sigemptyset(&sa.sa_mask);

#if (defined(SIGWINCH))
      /* And the winch hook .. */
      sa.sa_handler = gpm_winch_hook;
      sa.sa_flags = 0;
      sigaction(SIGWINCH, &sa, &gpm_saved_winch_hook);
#endif

#if (defined(SIGTSTP))
      if (gpm_flag == 1) {
         /* Install suspend hook */
         sa.sa_handler = SIG_IGN;
         sigaction(SIGTSTP, &sa, &gpm_saved_suspend_hook);

         /* if signal was originally ignored, job control is not supported */
         if (gpm_saved_suspend_hook.sa_handler != SIG_IGN) {
            sa.sa_flags = SA_NODEFER;
            sa.sa_handler = gpm_suspend_hook;
            sigaction(SIGTSTP, &sa, 0);
         }
      }
#endif

   }
   return gpm_fd;

  /*....................................... Error: free all memory */
   err:
   if (gpm_is_disabled < 2) /* be quiet if no gpmctl socket found */
      gpm_report(GPM_PR_ERR,"Oh, oh, it's an error! possibly I die! ");
   while(gpm_stack) {
      new=gpm_stack->next;
      free(gpm_stack);
      gpm_stack=new;
   }
   if (gpm_fd>=0) close(gpm_fd);
   if (sock_name) {
      unlink(sock_name);
      free(sock_name);
      sock_name = 0;
   }
   gpm_flag=0;
   return -1;
}

/*-------------------------------------------------------------------*/
int Gpm_Close(void)
{
  Gpm_Stst *next;

  gpm_tried=0; /* reset the error flag for next time */
  if (gpm_fd==-2) /* xterm */
    GPM_XTERM_OFF;
  else            /* linux */
    {
      if (!gpm_flag) return 0;
      next=gpm_stack->next;
      free(gpm_stack);
      gpm_stack=next;
      if (next)
        putdata(gpm_fd,&(next->info));

      if (--gpm_flag) return -1;
    }

  if (gpm_fd>=0) close(gpm_fd);
  gpm_fd=-1;
#ifdef SIGTSTP
  sigaction(SIGTSTP, &gpm_saved_suspend_hook, 0);
#endif
#ifdef SIGWINCH
  sigaction(SIGWINCH, &gpm_saved_winch_hook, 0);
#endif
  //close(gpm_consolefd);
  gpm_consolefd=-1;
  return 0;
}

/*-------------------------------------------------------------------*/
int Gpm_GetEvent(Gpm_Event *event)
{
  int count;
  MAGIC_P((int magic));

  if (!gpm_flag) return 0;

#ifdef GPM_USE_MAGIC
  if ((count=read(gpm_fd,&magic,sizeof(int)))!=sizeof(int))
    {
      if (count==0)
        {
          gpm_report(GPM_PR_INFO,"Warning: closing connection");
          Gpm_Close();
          return 0;
        }
      gpm_report(GPM_PR_INFO,"Read too few bytes (%i) at %s:%d",count,__FILE__,__LINE__);
      return -1;
    }
#endif

  if ((count=read(gpm_fd,event,sizeof(Gpm_Event)))!=sizeof(Gpm_Event))
    {
#ifndef GPM_USE_MAGIC
      if (count==0)
        {
          gpm_report(GPM_PR_INFO,"Warning: closing connection");
          Gpm_Close();
          return 0;
        }
#endif
      /*
       * avoid to send the message if there is no data; sometimes it makes
       * sense to poll the mouse descriptor any now an then using a
       * non-blocking descriptor
       */
      if (count != -1 || errno != EAGAIN)
	  gpm_report(GPM_PR_INFO,"Read too few bytes (%i) at %s:%d",
			count,__FILE__,__LINE__);
      return -1;
    }

  event->x -= gpm_zerobased;
  event->y -= gpm_zerobased;

  return 1;
}


#define MAXNBPREVCHAR 4         /* I don't think more is usefull, JD */
static int nbprevchar=0, prevchar[MAXNBPREVCHAR];

/*-------------------------------------------------------------------*/
int Gpm_CharsQueued ()
{
  return nbprevchar;
}


/*-------------------------------------------------------------------*/
int Gpm_Getc(FILE *f)
{
  fd_set selSet;
  int max, flag, result;
  static Gpm_Event ev;
  int fd=fileno(f);
  static int count;

  /* Hmm... I must be sure it is unbuffered */
  if (!(count++))
    setvbuf(f,NULL,_IONBF,0);

  if (!gpm_flag) return getc(f);

  /* If the handler asked to provide more keys, give them back */
  if (gpm_morekeys) return (*gpm_handler)(&ev,gpm_data);
  gpm_hflag=0;

  max = (gpm_fd>fd) ? gpm_fd : fd;

  /*...................................................................*/
  if (gpm_fd>=0)                                            /* linux */
    while(1)
      {
        if (gpm_visiblepointer) GPM_DRAWPOINTER(&ev);
        do
          {
            FD_ZERO(&selSet);
            FD_SET(fd,&selSet);
            if (gpm_fd>-1)
              FD_SET(gpm_fd,&selSet);
            gpm_timeout.tv_sec=SELECT_TIME;
            flag=select(max+1,&selSet,(fd_set *)NULL,(fd_set *)NULL,&gpm_timeout);
          }
        while (!flag);

        if (flag==-1)
          continue;
        
        if (FD_ISSET(fd,&selSet))
          return fgetc(f);
        
        if (Gpm_GetEvent(&ev) && gpm_handler 
            && (result=(*gpm_handler)(&ev,gpm_data)))
          {
            gpm_hflag=1;
            return result;
          }
      }
  else
    /*...................................................................*/
    if (gpm_fd==-2)                                           /* xterm */
      {
#define DELAY_MS 100
        static struct timeval to={0,DELAY_MS*1000};
        static fd_set selSet;
        extern int gpm_convert_event(unsigned char *data, Gpm_Event *event);
        int c; unsigned char mdata[4];

        if (nbprevchar)  /* if there are some consumed char ... */
          return prevchar[--nbprevchar];

        while(1)
          {
            do
              {
                FD_ZERO(&selSet); FD_SET(fd,&selSet);
                gpm_timeout.tv_sec=SELECT_TIME;
                flag=select(fd+1,&selSet,(fd_set *)NULL,(fd_set *)NULL,&gpm_timeout);
              }
            while (!flag);

            if ((c=fgetc(f))!=0x1b) return c;
            
            /* escape: go on */
            FD_ZERO(&selSet); FD_SET(fd,&selSet); to.tv_usec=DELAY_MS*1000;
            if ((flag=select(fd+1,&selSet,(fd_set *)NULL,(fd_set *)NULL,&to))==0)
              return c;
            if ((c=fgetc(f))!='[')
              {prevchar[nbprevchar++]=c; return 0x1B;}  /* patche par JD 11/08/1998 */

            /* '[': go on */
            FD_ZERO(&selSet); FD_SET(fd,&selSet); to.tv_usec=DELAY_MS*1000;
            if ((flag=select(fd+1,&selSet,(fd_set *)NULL,(fd_set *)NULL,&to))==0)
              {prevchar[nbprevchar++]=c; return 0x1B;}  /* patche par JD 11/08/1998 */
            if ((c=fgetc(f))!='M')
              /* patche par JD 11/08/1998 NOTICE: prevchar is a lifo !*/
              {prevchar[nbprevchar++]=c; prevchar[nbprevchar++]='['; return 0x1B;}
            
            /* now, it surely is a mouse event */

            for (c=0;c<3;c++) mdata[c]=fgetc(f);
            gpm_convert_event(mdata,&ev);

            if (gpm_handler && (result=(*gpm_handler)(&ev,gpm_data)))
              {
                gpm_hflag=1;
                return result;
              }
          } /* while(1) */
      }

  /*...................................................................*/
    else return fgetc(f);                        /* no mouse available */
}


/*-------------------------------------------------------------------*/
int Gpm_Repeat(int msec)
{
  struct timeval to={0,1000*msec};
  int fd;
  fd_set selSet;
  fd=gpm_fd>=0 ? gpm_fd : 0;  /* either the socket or stdin */

  FD_ZERO(&selSet);
  FD_SET(fd,&selSet);
  return (select(fd+1,&selSet,(fd_set *)NULL,(fd_set *)NULL,&to)==0);
}

/*-------------------------------------------------------------------*/
int Gpm_FitValuesM(int *x, int *y, int margin)
{
  if (margin==-1)
    {
    *x = min( max(*x,!gpm_zerobased), gpm_mx);
    *y = min( max(*y,!gpm_zerobased), gpm_my);
    return 0;
    }
  switch(margin)
    {
    case GPM_TOP: (*y)++; break;
    case GPM_BOT: (*y)--; break;
    case GPM_RGT: (*x)--; break;
    case GPM_LFT: (*x)++; break;
    }
  return 0;
}

/*-------------------------------------------------------------------*/
int gpm_convert_event(unsigned char *mdata, Gpm_Event *ePtr)
{
  static struct timeval tv1={0,0}, tv2;
  static int clicks=0;
  int c;

#define GET_TIME(tv) (gettimeofday(&tv, (struct timezone *)NULL))
#define DIF_TIME(t1,t2) ((t2.tv_sec -t1.tv_sec) *1000+ \
                         (t2.tv_usec-t1.tv_usec)/1000)


  /* Variable btn has following meaning: */
  c = mdata[0]-32; /* 0="1-down", 1="2-down", 2="3-down", 3="up" */

  if (c==3)
    {
      ePtr->type = GPM_UP | (GPM_SINGLE<<clicks);
      /* ePtr->buttons = 0; */ /* no, keep info from press event */
      GET_TIME (tv1);
      clicks = 0;
    }
  else
    {
      ePtr->type = GPM_DOWN;
      GET_TIME (tv2);
      if (tv1.tv_sec && (DIF_TIME(tv1,tv2)<250)) /* 250ms for double click */
        {clicks++; clicks%=3;}
      else clicks = 0;
      
      switch (c)
        {
        case 0: ePtr->buttons=GPM_B_LEFT;   break;
        case 1: ePtr->buttons=GPM_B_MIDDLE; break;
        case 2: ePtr->buttons=GPM_B_RIGHT;  break;
        default:    /* Nothing */          break;
        }
    }
  /* Coordinates are 33-based */
  /* Transform them to 1-based */
  ePtr->x = mdata[1]-32-gpm_zerobased;
  ePtr->y = mdata[2]-32-gpm_zerobased;
  return 0;
}

/* Local Variables: */
/* c-indent-level: 3 */
/* End: */

#endif // USE_GPM
