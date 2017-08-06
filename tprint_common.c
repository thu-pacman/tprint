#include"tprint_common.h"

#ifndef TPRT_SLAVE
volatile int tprtlock_v = 0;
volatile int tprtctrl_v = 0;
volatile int tprt_default_counter = 0;
volatile FILE *fp_tprintfs = NULL;

char tprtstrbuf[65][BUFSZ];
char tprtfmt[65][BUFSZ];
char tprtdata[65][BUFSZ];
char tprtbuf[BUFSZ<<4];

char tprtfarg[65][ARGSZ];
char tprtcarg[65][ARGSZ];
char tprtargt[65][ARGSZ>>2];

volatile TprintSwitch tprtsw;

#endif

#define TWRAP(stmts) \
  do { stmts } while(0)

static inline int tprt_atomic_inc(int* pos) {
  int rt;
#ifdef TPRT_SLAVE
  asm volatile ( "faaw %0, 0(%1) \n"
      "memb"
      : "=&r" (rt)
      : "r" (pos));
#else
  asm volatile ( "ldw_inc %0, 0(%1) \n"
      "memb"
      : "=&r" (rt)
      : "r" (pos));
#endif
  return rt;
}


inline void tprt_lock()
{
  int ret = -1;
  while (ret != 0)
  {
    while (tprtlock_v != 0);
    ret = tprt_atomic_inc(&tprtlock_v);
  }
}

inline void tprt_unlock(FILE* fp)
{
  if (fp != NULL)
    fflush(fp);
  tprtlock_v = 0;
}

