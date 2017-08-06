#ifdef TPRT_SLAVE
#include<slave.h>
#endif
#include"tprint_common.h"

////////////////////////////////////
//
// output switch
//
////////////////////////////////////

inline void tprint_set_master_(int* n)
{
  tprt_lock();
  tprtsw.master = *n;
  tprt_unlock(NULL);
}

inline void tprint_get_master_(int* ret)
{
  tprt_lock();
  *ret = tprtsw.master;
  tprt_unlock(NULL);
}

inline void tprint_set_proc_(int* n)
{
  tprt_lock();
  tprtsw.proc = *n;
  tprt_unlock(NULL);
}

inline void tprint_get_proc_(int* ret)
{
  tprt_lock();
  *ret = tprtsw.proc;
  tprt_unlock(NULL);
}

#define Tmask_H32 (0xFFFFFFFF00000000L)
#define Tmask_L32 (0x00000000FFFFFFFFL)
#define Tmerge(xh, xl)\
  (((((long long)xh)<<32) & Tmask_H32)|(xl & Tmask_L32))

// TODO fortran passed int[] a
static inline void tprint_code_smask_(int *a, int *xhp, int* xlp)
{
  int i;
  int xh = *xhp, xl = *xlp;
  tprt_lock();
  for (i = 63; i >= 32; i --)
  {
    xh <<= 1;
    if (a[i] == 0) xh &= 0;
    else xh |= 1; 
  }
  for (i = 31; i >= 0; i --)
  {
    xl <<= 1;
    if (a[i] == 0) xl &= 0;
    else xl |= 1; 
  }
  *xhp = xh, *xlp = xl;
  tprt_unlock(NULL);
}

// TODO fortran passed int[] a
static inline void tprint_decode_smask_(int* xhp, int* xlp, int* a)
{
  int i;
  long long x = Tmerge(*xhp, *xlp);
  tprt_lock();
  for (i = 0; i < 64; ++ i)
  {
    a[i] = x & 1;
    x >>= 1;
  }
  tprt_unlock(NULL);
}

inline void tprint_set_smask_(int* xhp, int* xlp)
{
  tprt_lock();
  long long x = Tmerge(*xhp, *xlp);

  printf("set smask to %llx \n", x);
  tprtsw.smask = x;
  // do not call, due to double lock
  //tprint_decode_smask(n, tprtsw.st);
  int i;
  for (i = 0; i < 64; ++ i)
  {
    tprtsw.st[i] = x & 1;
    x >>= 1;
  }
  tprt_unlock(NULL);
}

inline void tprint_get_smask_(int *xhp, int* xlp)
{
  tprt_lock();
  int ret = tprtsw.smask;
  *xhp = ret & Tmask_H32;
  *xlp = ret & Tmask_L32;
  tprt_unlock(NULL);
}

#ifdef TPRT_SLAVE
inline void tprint_set_slave_(int *nn)
{
  tprt_lock();
  int tid = athread_get_id(-1);
  int n = *nn;
  tprtsw.st[tid] = n;
  if (n == 1) tprtsw.smask |= (1<<n);
  else tprtsw.smask &= ~(1<<n);
  tprt_unlock(NULL);
}

inline void tprint_get_slave_(int *ret)
{
  tprt_lock();
  int tid = athread_get_id(-1);
  *ret = tprtsw.st[tid];
  tprt_unlock(NULL);
}
#endif

inline void tprint_ctrl_on_(int *proc, int *master, int *xhp, int *xlp)
{
  tprt_lock();
  tprtsw.proc = *proc;
  tprtsw.master = *master;
  long long x = Tmerge(*xhp, *xlp);
  tprtsw.smask = x;
  int i;
  for (i = 0; i < 64; ++ i)
  {
    tprtsw.st[i] = x & 1;
    x >>= 1;
  }
  tprtctrl_v = 1;
  tprt_unlock(NULL);
}

inline void tprint_ctrl_off_()
{
  tprt_lock();
  tprtctrl_v = 0;
  tprt_unlock(NULL);
}

// tprt_mem used for debug, print size bytes of memory from argp
static void tprt_pmem(void* argp, int size)
{
  char* p = (char*)argp;
  int i;
  for (i = 0; i < size; ++ i)
  {
    if (i % 16 == 0)
      printf("%p: ", &p[i]);
    printf("%02x ", p[i] & 0xFF);
    if (i % 8 == 7)
      printf(" | ");
    if (i % 16 == 15)
      printf("\n");
  }
  printf("\n");
}

#define Td(p,x) (**(int**)(p+(x<<3))) //1
#define Tu(p,x) (**(unsigned int**)(p+(x<<3))) //2
#define Tlld(p,x) (**(long long**)(p+(x<<3))) //3
#define Tllu(p,x) (**(unsigned long long**)(p+(x<<3))) //4
#define Tlf(p,x) (**(double**)(p+(x<<3))) //5
#define Tf(p,x) (**(float**)(p+(x<<3))) //6
#define Ts(p,x) (*(char**)(p+(x<<3))) //7
#define Targ(p,x,type) (**(type**)(p+(x<<3))) 
#define Tlen(p,x) (*(int*)(p+(x<<3))) 

// offset of each argument is 8bytes
// using sizeof(type) is wrong
#define Tputarg(k,type,typei) \
  do {\
    *(type*)(&carg[vx]) = k; \
    vx += 8; \
    argt[px] = typei;\
    px++;\
  } while (0)

// tprt_prepare does the following
// 1. parses the fmt and modifies it ending to '\n\0'
// 2. dereferences the arguments from pointer to value
//    however, char* is still char*
// 3. store the arguments into tprtargv
// return value is the length of tprtcarg (args to vprintf)
int  tprt_prepare(int tid, char *fmt, void *p)
{
  int idx = 0, s = 0, px = 0, vx = 0, i;
  char c;
#ifndef TPRT_SLAVE
  tid = 64;
#endif
  char* strbuf = tprtstrbuf[tid];
  char* carg = tprtcarg[tid];
  char* argt = tprtargt[tid];

  for (i = 0; i < (ARGSZ >> 2); ++i) argt[i] = 0;
  Tputarg(fmt, char*, 7);
  // we assume that any '\' means the end of the format string
  // such as "\n", "\r\n", "\0"
  while ((c = fmt[idx++]) != '\0')
  {
    //printf("Processing fmt[%d] = %c\n", idx-1, c);
    switch(s)
    {
      case 0:
        if (c == '%') s = 1;
        else if (c == '\\') 
        {
          fmt[idx-1] = '\n';
          fmt[idx] = '\0';
        }
        break;
      case 1:
        switch (c)
        {
          case 's':
            Tputarg(Ts(p,px),char*,7);
            s = 0;
            break;
          case 'd':
            Tputarg(Td(p,px),int,1);
            //printf("get %%d = %d, vx = %d\n", *(int*)(&tprtcarg[vx-sizeof(int)]), vx);
            s = 0;
            break;
          case 'u':
            Tputarg(Tu(p,px),unsigned int,2);
            s = 0;
            break;
          case 'f':
            Tputarg(Tf(p,px),float,6);
            s = 0;
            break;
          case 'l':
            s = 2;
            break;
          default:
            break;
        }
        break;
      case 2:
        if (c == 'l')
        {
          s = 3;
        }
        else if (c == 'f')
        {
          Tputarg(Tlf(p,px), double, 5);
          //printf("%lf, %lf\n", d, *(double*)(&argv[vx-sizeof(double)]));
          //Tputarg(Tlf(p,px),double,5);
          s = 0;
        }
        break;
      case 3:
        if (c == 'd')
        {
          Tputarg(Tlld(p,px),long long,3);
          s = 0;
        }
        else if (c == 'u')
        {
          Tputarg(Tllu(p,px),unsigned long long,4);
          s = 0;
        }
        break;
      default:
        break;
    }     
  }
  // turncate strings with their real length
  // the length args will not be passed to C
  // NOTE: the the fmt is already turncated
  int pxa = px;
  px ++; // skip fmt
#ifdef TPRT_SLAVE
  int bufp = 0;
  for (i = 1; i < pxa; ++ i)
  {
    if (argt[i] == 7)
    {
      int len = Tlen(p,px);
      char *str = Ts(p,i);
      //printf("[%d] has a string (len = %d), px = %d\n", i, len, px);
      strncpy(&strbuf[bufp], str, len);
      strbuf[len] = '\0';
      // here we assume each carg occupies 8 bytes
      *(char**)(&carg[i<<3]) = &(strbuf[bufp]);
      bufp += (len>>2 + 1)<<2;
      px++;
    }
  }
#else
  for (i = 1; i < pxa; ++ i)
  {
    if (argt[i] == 7)
    {
      int len = Tlen(p,px);
      char *str = Ts(p,i);
      str[len] = '\0';
      px++;
    }
  }
#endif
  //printf("vx = %d\n", vx);
  return vx;
}

#define TLOCK_CONV_VFPRT(fp, xfmt)\
  TWRAP(\
      tprt_lock();\
      memcpy(tprtfarg[tid], parg, vx);\
      memcpy(parg, tprtcarg[tid], vx);\
      va_list ap;\
      va_start(ap, fmt);\
      vfprintf(fp, fmt, ap);\
      va_end(ap);\
      memcpy(tprtfarg[tid], tprtcarg[tid], vx);\
      tprt_unlock(fp);\
      )

void tfprintf_(FILE* fp, char* fmt, ...)
{
  TPRT_SWITCH;
#ifdef TPRT_SLAVE
  int tid = athread_get_id(-1);
#else 
  int tid = 64;
#endif
  char* xfmt = tprtfmt[tid];
  strncpy(xfmt, fmt, BUFSZ);
  void* parg = &fmt; 
  int vx = tprt_prepare(tid, xfmt, parg);
  TLOCK_CONV_VFPRT(fp, fmt);
}

void tprintf_(char* fmt, ...)
{
  TPRT_SWITCH;
#ifdef TPRT_SLAVE
  int tid = athread_get_id(-1);
#else 
  int tid = 64;
#endif
  char* xfmt = tprtfmt[tid];
  strncpy(xfmt, fmt, BUFSZ);
  void* parg = &fmt; 
  int vx = tprt_prepare(tid, xfmt, parg);
  TLOCK_CONV_VFPRT(stdout, fmt);
}

void tprintfe_(char* fmt, ...)
{
  //TPRT_SWITCH;
#ifdef TPRT_SLAVE
  int tid = athread_get_id(-1);
#else 
  int tid = 64;
#endif
  char* xfmt = tprtfmt[tid];
  strncpy(xfmt, fmt, BUFSZ);
  void* parg = &fmt; 
  int vx = tprt_prepare(tid, xfmt, parg);
  TLOCK_CONV_VFPRT(stdout, fmt);
}

#ifdef TPRT_SLAVE
void tprintfx_(int xid, char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  if (tid == xid) 
  {
    char* xfmt = tprtfmt[tid];
    memset(xfmt, 0, BUFSZ);
    sprintf(xfmt, "<%2d> ", tid);
    int pre_len = strlen(xfmt);
    xfmt += pre_len;
    strncpy(xfmt, fmt, BUFSZ - pre_len);
    xfmt -= pre_len;
    void* parg = &fmt; 
    int vx = tprt_prepare(tid, xfmt, parg);
    TLOCK_CONV_VFPRT(stdout, xfmt); 
  }
}

void tprintfz_(char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  if (tid == 0) 
  {
    char* xfmt = tprtfmt[tid];
    memset(xfmt, 0, BUFSZ);
    sprintf(xfmt, "<%2d> ", tid);
    int pre_len = strlen(xfmt);
    xfmt += pre_len;
    strncpy(xfmt, fmt, BUFSZ - pre_len);
    xfmt -= pre_len;
    void* parg = &fmt; 
    int vx = tprt_prepare(tid, xfmt, parg);
    TLOCK_CONV_VFPRT(stdout, xfmt); 
  }
}

void tprintfi_(char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  char* xfmt = tprtfmt[tid];
  memset(xfmt, 0, BUFSZ);
  sprintf(xfmt, "<%2d> ", tid);
  int pre_len = strlen(xfmt);
  xfmt += pre_len;
  strncpy(xfmt, fmt, BUFSZ - pre_len);
  xfmt -= pre_len;
  void* parg = &fmt; 
  int vx = tprt_prepare(tid, xfmt, parg);
  TLOCK_CONV_VFPRT(stdout, xfmt); 
}

#define T_CONV_VSPRT(str, xfmt)\
  TWRAP(\
      memcpy(tprtfarg[tid], parg, vx);\
      memcpy(parg, tprtcarg[tid], vx);\
      va_list ap;\
      va_start(ap, fmt);\
      vsprintf(str, fmt, ap);\
      va_end(ap);\
      memcpy(tprtfarg[tid], tprtcarg[tid], vx);\
      )

#define T_VSPRT(str, xfmt)\
  TWRAP(\
      va_list ap; \
      va_start(ap, fmt);\
      vsprintf(str, xfmt, ap);\
      va_end(ap);\
      )

void tprintfm_(char* fmt, ...)
{
  int tid = athread_get_id(-1);
  char* xfmt = tprtfmt[tid];
  strncpy(xfmt, fmt, BUFSZ);
  void* parg = &fmt; 
  int vx = tprt_prepare(tid, xfmt, parg);
  T_CONV_VSPRT(tprtdata[tid], xfmt);
  athread_syn(ARRAY_SCOPE, 0xFFFF);
  
  // after count(mask) cores have done their output format
  if (tid == 0)
  {
    // skip tprintfa if none slave core will output
    if ( (tprtctrl_v == 1) &&
        ((tprtsw.proc == 0) || (tprtsw.smask == 0)))
    {
      athread_syn(ARRAY_SCOPE, 0xFFFF);
      return;
    }

    // parse fmt  
    char c;
    int cp = 0, s = 0, cnt = 0;
    int sp[10], ep[10];
    char* p0 = tprtfmt[0];
    char* p1 = tprtbuf;
    strncpy(p0, fmt, BUFSZ);
    //printf("p0:|%s|\n", p0);
    memset(p1, 0, BUFSZ);

    //printf("LINE: %d\n", __LINE__);

    while (1)
    {
      c = p0[cp];
      if (0)
      {
      if (c != '\0')
        printf("LINE: %d, s = %d, p0[%d] = '%c'\n", __LINE__, s, cp, c);
      else 
        printf("LINE: %d, s = %d, p0[%d] = '\\0'\n", __LINE__, s, cp);
      }
      switch(s)
      {
        case 0:
          if (c == '%') 
          {
            sp[cnt] = cp;
            s = 1;
          }
          break;
        case 1:
          // end of this arg
          if (c == ' ' || c == '\t' || c == '\n' || 
              c == '%' || c == '\0' || c == ','  || 
              c == ':' || c == ';')
          {
            p0[cp] = '\0';
            T_VSPRT(p1, p0);
            //T_VSPRT(&p1[ep[cnt-1]], p0);
            //printf("LINE: %d, p1 = |%s|\n", __LINE__,p1);
            p0[cp] = c;
            int len = strlen(p1);
            ep[cnt] = len;
            cnt ++;
            s = 0; 
            strcpy(&p1[len], &p0[cp]);
            char *tmp = p0;
            p0 = p1;
            p1 = tmp;
            cp = len-1; 
          }
          break;
        default:
          break;
      } 
      if (c == '\0') break;
      cp++;
    }

    char* ps = tprtdata[0];
    sp[cnt] = ep[cnt] = strlen(ps);
    int i;
    for (i = 0; i <= cnt; ++ i)
    {
      //printf("sp[%d] = %d, ep[%d] = %d\n", i, sp[i], i, ep[i]);
    }

    char* pm = tprtbuf;
    memset(pm, 0, BUFSZ<<4);
    int ids = 0, idm = 0, ida = 0;
    //printf("LINE: %d\n", __LINE__);
    pm[idm++] = '\n';
    int row;
    if (1)
    for (row = 0; row < 8; ++ row)
    {
      ids = 0, ida = 0;
      //printf("ROW = %d, ids = %d, sp[0] = %d\n", row, ids, sp[0]);
      while(ps[ids] != '\0')
      {
        //printf("LINE: %d, ps[%d] = %c\n", __LINE__, ids, ps[ids]);
        if (ids < sp[ida])
        {
          pm[idm] = ((row == 7) ? ps[ids] : ' ');
          idm ++;
          ids ++;
        }
        else 
        {
          int col, idx;
          int len = ep[ida]-sp[ida];
          for (col = 0; col < 8; ++ col)
          {
            idx = (row<<3) + col;
            pm[idm++] = ' ';
            if (tprtctrl_v && (!tprtsw.proc || !tprtsw.st[idx]))
            {
    //printf("LINE: %d, idx %d is disabled\n", __LINE__, idx);
              //strcpy(&pm[idm], TPRT_RED);
              //idm += strlen(TPRT_RED);
              int i;
              for (i = 0; i < len; ++ i)
                pm[idm++] = 'X';
    //printf("LINE: %d, append %d X\n", __LINE__, len);
              //strcpy(&pm[idm], TPRT_NOCOLOR);
              //idm += strlen(TPRT_NOCOLOR);
            }
            else
            {
              if (0 && tprtctrl_v)
              {
                strcpy(&pm[idm], TPRT_GREEN);
                idm += strlen(TPRT_GREEN);
              }
              strncpy(&pm[idm], &tprtdata[idx][sp[ida]], len); 
              idm += len;
              if (0 && tprtctrl_v)
              {
                strcpy(&pm[idm], TPRT_NOCOLOR);
                idm += strlen(TPRT_NOCOLOR);
              }
            }
            idm = strlen(pm);
            pm[idm++] = ' ';
          }
          ids = ep[ida];
          ida ++;
        }
      }
      if (pm[idm-1] != '\n')
        pm[idm++] = '\n';
      //printf("After %d row, pm = |%s|\n", row, pm);
    }
    tprt_lock();
    //printf("------------------\n");
    printf("%s", tprtbuf);
    //printf("------------------\n");
    tprt_unlock(stdout);
  }
  athread_syn(ARRAY_SCOPE, 0xFFFF);
}
#endif

