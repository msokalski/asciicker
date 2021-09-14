/*
 * defines used for messaging
 *
 * Copyright (c) 2001,2002    Nico Schottelius <nico@schottelius.org>
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

#ifndef _MESSAGE_H
#define _MESSAGE_H

/* printing / reporting */
#define FL  __LINE__,__FILE__
#define FLS "[%s(%d)]: "
#define FLP FLS,file,line

/* for switch (intern) */
#define GPM_STAT_DEBUG  2
#define GPM_STAT_INFO   3
#define GPM_STAT_ERR    4
#define GPM_STAT_WARN   5
#define GPM_STAT_OOPS   6

/* for print (extern) */
#define GPM_PR_DEBUG  FL,GPM_STAT_DEBUG
#define GPM_PR_INFO   FL,GPM_STAT_INFO
#define GPM_PR_ERR    FL,GPM_STAT_ERR
#define GPM_PR_WARN   FL,GPM_STAT_WARN
#define GPM_PR_OOPS   FL,GPM_STAT_OOPS

/* messages */
#define GPM_TEXT_INFO   "*** info "
#define GPM_TEXT_WARN   "*** warning "
#define GPM_TEXT_DEBUG  "*** debug "
#define GPM_TEXT_ERR    "*** err "
#define GPM_TEXT_OOPS   "O0o.oops(): "

/* strings */
#define GPM_STRING_INFO   GPM_TEXT_INFO FLP
#define GPM_STRING_WARN   GPM_TEXT_WARN FLP
#define GPM_STRING_DEBUG  GPM_TEXT_DEBUG FLP
#define GPM_STRING_ERR    GPM_TEXT_ERR FLP
#define GPM_STRING_OOPS   GPM_TEXT_OOPS FLP

/* running situations */
#define GPM_RUN_STARTUP 0
#define GPM_RUN_DAEMON  1
#define GPM_RUN_DEBUG   2

/* messages */

/* info */
#define GPM_MESS_VERSION            "gpm " PACKAGE_VERSION
#define GPM_MESS_STARTED            "Started gpm successfully. Entered daemon mode."
#define GPM_MESS_KILLED             "Killed gpm(%d)."
#define GPM_MESS_SKIP_DATA          "Skipping a data packet (?)"
#define GPM_MESS_DATA_4             "Data %02x %02x %02x (%02x)"
#define GPM_MESS_NO_MAGIC           "No magic"
#define GPM_MESS_CONECT_AT          "Connecting at fd %i"
#define GPM_MESS_USAGE              "Usage: %s [options]\n" \
         "  Valid options are (not all of them are implemented)\n" \
         "    -a accel         sets the acceleration (default %d)\n" \
         "    -A [limit]       start with selection disabled (`aged')\n" \
         "    -b baud-rate     sets the baud rate (default %d)\n" \
         "    -B sequence      allows changing the buttons (default '%s')\n" \
         "    -d delta         sets the delta value (default %d) (must be 2 or more)\n" \
         "    -D	             debug mode: don't auto-background\n" \
         "    -g tap-button    sets the button (1-3) that is emulated by tapping on\n" \
         "                     a glidepoint mouse, none by default. (mman/ps2 only)\n" \
         "    -i interval      max time interval for multiple clicks (default %i)\n" \
         "    -k               kill a running gpm, to start X with a busmouse\n" \
         "    -l charset       loads the inword() LUT (default '%s')\n" \
         "    -m mouse-device  sets mouse device\n" \
         "    -M               enable multiple mice. Following options refer to\n" \
         "                     the second device. Forces \"-R\"\n" \
         "    -o options       decoder-specific options (e.g.: \"dtr\", \"rts\")\n" \
         "    -p               draw the pointer while striking a selection\n" \
         "    -q               quit after changing mouse behaviour\n" \
         "    -r number        sets the responsiveness (default %i)\n" \
         "    -R mouse-type    enter repeater mode. X should read /dev/gpmdata\n" \
         "                     like it was a mouse-type device.  Default is MouseSystems.\n" \
         "                     You can also specify \"raw\" to relay the raw device data.\n" \
         "    -s sample-rate   sets the sample rate (default %d)\n" \
         "    -S [commands]    enable special commands (see man page)\n" \
         "    -t mouse-type    sets mouse type (default '%s')\n" \
         "                     Use a non-existent type (e.g. \"help\") to get a list\n" \
         "    -T               test: read mouse, no clients\n" \
         "    -v               print version and exit\n" \
         "    -V verbosity     increase number of logged messages\n\n\n" \
         "    Examples:\n\n" \
         "    gpm -m /dev/misc/psaux -t ps2    to start with a ps2 mouse\n" \
         "    gpm -m /dev/tts/0 -t mman        to use mouse man on COM1\n\n"

/* Later ... but first when it's running */
/*         "    -u               autodetect mice [not implemented yet]\n" \ */

#define GPM_MESS_IMPS2_AUTO         "imps2: Auto-detected intellimouse PS/2"
#define GPM_MESS_IMPS2_PS2          "imps2: Auto-detected standard PS/2"
#define GPM_MESS_AVAIL_MYT          "Available mouse types are:\n\n" \
                                    "r name   synonym         description\n\n"
#define GPM_MESS_SYNONYM            "%c %-8s %s\n            Synonyms: %s\n"

/* generic messages */
#define GPM_MESS_DOUBLE_S           "%s: %s"
#define GPM_MESS_DOUBLE_I           "%i - %i:"
#define GPM_MESS_X_Y_VAL            "x %i, y %i"


/* mem */
#define GPM_MESS_NO_MEM             "I couln't get any memory! I die! :("
#define GPM_MESS_ALLOC_FAILED       "allocation failed!"

/* files */
#define GPM_MESS_SOCKET             "socket(): %s"
#define GPM_MESS_TEMPNAM            "tempnam(): %s"
#define GPM_MESS_OPEN               "Could not open %s."
#define GPM_MESS_CREATE_FIFO        "Creating FIFO %s failed."
#define GPM_MESS_STALE_PID          "Removing stale pid file %s"
#define GPM_MESS_MKSTEMP_FAILED     "mkstemp failed with file %s!"
#define GPM_MESS_NOTWRITE           "Can you write to %s?"
#define GPM_MESS_WRITE_ERR          "write(): %s"
#define GPM_MESS_OPEN_CON           "Opening console failed."
#define GPM_MESS_OPEN_SERIALCON     "We seem to be on a serial console."
#define GPM_MESS_READ_FIRST         "Error in read()ing first: %s"
#define GPM_MESS_READ_REST          "Error in read()ing rest: %s"
#define GPM_MESS_REMOVE_FILES       "Removing files %s and %s"
#define GPM_MESS_READ_PROB          "Problem reading from %s"
#define GPM_MESS_CLOSE              "Closing"

/* error */
#define GPM_MESS_CANT_KILL          "Couldn't kill gpm(%d)!"
#define GPM_MESS_FSTAT              "fstat()"
#define GPM_MESS_SETSID_FAILED      "Setsid failed"
#define GPM_MESS_CHDIR_FAILED       "change directory failed"
#define GPM_MESS_FORK_FAILED        "Fork failed."
#define GPM_MESS_VCCHECK            "Failed on virtual console check."
#define GPM_MESS_PROT_ERR           "Error in protocol"
#define GPM_MESS_ROOT               "You should be root to run gpm!"
#define GPM_MESS_SPEC_ERR           "Error in the %s specification. Try \"%s -h\".\n"
#define GPM_MESS_ALREADY_RUN        "gpm is already running as pid %d"
#define GPM_MESS_NO_REPEAT          "Repeating into %s protocol not yet implemented :-("
#define GPM_MESS_GET_SHIFT_STATE    "get_shift_state"
#define GPM_MESS_GET_CONSOLE_STAT   "get_console_status"
#define GPM_MESS_DISABLE_PASTE      "disabling pasting per request from virtual console (%d)"
#define GPM_MESS_UNKNOWN_FD         "Unknown fd selected"
#define GPM_MESS_ACCEPT_FAILED      "accept() failed: %s"
#define GPM_MESS_ADDRES_NSOCKET     "Address %s not a socket in processConn"
#define GPM_MESS_SOCKET_OLD         "Socket too old in processConn"
#define GPM_MESS_GETSOCKOPT         "getsockopt(SO_PEERCRED): %s"
#define GPM_MESS_STAT_FAILS         "stat on %s fails in processConn"
#define GPM_MESS_NEED_MDEV          "Please use -m /dev/mouse -t protocol"
#define GPM_MESS_MOUSE_INIT         "mouse initialization failed"
#define GPM_MESS_SOCKET_PROB        "socket()"
#define GPM_MESS_BIND_PROB          "Problem binding %s"
#define GPM_MESS_SELECT_PROB        "Problem with select"
#define GPM_MESS_SELECT_STRING      "select(): %s"
#define GPM_MESS_SELECT_TIMES       "selected %i times"

#define GPM_MESS_OPTION_NO_ARG      "%s: Option \"%s\" takes no argument: ignoring \"%s\""
#define GPM_MESS_INVALID_ARG        "%s: Invalid arg. \"%s\" to \"%s\""
#define GPM_MESS_CONT_WITH_ERR      "%s: Continuing despite errors in option parsing"
#define GPM_MESS_TOO_MANY_OPTS      "%s: Too many options for \"-t %s\""

#define GPM_MESS_NETM_NO_ACK        "netmouse: No acknowledge (got %d)"
#define GPM_MESS_NETM_INV_MAGIC     "netmouse: Invalid reply magic (got %d,%d,%d)"
#define GPM_MESS_WACOM_MACRO        "WACOM Macro: %d"
#define GPM_MESS_GUNZE_INV_PACK     "gunze: invalid packet \"%s\""
#define GPM_MESS_MMAN_DETECTED      "MouseMan detected"
#define GPM_MESS_INIT_GENI          "initializing genitizer"
#define GPM_MESS_WACOM_MOD          "WACOM Mode: %c Modell: %s (Answer: %s)"
#define GPM_MESS_IMPS2_INIT         "imps2: PS/2 mouse failed init"
#define GPM_MESS_IMPS2_FAILED       "imps2: PS/2 mouse failed (3 button) init"
#define GPM_MESS_IMPS2_MID_FAIL     "imps2: PS/2 mouse failed to read id, assuming standard PS/2"
#define GPM_MESS_IMPS2_SETUP_FAIL   "imps2: PS/2 mouse failed setup, continuing..."
#define GPM_MESS_IMPS2_BAD_ID       "imps2: Auto-detected unknown mouse type %d, assuming standard PS/2"
#define GPM_MESS_GUNZE_WRONG_BAUD   "%s: %s: wrong baud rate, using 19200"
#define GPM_MESS_GUNZE_CALIBRATE    "%s: gunze: calibration data absent or invalid,using defaults"
#define GPM_MESS_TOO_MANY_SPECIAL   "%s: too many special functions (max is %i)"
#define GPM_MESS_SYNTAX_1           "%s: %s :%i: Syntax error"
#define GPM_MESS_UNKNOWN_MOD_1      "%s: %s :%i: Unknown modifier \"%s\""
#define GPM_MESS_INCORRECT_COORDS   "%s: %s :%i: Incorrect chord \"%s\""
#define GPM_MESS_INCORRECT_LINE     "%s: %s :%i: Incorrect line:\"%s\""
#define GPM_MESS_FIRST_DEV          "Use -m device -t protocol [-o options]!"
#define GPM_MESS_ELO_CALIBRATE      "%s: etouch: calibration file %s absent or invalid, using defaults"

/* warnings */
#define GPM_MESS_REQUEST_ON         "Request on vc %i > %i"
#define GPM_MESS_FAILED_CONNECT     "Failed gpm connect attempt by uid %d for vc %s"
#define GPM_MESS_ZERO_SCREEN_DIM    "zero screen dimension, assuming 80x25"
#define GPM_MESS_STRANGE_DATA       "Data on strange file descriptor %d"
#define GPM_MESS_RESIZING           "%s pid %i is resizing :-)"
#define GPM_MESS_KILLED_BY          "%s pid %i killed by a new %s"
#define GPM_MESS_REDEF_COORDS       "%s: %s :%i: Warning: Chord \"%s%s%s\" redefined"

/* IOCTL */
#define GPM_MESS_IOCTL_KDGETMODE    "ioctl(KDGETMODE)"
#define GPM_MESS_IOCTL_TIOCLINUX    "ioctl(TIOCLINUX)"
#define GPM_MESS_IOCTL_TIOCSTI      "%s: ioctl(TIOCSTI): %s"

/* debug */
#define GPM_MESS_PEER_SCK_UID       "peer socket uid = %d"
#define GPM_MESS_LONG_STATUS        "Pid %i, vc %i, ev %02X, def %02X, minm %02X, maxm %02X"
#define GPM_MESS_SKIP_DATAP         "Skipping a data packet: Data %02x %02x %02x %02x"
#define GPM_MESS_KILLING            "Trying to kill gpm running at pid %d"

/* other */
#define GPM_MESS_SKIP_PASTE         "Skipping paste; selection has aged"
#define GPM_EXTRA_DATA              "Extra %02x"
#define GPM_MESS_SIGNED             "Signed."
#define GPM_MESS_CSELECT            "is your kernel compiled with CONFIG_SELECTION on?"
#define GPM_MESS_NOTHING_MORE       "Nothing more"
#define GPM_MESS_CON_REQUEST        "Request on %i (console %i)"
#define GPM_MESS_SCREEN_SIZE        "Screen size: %d - %d"
/* #define GPM_MESS_                   "" */

/* functions */
#ifdef __GNUC__
__attribute__((__format__(printf, 4, 5)))
#endif
void gpm_report(int line, const char *file, int stat, const char *text, ... );

/* rest of wd.h */
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#else
#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7
#endif

#endif /* _MESSAGE_H */
