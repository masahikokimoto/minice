/*
 * minice -- small icecast encoder for stethocast/MP3
 * 
 * Copyright (c) 2001-2006, Masahiko KIMOTO,
 *                          Ohno Laboratory.
 * 
 * Programmed by Masahiko KIMOTO (2001-2006)
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Ohno Laboratory
 *      and Communications Research Laboratory.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "minice.h"

int
writen(int fd, char *ptr, int nbytes)
{

  int nleft, nwritten;

  nleft = nbytes;
  while (nleft > 0) {
    nwritten = write(fd, ptr, nleft);
    if (nwritten <= 0){
      return nwritten;
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  
  return (nbytes - nleft);
}

void
read_playlist()
{
  FILE *fp;
  char buff[BUFSIZ], *ptr;
  struct _pl_entry_ *npl, *tail;

  if (gconf.playlist == NULL) return;

  fp = fopen(gconf.playlist, "r");
  if (fp == NULL) {
    perror("fopen playlist");
    gconf.playlist = NULL;
    return;
  }

  tail = pl_head = NULL;
  while (feof(fp) == 0) {
    if (fgets(buff, sizeof(buff), fp) == NULL) {
      break;
    }
    /* 改行コードの削除 */
    ptr = buff;
    while(*ptr != '\r' && *ptr != '\n' && *ptr != 0) {ptr++;}
    *ptr = 0;
    
    /* remove trail spaces */
    ptr = buff; 
    while(*ptr != 0) {ptr++;}
    if (ptr != buff) {
      while((*ptr == '\n' || *ptr == ' ' || *ptr == '\t') && ptr != buff) {
	ptr--;
      }
      if (ptr != buff) {*(ptr+1) = 0;}
    }

    if (buff[0] == 0) continue;

    npl = malloc(sizeof(struct _pl_entry_));
    if (pl_head == NULL) {
      pl_head = tail = npl;
    }
    npl->next = pl_head;
    npl->arg = strdup(buff);
    tail->next = npl;
    tail = npl;
    /*    printf("ent: %s\n", npl->arg);fflush(stdout);*/
  }
  fclose(fp);

  pl_entry = pl_head;
}

int
connect_to_server()
{
  int s,err;
  char buff[BUFSIZ];
  unsigned long ia;
  struct sockaddr_in sai;
  struct hostent *he;

  sai.sin_family = AF_INET;
  sai.sin_port = htons(gconf.port);

  /* XXX: not suitable for IPv6 */
  if ((ia = inet_addr(gconf.server)) != INADDR_NONE) {
    *(unsigned long *)&(sai.sin_addr) = ia;
  } else {
    he = gethostbyname(gconf.server);
    memcpy((char *)&(sai.sin_addr),
           (char *)(he->h_addr_list[0]),
           he->h_length);
  }
  
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) return -1;
  
  /* If fail to connect peer, return condition */
  do {
    err=connect(s, (struct sockaddr *)&sai,
                sizeof(struct sockaddr));
  } while ((err == -1) && (errno == EINTR)); 
  if (err == -1) return -1;

  return s;
}

int
greeting()
{
  char buffer[BUFSIZ];

  /* failsafe */
  if (ssock < 0) FALSE;

  if (gconf.authtype) {
    /* icy login */
    
    sprintf(buffer, "%s\n",gconf.password);
    writen(ssock, buffer, strlen(buffer));
    
    /* OK from server */
    read(ssock,buffer, 3);  /* "OK\n" */
    buffer[2]=0;
    if (buffer[0] != 'O' && buffer[0] != 'o') {
      fprintf(stderr,"server didn't send OK response"); fflush(stderr);
      return FALSE;
    }
    
    sprintf(buffer, "icy-br:%d\n", gconf.bitrate);
    writen(ssock, buffer, strlen(buffer));
    sprintf(buffer, "icy-name:%s\n",gconf.name);
    writen(ssock, buffer, strlen(buffer));
    sprintf(buffer, "icy-genre:%s\n",gconf.genre);
    writen(ssock, buffer, strlen(buffer));
    sprintf(buffer, "icy-url:%s\n",gconf.url);
    writen(ssock, buffer, strlen(buffer));
    sprintf(buffer, "icy-pub:%d\n",gconf.public);
    writen(ssock, buffer, strlen(buffer));
    sprintf(buffer, "\n");
    writen(ssock, buffer, strlen(buffer));
    return TRUE;
  } else {
    /* x-audiocast style headers */
    /* send password */
    sprintf(buffer,"SOURCE %s ",gconf.password);
    writen(ssock, buffer, strlen(buffer));
    
    /* send the mountpoint string */
    sprintf(buffer,"/%s\n\n", gconf.mountpoint);
    write(ssock, buffer, strlen(buffer));
    
    /* send the Bitrate */
    sprintf(buffer, "x-audiocast-bitrate:%d\n", gconf.bitrate);
    writen(ssock, buffer, strlen(buffer));

    /* stream name */
    sprintf(buffer, "x-audiocast-name:%s\n",gconf.name);
    writen(ssock, buffer, strlen(buffer));
    
    /* genre */
    sprintf(buffer, "x-audiocast-genre:%s\n",gconf.genre);
    writen(ssock, buffer, strlen(buffer));
    
    /* URL */
    sprintf(buffer, "x-audiocast-url:%s\n",gconf.url);
    writen(ssock, buffer, strlen(buffer));
    
    /* Public or private? */
    sprintf(buffer, "x-audiocast-public:%d\n",gconf.public);
    writen(ssock, buffer, strlen(buffer));
    
    /* Description string */
    sprintf(buffer, "x-audiocast-description:%s\n","minice");
    writen(ssock, buffer, strlen(buffer));      
        
    sprintf(buffer, "x-audiocast-contentid:%s\n","minice");
    writen(ssock, buffer, strlen(buffer));
    
    sprintf(buffer, "\n");
    writen(ssock, buffer, strlen(buffer));
    return TRUE;
  }
}

/*
  my_exec(char *cmd)
  cmd: プログラム, 空白で区切られた引き数の文字列
*/
static void
my_exec(char *args)
{
  char targs[BUFSIZ]; 
  char *rargs[256]; /* 数は適当 */
  char *ptr;
  int i,j,argc;

  if (gconf.verbose > 0) {
    fprintf(stderr,"exec: %s\n", args);fflush(stderr);
  }

  strncpy(targs, args, sizeof(targs));

  targs[BUFSIZ - 1] = 0; /* 念のため */
  ptr = targs;

  rargs[0] = NULL;
  argc = 0;

  while (1) {
    /* 空白以外の文字を探す */
    while ((*ptr == ' ' || *ptr == '\t') && *ptr != 0) {ptr++;};
    if (*ptr == 0) {break;}

    /* 引き数として配列にいれる */
    rargs[argc++] = ptr;

    /* 空白を探す */
    while (*ptr != ' ' && *ptr != '\t' && *ptr != 0) {ptr++;};
    if (*ptr == 0) {break;}
    *ptr++ = 0;
  }
  rargs[argc] = NULL;

  execvp(rargs[0], rargs);
}

/*
  playerとencoderをpipeでつなげて実行する
 */
int
spawn_player_and_encoder(int outp)
{
  int p[2], pid, status;
  char junk[4] = {0x01,0x01,0x01,0x01};
  char buff[BUFSIZ];
  
  pipe(p);

  pid = fork();
  if (pid == 0) {
    /* encoder child process */
    close(0);
    dup(p[0]);
    close(1);
    dup(outp);
    close(2);
    close(p[0]); close(p[1]); close(outp);

    /* encoderを実行する */
    my_exec(gconf.encoder);
    exit(1);
  }
  if (pid < 0) {
    return FALSE;
  }
  encoder_pid = pid;
  if (gconf.verbose > 0) {
    fprintf(stderr,"encoder_pid: %d\n", encoder_pid);fflush(stderr);
  }

  if (gconf.playlist != NULL) {
    /* playlistが指定されていたら、その内容を順番に引き数にあたえる */
    sprintf(buff,gconf.player,pl_entry->arg);
    pl_entry = pl_entry->next;
  }

  pid = fork();

  if (pid == 0) {
    /* player child process */
    close(1);
    dup(p[1]);
    close(2);
    close(p[0]); close(p[1]); close(outp);

    /* 先頭の0を省略されないようにするために、ゴミデータを送る */
    if (gconf.junkheader) {
      write(1,junk,4);
    }

    /* playerを実行する */
    if (gconf.playlist != NULL) {
      /* playlistが指定されていたら、その内容を順番に引き数にあたえる */
      my_exec(buff);
    } else {
      my_exec(gconf.player);
    }
    exit(1);
  }
  if (pid < 0) {
    if (encoder_pid > 0) {
      kill(encoder_pid, SIGTERM);
      if (waitpid(encoder_pid, &status, WNOHANG) != encoder_pid) {
      }
    }
    return FALSE;
  }
  player_pid = pid;
  if (gconf.verbose > 0) {
    fprintf(stderr,"player_pid: %d\n", player_pid);fflush(stderr);
  }
  return TRUE;
}

/*
  一旦icecastとの接続をcloseして、1/10秒待ってから再度接続する。
 */
reconnect_server()
{
  if (gconf.verbose > 0) {
    fprintf(stderr, "Reconnecting to server.\n");fflush(stderr);
  }
  shutdown(ssock,2);
  close(ssock);

  ssock = -1;
  usleep(100*1000);

  ssock=connect_to_server();
  if (ssock < 0) {
    perror("reconnect to server");
    terminate();
  }
  if (greeting() == FALSE) {
    terminate();
  }

  /* 最終接続時刻を更新する */
  last_reconnected = time(NULL);
}

void
set_timeout()
{
  /* setitimer/alarm ??? */

  struct itimerval ival;

  ival.it_interval.tv_sec = 
    ival.it_value.tv_sec = gconf.refresh;
  ival.it_interval.tv_usec = 
    ival.it_value.tv_usec = 0;

  if (setitimer(ITIMER_REAL, &ival,NULL) == -1) {
    perror("setitimer");
    /* error */
  }
}

void
sigchld_handler()
{
  fprintf(stderr,"SIGCHLD\n");fflush(stderr);
  children_ended = TRUE;
}

void
sigpipe_handler()
{
  children_ended = TRUE;
}

void
sigalrm_handler()
{
  time_expired = TRUE;
}

void
sigterm_handler()
{
  terminate();
}

terminate()
{
  cleanup_children();
  if (ssock >= 0) {
    shutdown(ssock,2);
    close(ssock);
  }
  fprintf(stderr,"Terminated.\n");
  exit(1);
}

cleanup_children()
{
  int status;

  if (gconf.verbose > 0) {
    fprintf(stderr,"Cleaning up children (%d, %d).\n",
	    player_pid, encoder_pid); fflush(stderr);
  }
  if (player_pid > 0) {
    usleep(100*1000); /* work around */
    kill(player_pid, SIGTERM);
    if (waitpid(player_pid, &status, WNOHANG) != player_pid) {
    }
  }

  if (encoder_pid > 0) {
    usleep(100*1000); /* work around */
    kill(encoder_pid, SIGTERM);
    if (waitpid(encoder_pid, &status, WNOHANG) != encoder_pid) {
    }
  }
}

restart_player_and_encoder(int outp)
{
  int now;

  /* 残っている child proces を掃除する */
  /* 本当はここでcleanupしてしまって良いのかどうかは、ちょっと微妙。
     二つのプロセスが綺麗に終るのを待った方が良いのでは… */
  cleanup_children();

  /* もし一定時間以上、接続しつづけていたら、再接続 */
  now = time(NULL);
  if (gconf.refresh > 0 && now >= (last_reconnected + gconf.refresh)) {
    /* last_reconnectedはreconnect_server()で再定義される */
    reconnect_server();
  }
  set_timeout();
  spawn_player_and_encoder(outp);
}


#ifdef HAVE_LIBSHOUT
mainloop()
{
  int p[2], len;
  shout_t *shout;
  long readlen, ret, total;
  char mp[BUFSIZ];
  
  pipe(p);
  
  children_ended = FALSE;
  time_expired = FALSE;

  shout_init();
  
  if (!(shout = shout_new())) {
    printf("Could not allocate shout_t\n");
    return 1;
  }
  
  if (shout_set_host(shout, gconf.server) != SHOUTERR_SUCCESS) {
    printf("Error setting hostname: %s\n", shout_get_error(shout));
    return 1;
  }
  
  switch (gconf.authtype) {
  case 1:
    if (shout_set_protocol(shout, SHOUT_PROTOCOL_ICY) != SHOUTERR_SUCCESS) {
      printf("Error setting protocol: %s\n", shout_get_error(shout));
      return 1;
    }
    break;
  case 0:
    if (shout_set_protocol(shout, SHOUT_PROTOCOL_XAUDIOCAST) != SHOUTERR_SUCCESS) {
      printf("Error setting protocol: %s\n", shout_get_error(shout));
      return 1;
    }
    break;
  case 2:
    if (shout_set_protocol(shout, SHOUT_PROTOCOL_HTTP) != SHOUTERR_SUCCESS) {
      printf("Error setting protocol: %s\n", shout_get_error(shout));
      return 1;
    }
    break;
  }
  
  if (shout_set_port(shout, gconf.port) != SHOUTERR_SUCCESS) {
    printf("Error setting port: %s\n", shout_get_error(shout));
    return 1;
  }
  
  if (shout_set_password(shout, gconf.password) != SHOUTERR_SUCCESS) {
    printf("Error setting password: %s\n", shout_get_error(shout));
    return 1;
  }
  sprintf(mp,"/%s",gconf.mountpoint);
  if (shout_set_mount(shout, mp) != SHOUTERR_SUCCESS) {
    printf("Error setting mount: %s\n", shout_get_error(shout));
    return 1;
  }
  
  if (shout_set_user(shout, gconf.user) != SHOUTERR_SUCCESS) {
    printf("Error setting user: %s\n", shout_get_error(shout));
    return 1;
  }
  
  if (shout_set_format(shout, SHOUT_FORMAT_MP3) != SHOUTERR_SUCCESS) {
    printf("Error setting format: %s\n", shout_get_error(shout));
    return 1;
  }
  
  if (shout_set_name(shout, gconf.name) != SHOUTERR_SUCCESS) {
    printf("Error setting name: %s\n", shout_get_error(shout));
    return 1;
  }
  
  if (shout_set_genre(shout, gconf.genre) != SHOUTERR_SUCCESS) {
    printf("Error setting genre: %s\n", shout_get_error(shout));
    return 1;
  }
  
  if (shout_set_url(shout, gconf.url) != SHOUTERR_SUCCESS) {
    printf("Error setting url: %s\n", shout_get_error(shout));
    return 1;
  }
  
  if (shout_set_public(shout, gconf.public) != SHOUTERR_SUCCESS) {
    printf("Error setting public: %s\n", shout_get_error(shout));
    return 1;
  }

  if (shout_set_agent(shout, "minice") != SHOUTERR_SUCCESS) {
    printf("Error setting agent: %s\n", shout_get_error(shout));
    return 1;
  }

  sprintf(mp,"%d",gconf.bitrate);
  if (shout_set_audio_info(shout, SHOUT_AI_BITRATE, mp) != SHOUTERR_SUCCESS) {
    printf("Error setting bitrate: %s\n", shout_get_error(shout));
    return 1;
  }
  
#if 0
  if (shout_set_nonblocking(shout, 1) != SHOUTERR_SUCCESS) {
    printf("Error setting non-blocking mode: %s\n", shout_get_error(shout));
    return 1;
  }
#endif
	
  /* player | encoder | pipe[0] として子プロセスを実行 */
  spawn_player_and_encoder(p[1]);

  /* FDをNONBLOCKING モードにする */
  fcntl(p[0],O_NONBLOCK);

  /* もしこの段階でchild processが死んでいたら、異常終了 */
  if (children_ended == TRUE) {
    terminate();
  }

 restart:
  ret = shout_open(shout);
  if (ret == SHOUTERR_SUCCESS)
    ret = SHOUTERR_CONNECTED;

  while (ret == SHOUTERR_BUSY) {
    printf("Connection pending. Sleeping...\n");
    sleep(1);
    ret = shout_get_connected(shout);
  }
  
  if (ret == SHOUTERR_CONNECTED) {
    printf("Connected to server...\n");
    total = 0;
    while (1) {
      readlen = read(p[0], transbuf, TRANSBUFSIZ);
      total = total + readlen;
      
      if (readlen > 0) {
	ret = shout_send(shout, transbuf, readlen);
	if (ret != SHOUTERR_SUCCESS) {
	  printf("DEBUG: Send error: %s\n", shout_get_error(shout));
	  break;
	}
      } else {
	break;
      }
      
      shout_sync(shout);

      if (children_ended == TRUE) {
      /* encoder/playerが終了したら。signal handler(SIGPIPE, SIGCHLD)で
	 フラグをセット */
	restart_player_and_encoder(p[1]);
	children_ended = FALSE;
	/*      tb_head = tb_tail = 0;*/ /* XXX */
	/*      continue;*/ /* XXX??? */
	
      }
      if (time_expired == TRUE) {
	/* 一定時間以上動き続けたら、一旦サーバとのコネクションを切って
	   張りなおす */
	len = read(p[0], transbuf, TRANSBUFSIZ);
	time_expired = FALSE;
	set_timeout();
	shout_close(shout);
	goto restart;
      }
    }
  } else {
    perror("connect failed.");
    printf("Error connecting: %s\n", shout_get_error(shout));
  }
  
  shout_close(shout);
  
  shout_shutdown();
  
  return 0;
}
#else
mainloop()
{
  int p[2],len,nfds, tb_head, tb_tail, timestep, dt, gtblsz, maxlen;
  char buff[16];
  fd_set ifd,ofd;
  struct timeval tv, ptv, ntv;

  pipe(p);
  
  children_ended = FALSE;
  time_expired = FALSE;
  timestep = 0;
  tb_head = 0;
  tb_tail = 0;

  /* icecastに接続する */
  ssock = connect_to_server();
  if (ssock < 0) {
    perror("connect to server");
    terminate();
  }
  /* icecast への挨拶 */
  if (greeting() == FALSE) {
    terminate();
  }
  last_reconnected = time(NULL);
  set_timeout();

  /* player | encoder | pipe[0] として子プロセスを実行 */
  spawn_player_and_encoder(p[1]);

  /* FDをNONBLOCKING モードにする */
  fcntl(ssock,O_NONBLOCK);
  fcntl(p[0],O_NONBLOCK);


  /* もしこの段階でchild processが死んでいたら、異常終了 */
  if (children_ended == TRUE) {
    terminate();
  }

  gtblsz = getdtablesize();

  /* 無限ループ */
  for(;;) {
    FD_ZERO(&ifd);
    FD_ZERO(&ofd);

    if (tb_tail < TRANSBUFSIZ) {
      /* バッファが一杯でないときだけ読み込みsocketを監視する */
      FD_SET(p[0],&ifd);
    }
    if (timestep == 0 /*(tb_tail != 0 || tb_head != 0)*/) {
      /* バッファが空でないときだけ書き込みsocketを監視する */
      FD_SET(ssock,&ofd);
    }

    if (children_ended != TRUE && time_expired != TRUE) {
      /* 各フラグが立っていないときだけ select()で待ち受ける */

      /* timestep(msec)分だけ待つ */
      tv.tv_sec  = timestep / 1000;
      tv.tv_usec = (timestep % 1000) * 1000;

      /* select()中に割り込みがあったら抜ける */
      siginterrupt(SIGCHLD, TRUE);
      siginterrupt(SIGPIPE, TRUE);
      siginterrupt(SIGALRM, TRUE);
      
      gettimeofday(&ptv,NULL);
      if (timestep == 0) {
	/* 無限に待つ */
	nfds = select(gtblsz, &ifd, &ofd, NULL, NULL);
      } else {
	/* timestep だけ待つ */
	nfds = select(gtblsz, &ifd, &ofd, NULL, &tv);
      }

      if (nfds < 0 && errno == EINTR &&
	  children_ended != TRUE && time_expired != TRUE) {
	continue;
      }
      if (nfds == 0) {
	/* timeout */
	timestep = 0;
	/* ここでcontinueした場合は、siginterruptまでsystem callは
	   ないので、解除する必要はない */
	continue;
      }
      if (timestep != 0) {
	gettimeofday(&ntv, NULL);
	/* selectがtimeoutじゃ無い場合、経過した時間分だけ
	   次回の待ち時間を減らす */
	dt = (ntv.tv_sec * 1000 + ntv.tv_usec / 1000) - 
	  (ptv.tv_sec * 1000 + ptv.tv_usec / 1000);
	if (dt > 0) {
	  timestep -= dt;
	  if (timestep < 0) timestep = 0;
	}
      }

      /* read(), write()中に割り込みがあったらrestart */
      siginterrupt(SIGCHLD, FALSE);
      siginterrupt(SIGPIPE, FALSE);
      siginterrupt(SIGALRM, FALSE);

    }

    /* 読みこめるなら読んでバッファにいれる */
    if (FD_ISSET(p[0], &ifd) != 0) {
      if (tb_tail >= TRANSBUFSIZ) {
	/* read buffer is already full, so discard current buffer. */
	if (gconf.verbose > 0) {
	  fprintf(stderr, "Buffer was expired.\n");fflush(stderr);
	}
	tb_tail = 0;
	tb_head = 0;
      }
      len = read(p[0], transbuf + tb_tail, TRANSBUFSIZ - tb_tail);
      if (len <= 0) {
	/* 子プロセスからの接続が切れたので、再度接続を試みる */
	children_ended = TRUE;
      } else {
	tb_tail += len;
      }
      if (gconf.verbose > 0) {
	fprintf(stderr,"Read from source: %d bytes.\n", len);fflush(stderr);
      }
      if (tb_tail >= TRANSBUFSIZ) {
	if (gconf.verbose > 0) {
	  fprintf(stderr,"Stream buffer is full\n"); fflush(stderr);
	}
	/*continue;*/
      }
    }

    /* 書けるならバッファを書き出す */
    if (FD_ISSET(ssock, &ofd) != 0) {
      if (tb_tail == tb_head) {
#if 0
	/* something should be written, but no data is available
	   in the buffer. */
	tb_tail = tb_head = 0;
	len = gconf.bitrate * (1024/8) / 10;
	memset(transbuf, 0, len);
	len = write(ssock, transbuf, len);
#endif
	timestep = 100;
	continue;
      } else {
	maxlen = tb_tail - tb_head;
	maxlen = ((maxlen > (gconf.bitrate * (1024/8))) ?
	  (gconf.bitrate * (1024/8)) : maxlen);
	len = write(ssock, transbuf + tb_head, maxlen);
      }
      if (len <= 0) {
	/* サーバとの接続が切れたので、再度接続を試みる */
	time_expired = TRUE;
      } else if (len == maxlen) {
	/* バッファが空になったらポインタを全部先頭に戻す */
	tb_tail = tb_head = 0;
      } else {
	tb_head += len;
      }
      /* 次の回は、送ったデータの時間分(msec)だけ待つ */
      timestep = (int)((1000 * len) / (gconf.bitrate * (1024 / 8)));
      if (timestep > 100) timestep -= 10;
      if (gconf.verbose > 0) {
	fprintf(stderr,"len=%d, timestep(ms)=%d\n",len,timestep);
	fflush(stderr);
      }
    }

    if (tb_tail >= TRANSBUFSIZ) {
      /* read buffer is already full, so discard current buffer. */
      if (gconf.verbose > 0) {
	fprintf(stderr, "Buffer was expired.\n");fflush(stderr);
      }
      tb_tail = 0;
      tb_head = 0;
      timestep = 0;
    }

    if (children_ended == TRUE) {
      /* encoder/playerが終了したら。signal handler(SIGPIPE, SIGCHLD)で
	 フラグをセット */
      exit(1);
      restart_player_and_encoder(p[1]);
      children_ended = FALSE;
      /*      tb_head = tb_tail = 0;*/ /* XXX */
      /*      continue;*/ /* XXX??? */
    }
    if (time_expired == TRUE) {
      /* 一定時間以上動き続けたら、一旦サーバとのコネクションを切って
       張りなおす */
      exit(1);
      reconnect_server();
      tb_head = 0;
      tb_tail = 0;
      len = read(p[0], transbuf, TRANSBUFSIZ);
      time_expired = FALSE;
      timestep = 0;
      set_timeout();
    }
  }
}
#endif

main(int argc, char *argv[])
{
  char *configfile;
  struct stat sb;

  ssock = -1;
  player_pid = -1;
  encoder_pid = -1;
  
  init_gconf();

  if (argc == 2) {
    configfile = argv[1];
  } else {
    configfile = CONFIGFILE; /* in current directory */
  }
  if (stat(configfile, &sb) != 0) {
    configfile = DEFAULTCONFIGFILE; /* /usr/local/etc/minice.conf */
    if (stat(configfile, &sb) != 0) {
      configfile = NULL;
    }
  }

  if (configfile == NULL) {
    fprintf(stderr,"Unable to find configuration file.\n"); fflush(stderr);
    exit(1);
  }
  if (read_config_file(configfile) == FALSE) {
    exit(1);
  }
  if (check_gconf() == FALSE) {
    fprintf(stderr,"Configuration is not reasonable\n");
    exit(1);
  }

  read_playlist();

  signal(SIGCHLD, sigchld_handler);
  signal(SIGPIPE, sigpipe_handler);
  signal(SIGTERM, sigterm_handler);
  signal(SIGALRM, sigalrm_handler);

  siginterrupt(SIGCHLD, TRUE);
  siginterrupt(SIGPIPE, TRUE);
  siginterrupt(SIGALRM, TRUE);

  mainloop();
}
