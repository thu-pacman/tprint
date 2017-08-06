///////////////////////////////////////////////////////////////
//                                                           //
// The Tprint is a library written for SunWay platform.      //
// It provides clear and rich output for C and Fortran       //
// parallel program which suffers garbages from slave cores. // 
//                                                           //
//======= Written by TANG Xiongchao (Tsinghua Univ.) ========//
//                                                           //
// You are free to use/modify/redistribute its source and    //
// binary, but please keep this copyright section in this    //
// header file. And add yourself's if you like.              //
//                                                           //
// Please let me know if you have any questions or ideas     //
// about this tool.                                          //
//                                                           //
// Email: tomxice@gmail.com                                  //
//                                   2015.08.17 @ WuXi,China //
//                                                           //
///////////////////////////////////////////////////////////////
#ifndef __TPRINT_H__
#define __TPRINT_H__

/////////////////////////////////////////////////
//
// Tprint control API
//
/////////////////////////////////////////////////

// turn on the tprint control API
// proc: process mask 
// master: master mask
// smask: slave mask
// tprint_ctrl_on(1, 1, -1) to enable all
void tprint_ctrl_on(int proc, int master, long long smask);
// Fortran binding:
// call tprint_ctrl_on(proc, master, smask_high_32, smask_low_32)
// literal integer in fortran may be 32bit

// turn off the tprint control API
// when tprint control API is off 
// everything will be printed
// just as tprint_ctrl_on(1,1,-1)
void tprint_ctrl_off();
// Fortran binding: the same

// set process mask
// if proc mask is set to 0, this process will not output
// if proc mask is set to 1, it depends on master and smask
void tprint_set_proc(int);
// Fortran binding: the same

// get current process mask
int tprint_get_proc();
// Fortran binding: 
// call tprint_get_proc(proc_mask)
// fotran passes arguments by references

// set master output mask
// 0 for disable master's output, 1 for enable
// master mask is less prior than process mask
// so master will output iff.
// ctrl_off || (proc_on && master_on)
void tprint_set_master(int n);
// Fortran binding: the same

// get current master output mask
int  tprint_get_master();
// Fortran binding: 
// call tprint_get_master(master_mask)

// similar to set/get master, only available on slave
void tprint_set_slave(int n);
int tprint_get_slave();
// Fortran binding:
// call tprint_get_slave(slave_mask)

// encode a 64-bit smask from int a[64]
// a[i] stands for mask of slave i
// 0 for disable and 1 for enable
// this func doesn't change output settings
long long tprint_code_smask(int* a);
// NO FORTRAN BINDING

// decode a 64-bit smask into a[64]
// this func doesn't change output settings
void tprint_decode_smask(long long, int* a);
// NO FORTRAN BINDING

// set slave mask to x
// the i-th lowest bit of x stands to slave i
// 0 for disable and 1 for enable
void tprint_set_smask(long long x);
// Fortran binding:
// call tprint_set_smask(smask_high_32, smask_low_32)

// get current slave mask
long long tprint_get_smask();
// Fortran binding:
// call tprint_get_smask(smask_high_32, smask_low_32)

/////////////////////////////////////////////////
//
// Tprint output API
//
// all the APIs use lock/unlock to ensure there is
// only one core is printing (for each CG on sw 
// machine, multi-processes output should be handled
// as normal by MPI runtime).
//
// all APIs are available on C, and some for Fortran.
//
/////////////////////////////////////////////////

// these macros can be used as args to tprint colorful output
#define TPRT_NOCOLOR "\033[0m"
#define TPRT_RED "\033[1;31m"
#define TPRT_GREEN "\033[1;32m"
#define TPRT_YELLOW "\033[1;33m"
#define TPRT_BLUE "\033[1;34m"
#define TPRT_MAGENTA "\033[1;35m"
#define TPRT_CYAN "\033[1;36m"
#define TPRT_REVERSE "\033[7m"

////////////////////////////////////////////
//
// These several API available on 
// both master and slave core
//
////////////////////////////////////////////

// better than printf
void tprintf(char* fmt, ...);
// Fortran binding: 
// same API as for C, but fmt must ends with something like
// '\n', '\0' that contains an backslash in second last position.
// no matter whether fmt has a '\n', tprintf will give it an '\n'.
// this is due to the string conversion between C and Fortran.
// still looking for better solution.

// output to stderr, unmaskable
void tprintfe(char* fmt, ...);
// Fortran binding:
// same API as for C.

// another choice of fprintf
void tfprintf(FILE* fp, char* fmt, ...);
// Fortran binding:
// it has the same API as for C, but what's the FILE* in fortran ?

// printf with color
// color feature depends on your term
void tprintfc(const char* color, char* fmt, ...);
// NO FORTRAN BINDING

// printf with counter
// use the default counter if counter == NULL
void tprintfn(int* counter, char* fmt, ...);
// NO FORTRAN BINDING

// printf in fix position,
// use to show states
void tprintfs(int height, int width, char* fmt, ...);
// NO FORTRAN BINDING

/////////////////////////////////////////////
//
// Following API only available on slave core
//
/////////////////////////////////////////////

// Fortran binding
// only tprintfi, tprintfx, tprintfz and tprintfm
// have fortran binding (same API as for C).
// color option is not available on fortran.

// printf with [id] as prefix
void tprintfi(char* fmt, ...);
// [id] COLORFUL(...)
void tprintfic(const char* color, char* fmt, ...);
// COLORFUL([id] ...)
void tprintfci(const char* color, char* fmt, ...);

// printf with (counter) as prefix,
// counter ++ each time.
void tprintfin(int *counter, char* fmt, ...);
void tprintfni(int *counter, char* fmt, ...);

// printf when id == xid
void tprintfx(int xid, char* fmt, ...);
void tprintfxc(int xid, const char* color, char* fmt, ...);
void tprintfcx(const char* color, int xid, char* fmt, ...);

// printf when id == 0
void tprintfz(char* fmt, ...);

// This will format the args of 64 slaves into an 8x8 matrix
// if all slaves' output are disabled, nothing will be print.
// if some slaves still print something, tprintfm still 
// prints a matrix, with red 'X's on disabled cores.
void tprintfm(char* fmt, ...);

#endif
