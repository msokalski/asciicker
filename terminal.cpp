#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <stdio.h>

#include <pthread.h>  // compile with -pthread
#include <pty.h> // link  with -lutil

#include <fcntl.h>

// BUILD ME:
// g++ -pthread -o .run/terminal terminal.cpp -lutil

// RUN ME:
// .run/terminal
// .run/terminal <logfile>

volatile int stop = 0;
int pty_fd = -1; // PTY
int log_fd = -1;

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int Escape(int fd, const char* hdr, int hdrlen, const char* buf, int buflen)
{
    pthread_mutex_lock( &log_mutex );

    static const char* last_hdr = 0;
    const int siz = 4096;
    char esc[siz];
    int ret = 0;

    int l = 0;
    if (last_hdr != hdr)
    {
        if (last_hdr)
        {
            esc[0] = '\n';
            memcpy(esc+1,hdr,hdrlen);
            l = hdrlen+1;
        }
        else
        {
            memcpy(esc,hdr,hdrlen);
            l = hdrlen;
        }
        
        last_hdr = hdr;
    }
    
    for (int i=0; i<buflen; i++)
    {
        if (l>siz-4)
        {
            ret+=l;
            write(fd,esc,l);
            // l=hdrlen;
            l=0;
        }

        if (buf[i]>0x20 || buf[i]<0)
            esc[l++] = buf[i];
        else
        switch (buf[i])
        {
            case '\\' :
                esc[l++] = '\\';
                esc[l++] = '\\';
                break;
            case '\r' :
                esc[l++] = '\\';
                esc[l++] = 'r';
                break;
            case '\n' :
                esc[l++] = '\\';
                esc[l++] = 'n';
                esc[l++] = '\n';
                ret+=l;
                write(fd,esc,l);
                memcpy(esc,hdr,hdrlen);
                l=hdrlen;
                break;
            case '\t' :
                esc[l++] = '\\';
                esc[l++] = 't';
                break;
            case '\v' :
                esc[l++] = '\\';
                esc[l++] = 'v';
                break;
            default:
            {
                int hi = (buf[i]>>4)&0xF;
                int lo = buf[i]&0xF;

                if (hi<10)
                    hi+='0';
                else
                    hi+='A'-10;

                if (lo<10)
                    lo+='0';
                else
                    lo+='A'-10;

                esc[l++] = '\\';
                esc[l++] = 'x';
                esc[l++] = hi;
                esc[l++] = lo;
            }
        }
    }

    if (l)
    {
        ret+=l;
        write(fd,esc,l);
    }

    pthread_mutex_unlock( &log_mutex );

    return ret;
}

void* Read(void* arg)
{
    // read chars from child
    // display them
    const int siz = 1024;
    char buf[siz];
    while (1)
    {
        int len = read(pty_fd, buf, siz);
        if (len<=0 || stop)
            return 0;
        write(STDOUT_FILENO, buf, len);
        if (log_fd >= 0)
            Escape(log_fd, "O: ", 3, buf, len);
    }
    return 0;
}

void* Write(void* arg)
{
    // read KBD keys
    // write them to child
    const int siz = 1024;
    char buf[siz];
    while (1)
    {
        int len = read(STDIN_FILENO, buf, siz);
        if (len<=0 || stop)
            return 0;
        write(pty_fd, buf, len);
        if (log_fd >= 0)
            Escape(log_fd, "I: ", 3, buf, len);
    }

    return 0;
}

void SignalHandler(int s)
{
    if (s==SIGWINCH)
    {
        // just pipe SIGWINCH
        struct winsize ws;
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);

        if (log_fd)
        {
            char buf[64];
            int len = sprintf(buf,"{%d,%d} ", ws.ws_col, ws.ws_row);
            Escape(log_fd, "S: ", 3, buf, len);
        }

        //ioctl(pty_fd, TIOCSWINSZ, &ws);

    }

    /* 
    - it turns out that we get this signal only if parent terminal is changin window size.
    - if child process like stty makes ioctl(TIOCSWINSZ) it modifies PTY window size silently!
      so neither we nor our parent terminal can respond to it in any way. 
    */
}

int main(int argc, char** argv)
{
    struct winsize ws;
    if ( ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0 )
    {
        // we are not attached to parent terminal!
        return -2;
    }

    char name[64]="";
    pid_t pid = forkpty(&pty_fd, name, 0, &ws);
    if (pid == 0)
    {
        // child
        execl("/bin/bash", "bash", (char*) NULL);
        exit(1);
    }

    // something went wrong ?
    if (pid < 0 || pty_fd < 0)
    {
        if (pty_fd>=0)
            close(pty_fd);
        return -1;
    }

    if (argc>1)
        log_fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    struct termios org_ts;
    tcgetattr(STDIN_FILENO, &org_ts);

    // let's become a dumb pipe
    struct termios ts;
    cfmakeraw(&ts);
    tcsetattr(STDIN_FILENO, 0, &ts);

    // setup signal handler too
    // for piping SIGWINCH from master to slave
    signal(SIGWINCH, SignalHandler);

    // start pumping from(r) and to(w) PTY
    pthread_t r,w;
    pthread_create(&r, 0, Read, 0);
    pthread_create(&w, 0, Write, 0);

    // wait for read thread to finish 
    // (most likely because child process 
    // terminated or it closed its 'stdout')
    pthread_join(r,0);

    // writer is most likely locked in read(STDIN_FILENO)
    // wake it up with fake input using ioctl 
    // then it will check for stop flag and exit
    stop = true;
    ioctl(STDIN_FILENO,TIOCSTI,"!");
    pthread_join(w,0);

    // no more signal pumping
    signal(SIGWINCH, SIG_DFL);

    // both threads are done
    // it is safe to clode pty 
    close(pty_fd);
    pty_fd = -1;

    if (log_fd>=0)
    {
        close(log_fd);
        log_fd = -1;
    }

    // wait for child to die
    int status;
    waitpid(pid, &status, 0);

    // revert from dumb pipe to original termios
    tcsetattr(STDIN_FILENO, 0, &org_ts);

    return 0;
}
