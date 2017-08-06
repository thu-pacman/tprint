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

#ifdef TPRT_SLAVE
inline void slv_spin_lock(int* lock_p)
{
  unsigned int __tmp = 0;
  unsigned int __cnt;
  __asm__ __volatile__ (
      "0:     ldw     %[__tmp], %[lock_p]\n"
      "       beq     %[__tmp], 2f\n"
      "       ldi     %[__cnt], 1024\n"
      "       sll     %[__cnt], 4, %[__cnt]\n"
      "1:     subw    %[__cnt], 1, %[__cnt]\n"
      "       bne     %[__cnt], 1b\n"
      "       br      0b\n"
      "2:     faaw    %[__tmp], %[lock_p]\n"
      "       bne     %[__tmp], 0b\n"
      "       memb    \n"
      "       br      3f\n"
      "3:     unop    \n"
      : [__tmp] "=&r" (__tmp), [__cnt] "=&r" (__cnt)
      : [lock_p] "m" (*(lock_p))
      : "memory");
  return;
}
inline void slv_spin_unlock(int *lock_p)
{
  unsigned int __tmp = 0;
  __asm__ __volatile__ (
      "       memb    \n" 
      "       mov     0, %[__tmp]\n"
      "       stw     %[__tmp], %[lock_p]\n"
      : [__tmp] "=&r" (__tmp)
      : [lock_p] "m"  (*(lock_p))
      : "memory"
      );
  return;
}
#endif

inline void tprt_lock()
{
#ifdef TPRT_SLAVE
  slv_spin_lock(&tprtlock_v);
#else
  int ret = -1;
  while (ret != 0)
  {
    while (tprtlock_v != 0);
    ret = tprt_atomic_inc(&tprtlock_v);
  }
#endif
}

inline void tprt_unlock(FILE* fp)
{
  if (fp != NULL)
    fflush(fp);
#ifdef TPRT_SLAVE
  slv_spin_unlock(&tprtlock_v);
#else
  tprtlock_v = 0;
#endif
}

