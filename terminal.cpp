#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <pthread.h>  // compile with -pthread
#include <pty.h> // link  with -lutil

volatile int stop = 0;
int pty_fd = -1; // PTY

void* Read(void* arg)
{
    // read chars from child
    // display them
    char buf[80];
    while (1)
    {
        int len = read(pty_fd, buf, 80);
        if (len<=0 || stop)
            return 0;
        write(STDOUT_FILENO, buf, len);
    }
    return 0;
}

void* Write(void* arg)
{
    // read KBD keys
    // write them to child
    char buf[80];
    while (1)
    {
        int len = read(STDIN_FILENO, buf, 80);
        if (len<=0 || stop)
            return 0;
        write(pty_fd, buf, len);
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
        ioctl(pty_fd, TIOCSWINSZ, &ws);
    }
}

int main(int argc, char** argv)
{
    struct winsize ws;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);

    char name[64]="";
    pid_t pid = forkpty(&pty_fd, name, 0, &ws);
    if (pid == 0)
    {
        // child
        execl("/bin/bash", "bash", (char*) NULL);
        exit(1);
    }

    if (pid < 0 || pty_fd < 0)
    {
        if (pty_fd>=0)
            close(pty_fd);
        return -1;
    }

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

    // both threads are done
    // it is safe to clode pty 
    close(pty_fd);
    pty_fd = -1;

    // wait for child to go die
    int status;
    waitpid(pid, &status, 0);

    // revert from dumb pipe to original termios
    tcsetattr(STDIN_FILENO, 0, &org_ts);

    return 0;
}
