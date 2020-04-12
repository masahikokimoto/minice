#ifndef _MINICE_H_
#define _MINICE_H_

#include "config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include <stdio.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#include <signal.h>
#include <errno.h>
#include <sys/file.h> 
#include <sys/ioctl.h> 
#include <termios.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_LIBSHOUT
#include <shout/shout.h>
#endif

/* Äê¿ô */
#ifndef TRUE
#define TRUE                 (1)
#endif
#ifndef FALSE
#define FALSE                (0)
#endif
#define FAIL                 (-1)
#define UNDEFINED            (-1)

/* Global configuration */
struct _gconf_ {
  char *server;
  int port;
  char *user;
  char *name;
  char *genre;
  int public;
  char *url;
  char *password;
  char *mountpoint;
  int bitrate;
  int verbose;
  char *player;
  char *encoder;
  char *playlist;
  int refresh;
  int junkheader;
  int authtype;
} gconf;

/* playlist */
struct _pl_entry_ {
  char *arg;
  struct _pl_entry_ *next;
};

struct _pl_entry_ *pl_entry;
struct _pl_entry_ *pl_head;

/*
               pl_entry ===+ (current pointer)
                           |
pl_head  ==> +----------+  +=> +----------+
         +=> |_pl_entry_|  +=> |_pl_entry_|
         |   +----------+  |   +----------+ 
         |   |char *arg |  |   |char *arg |  
         |   |*next     |--+   |*next     |--+
         |   +----------+      +----------+  |
         |                                   |
         +-----------------------------------+
*/

/* socket for server connection*/
int ssock;

/* flags */
int children_ended;
int time_expired;

/* time of last reconnected */
int last_reconnected;

/* pids of children */
int encoder_pid, player_pid;

/* transimit buffer */
#define TRANSBUFSIZ 32768
char transbuf[TRANSBUFSIZ];

#ifndef ETCDIR
#define ETCDIR                          "/etc"
#endif

#define CONFIGFILE  "minice.conf"
#define DEFAULTCONFIGFILE ETCDIR "/" CONFIGFILE

#endif /* _MINICE_H_ */
