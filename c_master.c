#include<stdio.h>
#include<stdlib.h>
#include<athread.h>
#include"tprint.h"

extern SLAVE_FUN (sfunc) ();

int main(int argc, char** argv)
{
  int i, r, c;
  int spe_cnt;
  athread_init(); 
  spe_cnt = athread_get_max_threads(); 
  if (spe_cnt != 64) 
  {
    printf("This CG cannot afford 64 SPEs (%d only).\n", spe_cnt);
  }
  athread_set_num_threads(spe_cnt);

  // turn contrl apis on 
  // and enables all output
  tprint_ctrl_on(1,1,-1); 
  athread_spawn(sfunc, NULL);
  athread_join();

  athread_halt();
  return 0;
}
