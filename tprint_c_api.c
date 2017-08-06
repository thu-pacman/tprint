#ifdef TPRT_SLAVE
#include<slave.h>
#endif
#include"tprint_common.h"

////////////////////////////////////
//
// output switch
//
////////////////////////////////////

inline void tprint_set_master(int n)
{
  tprt_lock();
  tprtsw.master = n;
  tprt_unlock(NULL);
}

inline int tprint_get_master()
{
  tprt_lock();
  int ret = tprtsw.master;
  tprt_unlock(NULL);
  return ret;
}

inline void tprint_set_proc(int n)
{
  tprt_lock();
  tprtsw.proc = n;
  tprt_unlock(NULL);
}

inline int tprint_get_proc()
{
  tprt_lock();
  int ret = tprtsw.proc;
  tprt_unlock(NULL);
  return ret;
}

inline long long tprint_code_smask(int *a)
{
  long long x = 0;
  int i;
  tprt_lock();
  for (i = 63; i >= 0; i --)
  {
    if (a[i] == 0) x &= 0;
    else x |= 1; 
    x <<= 1;
  }
  tprt_unlock(NULL);
  return x;
}

inline void tprint_decode_smask(long long x, int* a)
{
  int i;
  tprt_lock();
  for (i = 0; i < 64; ++ i)
  {
    a[i] = x & 1;
    x >>= 1;
  }
  tprt_unlock(NULL);
}

inline void tprint_set_smask(long long x)
{
  tprt_lock();
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

inline long long tprint_get_smask()
{
  tprt_lock();
  long long ret = tprtsw.smask;
  tprt_unlock(NULL);
  return ret;
}

#ifdef TPRT_SLAVE
inline void tprint_set_slave(int n)
{
  tprt_lock();
  int tid = athread_get_id(-1);
  tprtsw.st[tid] = n;
  if (n == 1) tprtsw.smask |= (1<<n);
  else tprtsw.smask &= ~(1<<n);
  tprt_unlock(NULL);
}

inline int tprint_get_slave()
{
  tprt_lock();
  int tid = athread_get_id(-1);
  int ret = tprtsw.st[tid];
  tprt_unlock(NULL);
  return ret;
}
#endif

inline void tprint_ctrl_on(int proc, int master, long long x)
{
  tprt_lock();
  tprtsw.proc = proc;
  tprtsw.master = master;
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

inline void tprint_ctrl_off()
{
  tprt_lock();
  tprtctrl_v = 0;
  tprt_unlock(NULL);
}


////////////////////////////////////////////////
//
// Following several function can be call from
// both master and slave
//
///////////////////////////////////////////////

#define TLOCK_VFPRT(fp, xfmt)\
  TWRAP(\
      va_list ap; \
      va_start(ap, fmt);\
      tprt_lock();\
      vfprintf(fp, xfmt, ap);\
      tprt_unlock(fp);\
      va_end(ap);\
      )

// tfprintf provide a simple function as fprintf
// however, tfprintf is exclusive (with lock/unlock)
// all outputs through tfprintf in CG are serialized
void tfprintf(FILE* fp, char* fmt, ...)
{
  TPRT_SWITCH;
  TLOCK_VFPRT(fp, fmt);
}

// see tfprintf, output to stdout
void tprintf(char* fmt, ...)
{
  TPRT_SWITCH;
  TLOCK_VFPRT(stdout, fmt);
}

// to stderr, unmaskable
void tprintfe(char* fmt, ...)
{
  //TPRT_SWITCH;
  TLOCK_VFPRT(stderr, fmt);
}

// printf with color
void tprintfc(const char* color, char* fmt, ...)
{
  TPRT_SWITCH;
#ifdef TPRT_SLAVE
  int tid = athread_get_id(-1);
#else
  int tid = 64;
#endif
  char* nfmt = tprtfmt[tid];
  sprintf(nfmt, "%s%s%s", color, fmt, TPRT_NOCOLOR);
  TLOCK_VFPRT(stdout, nfmt);
}

// printf with counter
void tprintfn(int *counter, char* fmt, ...)
{
  TPRT_SWITCH;
#ifdef TPRT_SLAVE
  int tid = athread_get_id(-1);
#else
  int tid = 64;
#endif
  va_list ap; 
  va_start(ap, fmt);
  tprt_lock();
  int cnt = counter ? (*counter)++ : tprt_default_counter++;
  char* nfmt = tprtfmt[tid];
  sprintf(nfmt, "(%2d) %s", cnt, fmt);
  vprintf(nfmt, ap);
  tprt_unlock(stdout);
  va_end(ap);
}

// printf to fix position, useful to show state
void tprintfs(int height, int width, char* fmt, ...)
{
  TPRT_SWITCH;

#if 0
  if (height <= 0 || width <= 0)
  {
    tprt_lock();
    if (fp_tprintfs)
      fclose(fp_tprintfs);
    tprt_unlock(stdout);
    return;
  }
#endif

#if 0
tprt_lock();
  if (!fp_tprintfs)
  {
    printf("fp_tprintfs == NULL, open new file\n");

    fp_tprintfs = fopen("FP_TPRINTFS.dat", "a");
  }
tprt_unlock(NULL);
#endif
  fp_tprintfs = stdout;
  if (!fp_tprintfs) 
  {
    tprintfe("FILE FOR TPRINTFS IS NULL\n");
    return;
  }

#ifdef TPRT_SLAVE
  int tid = athread_get_id(-1);
#else
  int tid = 64;
#endif
  char* nfmt = tprtfmt[tid];
  if (tid == 64)
  {
    sprintf(nfmt, "\033[%d;%dH(MASTER)%s", 
        height*9, 0, fmt);
  }
  else
  {
    sprintf(nfmt, "\033[%d;%dH%s", 
        height*((tid&0x38)>>3), width*(tid&0x7), fmt);
  }
  TLOCK_VFPRT(fp_tprintfs, nfmt);
}

#ifdef TPRT_SLAVE

////////////////////////////////////////////////////
//
// Following functions are only available for slaves.
//
////////////////////////////////////////////////////

// tprintfi has [id] prefix in each output
// if the string passed to tprintfi is not end with '\n', 
// the output will be very strange.
void tprintfi(char *fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  char* nfmt = tprtfmt[tid];
  sprintf(nfmt, "<%2d> %s", tid, fmt);
  TLOCK_VFPRT(stdout, nfmt);
}

void tprintfic(const char* color, char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  char* nfmt = tprtfmt[tid];
  sprintf(nfmt, "<%2d> %s%s%s", tid, color, fmt, TPRT_NOCOLOR);
  TLOCK_VFPRT(stdout, nfmt);
}

void tprintfci(const char* color, char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  char* nfmt = tprtfmt[tid];
  sprintf(nfmt, "%s<%2d> %s%s", color, tid, fmt, TPRT_NOCOLOR);
  TLOCK_VFPRT(stdout, nfmt);
}

void tprintfin(int *counter, char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  va_list ap; 
  va_start(ap, fmt);
  tprt_lock();
  int cnt = counter ? (*counter)++ : tprt_default_counter++;
  char* nfmt = tprtfmt[tid];
  sprintf(nfmt, "<%2d>(%2d) %s", tid, cnt, fmt);
  vprintf(nfmt, ap);
  tprt_unlock(stdout);
  va_end(ap);
}

void tprintfni(int *counter, char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  va_list ap; 
  va_start(ap, fmt);
  tprt_lock();
  int cnt = counter ? (*counter)++ : tprt_default_counter++;
  char* nfmt = tprtfmt[tid];
  sprintf(nfmt, "(%2d)<%2d> %s", cnt, tid, fmt);
  vprintf(nfmt, ap);
  tprt_unlock(stdout);
  va_end(ap);
}

// tprintfx only output when its tid==xid
void tprintfx(int xid, char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  if (tid == xid) 
  {
    char* nfmt = tprtfmt[tid];
    sprintf(nfmt, "<%2d> %s", tid, fmt);
    TLOCK_VFPRT(stdout, nfmt);
  } 
}

void tprintfxc(int xid, const char* color, char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  if (tid == xid) 
  {
    char* nfmt = tprtfmt[tid];
    sprintf(nfmt, "<%2d> %s%s%s", tid, color, fmt, TPRT_NOCOLOR);
    TLOCK_VFPRT(stdout, nfmt);
  } 
}

void tprintfcx(const char* color, int xid, char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  if (tid == xid) 
  {
    char* nfmt = tprtfmt[tid];
    sprintf(nfmt, "%s<%2d> %s%s", color, tid, fmt, TPRT_NOCOLOR);
    TLOCK_VFPRT(stdout, nfmt);
  } 
}

// tprintfx only output when its tid==0
void tprintfz(char* fmt, ...)
{
  TPRT_SWITCH;
  int tid = athread_get_id(-1);
  if (tid == 0) 
  {
    char* nfmt = tprtfmt[tid];
    sprintf(nfmt, "<%2d> %s", tid, fmt);
    TLOCK_VFPRT(stdout, nfmt);
  } 
}

#define T_VSPRT(str, xfmt)\
  TWRAP(\
      va_list ap; \
      va_start(ap, fmt);\
      vsprintf(str, xfmt, ap);\
      va_end(ap);\
      )

// tprintfm similar to tfprintfa(stdout, MATRIX, notitle, fmt, ...)
// first print to a string buffer, then call printf only once,
// this can inhibits the output interleave in MPI situations
void tprintfm(char* fmt, ...)
{
  int tid = athread_get_id(-1);
  T_VSPRT(tprtdata[tid], fmt);
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
    //pm[idm++] = '\n';
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
              strcpy(&pm[idm], TPRT_RED);
              idm += strlen(TPRT_RED);
              int i;
              for (i = 0; i < len; ++ i)
                pm[idm++] = 'X';
    //printf("LINE: %d, append %d X\n", __LINE__, len);
              strcpy(&pm[idm], TPRT_NOCOLOR);
              idm += strlen(TPRT_NOCOLOR);
            }
            else
            {
              if (1 && tprtctrl_v)
              {
                strcpy(&pm[idm], TPRT_GREEN);
                idm += strlen(TPRT_GREEN);
              }
              strncpy(&pm[idm], &tprtdata[idx][sp[ida]], len); 
              idm += len;
              if (1 && tprtctrl_v)
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

// backup for tprintfm
#if 0
void tprintfm(char* fmt, ...)
{
  //TPRT_SWITCH;
  int tid = athread_get_id(-1);
  T_VSPRT(tprtdata[tid], fmt);
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

    int r,c,idx,off = 0;
    // the lock here is for slave[0] and master
    sprintf(tprtbuf+off, "----- TPRINTF MATRIX -----\n");
    off = strlen(tprtbuf);
    for (r = 0; r < 8; ++ r)
    {
      for (c = 0; c < 8; ++ c)
      {
        idx = r*8+c;
        if (tprtsw.st[idx] == 0)
        {
          int kk = 0;
          while (tprtdata[idx][kk] != '\0') 
          {
            tprtdata[idx][kk] = 'X';
            kk ++;
          }
          sprintf(tprtbuf+off, TPRT_RED " %s " TPRT_NOCOLOR, tprtdata[idx]);
        }
        else
        {
          sprintf(tprtbuf+off, " %s ", tprtdata[idx]);
        }
        off = strlen(tprtbuf);
        if (c == 3) 
        {
          sprintf(tprtbuf+off, " | ");
          off = strlen(tprtbuf);
        }
      }
      if (r == 3) 
      {
        sprintf(tprtbuf+off, "\n");
        off = strlen(tprtbuf);
      }
      sprintf(tprtbuf+off, "\n");
      off = strlen(tprtbuf);
    }
    sprintf(tprtbuf+off, "--------------------------\n");
    off = strlen(tprtbuf);
    tprt_lock();
    printf("%s", tprtbuf);
    tprt_unlock(stdout);
  }
  athread_syn(ARRAY_SCOPE, 0xFFFF);
}

#define TPRT_LIST 0x384821
#define TPRT_MATRIX 0x847829
// tprintfa provides a sync and aggregated output from all slave cores
void tfprintfa(FILE* fp, int mode, const char* title, char* fmt, ...)
{
  int tid = athread_get_id(-1);
  athread_syn(ARRAY_SCOPE, 0xFFFF);
  T_VSPRT(tprtdata[tid], fmt);

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

    // the lock here is for slave[0] and master
    if (mode == TPRT_LIST)
    {
      int i, off = 0;
      sprintf(tprtbuf+off, "----- TPRINTF LIST -----\n");
      off = strlen(tprtbuf);
      sprintf(tprtbuf+off, "-- %s --\n", title);
      off = strlen(tprtbuf);
      for (i = 0; i < 64; ++ i)
      {
        sprintf(tprtbuf+off, "[%2d] %s\n", i, tprtdata[i]);
        off = strlen(tprtbuf);
      }
    }
    else if (mode == TPRT_MATRIX)
    {
      int r,c,idx,off = 0;
      // the lock here is for slave[0] and master
      sprintf(tprtbuf+off, "----- TPRINTF MATRIX -----\n");
      off = strlen(tprtbuf);
      for (r = 0; r < 8; ++ r)
      {
        for (c = 0; c < 8; ++ c)
        {
          idx = r*8+c;
          sprintf(tprtbuf+off, " %s ", tprtdata[idx]);
          off = strlen(tprtbuf);
          if (c == 3) 
          {
            sprintf(tprtbuf+off, " | ");
            off = strlen(tprtbuf);
          }
        }
        if (r == 3) 
        {
          sprintf(tprtbuf+off, "\n");
          off = strlen(tprtbuf);
        }
        sprintf(tprtbuf+off, "\n");
        off = strlen(tprtbuf);
      }
      sprintf(tprtbuf+off, "--------------------------\n");
      off = strlen(tprtbuf);
    }
    else
    {
      fprintf(fp, "Unkown output format\n");
    }
    tprt_lock();
    fprintf(fp, "%s", tprtbuf);
    tprt_unlock(fp);
  }
  athread_syn(ARRAY_SCOPE, 0xFFFF);
}
#endif

#endif
