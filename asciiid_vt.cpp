#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <stdio.h>
#include <fcntl.h>

#include <pthread.h>  // compile with -pthread
#include <pty.h> // link  with -lutil

#include "asciiid_platform.h"

#include "gl.h"
#include "rgba8.h"

pid_t pid = -1;
int pty_fd = -1; // PTY

void vt_render()
{
    glClearColor(.1,.1,.1,0);
    glClear(GL_COLOR_BUFFER_BIT);
	a3dSwapBuffers();
}

void vt_mouse(int x, int y, MouseInfo mi)
{
}

void vt_resize(int w, int h)
{
    // recalc new vt w,h
    struct winsize ws;
    ws.ws_col = w/8;
    ws.ws_row = h/8;
    ws.ws_xpixel=0;
    ws.ws_ypixel=0;

    ioctl(pty_fd, TIOCSWINSZ, &ws);
}

void vt_init()
{
    int wh[2];
    a3dGetRect(0,wh);
    // setup default font & colors ... state!
    // calc terminal w,h

    struct winsize ws;
    ws.ws_col = wh[0]/8;
    ws.ws_row = wh[1]/8;
    ws.ws_xpixel=0;
    ws.ws_ypixel=0;

    char name[64]="";
    pid = forkpty(&pty_fd, name, 0, &ws);
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

        a3dClose();
    }
    else
    {
        a3dSetTitle("TERMINALICO");
        a3dSetIcon("./icons/app.png");
        a3dSetVisible(true);
    }
    
}

void vt_keyb_char(wchar_t chr)
{
    // send regular keys in oridinary way (utf8)
    // ...
}

void vt_keyb_key(KeyInfo ki, bool down)
{
    // process special keys into sequences before sending to child
    // ...
}

void vt_keyb_focus(bool set)
{
}

void vt_close()
{
    // kill child etc...
    close(pty_fd);
    pty_fd=-1;
    kill(pid, SIGQUIT);

    int status;
    waitpid(pid, &status, 0);

	a3dClose();
}

int Terminal(int argc, char *argv[])
{
	PlatformInterface pi;
	pi.close = vt_close;
	pi.render = vt_render;
	pi.resize = vt_resize;
	pi.init = vt_init;
	pi.keyb_char = vt_keyb_char;
	pi.keyb_key = vt_keyb_key;
	pi.keyb_focus = vt_keyb_focus;
	pi.mouse = vt_mouse;

	GraphicsDesc gd;
	gd.color_bits = 32;
	gd.alpha_bits = 8;
	gd.depth_bits = 24;
	gd.stencil_bits = 8;
	gd.flags = (GraphicsDesc::FLAGS) (GraphicsDesc::DEBUG_CONTEXT | GraphicsDesc::DOUBLE_BUFFER);

	int rc[] = {0,0,1920*2,1080+2*1080};
	gd.wnd_mode = A3D_WND_NORMAL;
	gd.wnd_xywh = 0;

	a3dOpen(&pi, &gd);

	return 0;
}