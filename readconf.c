#include "minice.h"

/*
  SERVER a.b.c.d
  PORT xx
  NAME xx
  GENRE xx
  PUBLIC xx
  URL xx
  PASSWORD xx
  BITRATE xx
  VERBOSE xx
  PLAYER xxx
  ENCODER xxx
  PLAYLIST xxx
  MOUNTPOINT xxx
 */

void
init_gconf()
{
  gconf.server = "127.0.0.1";
  gconf.port = 8001;
  gconf.user = "";
  gconf.name = "";
  gconf.genre = "";
  gconf.public = 0;
  gconf.url = "";
  gconf.password = "";
  gconf.mountpoint = "";
  gconf.bitrate = 0;
  gconf.verbose = 0;
  gconf.player = NULL;
  gconf.encoder = NULL;
  gconf.playlist = NULL;
  gconf.mountpoint = "/";
  gconf.refresh = 60*60; /* 1h */
  gconf.junkheader = 0;
  gconf.authtype = 0; /* x-audio */
}

int
check_gconf()
{
  if (gconf.server == NULL ||
      gconf.port == 0 ||
      gconf.bitrate == 0 ||
      gconf.player == NULL ||
      gconf.encoder == NULL) return FALSE;
  return TRUE;
}


int
read_config_file (char *filename)
{
  int err,line;
  FILE *fp;
  char buffer[BUFSIZ];
  char *ptr, *ptr1;

  fp = fopen(filename,"r");
  if (fp == NULL) {
    perror("fopen config file");
    return FALSE;
  }

  err = TRUE;
  line = 0;
  while(feof(fp) == 0) {
    buffer[BUFSIZ-1] = 0;
    if (fgets(buffer,sizeof(buffer),fp) < 0) {
      err = FALSE;
      break;
    }
    line ++;

    /* コメントと改行コードの削除 */
    ptr = buffer;
    while(*ptr != '#' && *ptr != '\n' && *ptr != '\r' && *ptr != 0) {ptr++;}
    *ptr = 0;
    
    /* remove trail spaces */
    ptr = buffer; 
    while(*ptr != 0) {ptr++;}
    if (ptr != buffer) {
      while((*ptr == '\n' || *ptr == ' ' || *ptr == '\t') && ptr != buffer) {
	ptr--;
      }
      if (ptr != buffer) {*(ptr+1) = 0;}
    }
    
    /* 明らかな空行は無視 */
    if (buffer[0] == 0) {
      continue;
    }
    
#define SKIP_SPC(x) {while((*(x)==' ' || *(x)=='\t') && *(x)!=0){(x)++;}}
    ptr = buffer;
    
    if (strncmp("server",buffer,strlen("server")) == 0) {
      ptr += 6;
      SKIP_SPC(ptr);
      gconf.server = strdup(ptr);
      continue;
    }
    
    if (strncmp("port",buffer,strlen("port")) == 0) {
	  ptr += 4;
      SKIP_SPC(ptr);
      gconf.port = strtol(ptr,NULL,10);
      continue;
    }

    if (strncmp("name",buffer,strlen("name")) == 0) {
      ptr += 4;
      SKIP_SPC(ptr);
      gconf.name = strdup(ptr);
      continue;
    }

    if (strncmp("genre",buffer,strlen("genre")) == 0) {
      ptr += 5;
      SKIP_SPC(ptr);
      gconf.genre = strdup(ptr);
      continue;
    }

    if (strncmp("user",buffer,strlen("user")) == 0) {
      ptr += 4;
      SKIP_SPC(ptr);
      gconf.user = strdup(ptr);
      continue;
    }

    if (strncmp("public",buffer,strlen("public")) == 0) {
      ptr += 6;
      SKIP_SPC(ptr);
      gconf.public = strtol(ptr,NULL,10);
      continue;
    }

    if (strncmp("url",buffer,strlen("url")) == 0) {
      ptr += 3;
      SKIP_SPC(ptr);
      gconf.url = strdup(ptr);
      continue;
    }

    if (strncmp("password",buffer,strlen("password")) == 0) {
      ptr += 8;
      SKIP_SPC(ptr);
      gconf.password = strdup(ptr);
      continue;
    }

    if (strncmp("mountpoint",buffer,strlen("mountpoint")) == 0) {
      ptr += 10;
      SKIP_SPC(ptr);
      gconf.mountpoint = strdup(ptr);
      continue;
    }

    if (strncmp("bitrate",buffer,strlen("bitrate")) == 0) {
      ptr += 7;
      SKIP_SPC(ptr);
      gconf.bitrate = strtol(ptr,NULL,10);
      continue;
    }

    if (strncmp("verbose",buffer,strlen("verbose")) == 0) {
      ptr += 7;
      SKIP_SPC(ptr);
      gconf.verbose = strtol(ptr,NULL,10);
      continue;
    }

    if (strncmp("player",buffer,strlen("player")) == 0) {
      ptr += 6;
      SKIP_SPC(ptr);
      gconf.player = strdup(ptr);
      continue;
    }

    if (strncmp("encoder",buffer,strlen("encoder")) == 0) {
      ptr += 7;
      SKIP_SPC(ptr);
      gconf.encoder = strdup(ptr);
      continue;
    }

    if (strncmp("playlist",buffer,strlen("playlist")) == 0) {
      ptr += 8;
      SKIP_SPC(ptr);
      gconf.playlist = strdup(ptr);
      continue;
    }

    if (strncmp("refresh",buffer,strlen("refresh")) == 0) {
      ptr += 7;
      SKIP_SPC(ptr);
      gconf.refresh = strtol(ptr,NULL,10);
      continue;
    }

    if (strncmp("authtype",buffer,strlen("authtype")) == 0) {
      ptr += 8;
      SKIP_SPC(ptr);
      if (strcmp(ptr, "x-audio") == 0) {
	gconf.authtype = 0;
	continue;
      } else if (strcmp(ptr, "icecast2") == 0) {
	gconf.authtype = 2;
	continue;
      } else if (strcmp(ptr, "icy") == 0) {
	gconf.authtype = 1;
	continue;
      }
    }

    if (strncmp("junkheader",buffer,strlen("junkheader")) == 0) {
      gconf.junkheader = 1;
      continue;
    }

    err = FALSE;
    /* Undefined variable */
    fprintf(stderr,"Syntax error in configfile (line: %d).\n", line);
    break;
  }

  fclose(fp);
  return err;
}

