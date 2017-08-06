#ifndef __TPRINT_COMMON_H__
#define __TPRINT_COMMON_H__

#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<string.h>
#include"tprint.h"

extern volatile int tprtlock_v;
extern volatile int tprtctrl_v;
extern volatile int tprt_default_counter;
extern volatile FILE* fp_tprintfs;

#define BUFSZ 512
extern char tprtstrbuf[65][BUFSZ];
extern char tprtfmt[65][BUFSZ];
extern char tprtdata[65][BUFSZ];
extern char tprtbuf[BUFSZ<<4];

#define ARGSZ 256
extern char tprtfarg[65][ARGSZ];
extern char tprtcarg[65][ARGSZ];
extern char tprtargt[65][ARGSZ>>2];

#define TWRAP(stmts) \
  do { stmts } while(0)

#ifdef TPRT_SLAVE
#define TPRT_SWITCH\
  TWRAP(\
      if ( (tprtctrl_v == 1) && \
          ((tprtsw.proc == 0) || (tprtsw.st[athread_get_id(-1)] == 0)))\
        return;\
      )
#else
#define TPRT_SWITCH\
  TWRAP(if (tprtctrl_v == 1 && \
        ((tprtsw.proc == 0 || tprtsw.master == 0))) \
        return;)
#endif

typedef struct tprint_switch_t {
  long long smask;
  int st[64];
  int master;
  int proc;
} TprintSwitch;

extern volatile TprintSwitch tprtsw;

#endif
