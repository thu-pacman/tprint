#include <slave.h>
#include "tprint.h"

#define DEMOINFO(info) \
  do{\
    athread_syn(ARRAY_SCOPE, 0xFFFF);\
    if (id == 0) \
      tprintf(TPRT_REVERSE info TPRT_NOCOLOR);\
    athread_syn(ARRAY_SCOPE, 0xFFFF);\
  }while (0)

#define POS(row, col) "\033["#row";"#col"H"


void pa(void)
{
  int id, row, col;
  id = athread_get_id(-1);
  row = (id & 0x38) >> 3;
  col = (id & 0x7);


  tprintfz("-------------- Before printing -------------\n");
  tprintfz(POS(0,0)"\033[2J");
  athread_syn(ARRAY_SCOPE, 0xFFFF);
  //printf("\033[s");
  tprintf("\033[%d;%dH""%2d", 3*row, 5*col, id);
  //tprintf(POS(0,0)"Goodbye Earth\n");
  //printf("\033[u");
  tprintf("\033[%d;%dH", 25, 0);
  athread_syn(ARRAY_SCOPE, 0xFFFF);
  tprintfz("############## After printing  #############\n");
  
}

#define PRTS(...)
//#define PRTS(...) tprintfs(4,8,__VA_ARGS__)

void sfunc (void *argp)
{
  int id;
  id = athread_get_id(-1);

#if 1
  DEMOINFO("======== tprintf is just like printf ========\n");
  tprintf("I'm a synergetic core, my id = %d\n", id);
  DEMOINFO("---------------------------------------------\n\n");

  PRTS("%d", __LINE__);
  DEMOINFO("======== use the ctrl api to mask output ====\n");
  tprint_set_slave(id < 4); // output if id < 4
  tprintf("I'm a synergetic core, my id = %d\n", id);
  DEMOINFO("---------------------------------------------\n\n");
  PRTS("%d", __LINE__);

  DEMOINFO("======== tfprintf prints to a file ==========\n");
  tfprintf(stderr, "I'm a synergetic core, my id = %d\n", id);
  DEMOINFO("---------------------------------------------\n\n");
  PRTS("%d", __LINE__);

  tprint_set_slave(id == 0); // output if id == 0
  DEMOINFO("======== tprintfc makes the output colorful =\n");
  tprintfc(TPRT_RED, "I'm a synergetic core, my id = %d\n", id);
  tprintfc(TPRT_GREEN, "I'm a synergetic core, my id = %d\n", id);
  tprintfc(TPRT_BLUE, "I'm a synergetic core, my id = %d\n", id);
  tprintfc(TPRT_CYAN, "I'm a synergetic core, my id = %d\n", id);
  tprintfc(TPRT_MAGENTA, "I'm a synergetic core, my id = %d\n", id);
  tprintfc(TPRT_YELLOW, "I'm a synergetic core, my id = %d\n", id);
  DEMOINFO("---------------------------------------------\n\n");
  PRTS("%d", __LINE__);

  DEMOINFO("======== tprintfi tells who am i ============\n");
  tprintfi("I'm a synergetic core, my id = %d\n", id);
  DEMOINFO("---------------------------------------------\n\n");
  PRTS("%d", __LINE__);

  DEMOINFO("======== tprintfi can combine with color ====\n");
  DEMOINFO("........ could you tell the difference ? ....\n");
  tprintfic(TPRT_GREEN, "I'm a synergetic core, my id = %d\n", id);
  tprintfci(TPRT_GREEN, "I'm a synergetic core, my id = %d\n", id);
  DEMOINFO("---------------------------------------------\n\n");
  PRTS("%d", __LINE__);

  DEMOINFO("======== tprintfn has a counter =============\n");
  int a = 0;
  tprintfn(NULL,"I'am a synergetic core, my id = %d\n", id);
  tprint_set_slave(id == 1);
  tprintfin(NULL,"I'am a synergetic core, my id = %d\n", id);
  tprint_set_slave(id == 2);
  tprintfni(NULL,"I'am a synergetic core, my id = %d\n", id);
  tprint_set_slave(id == 0);
  DEMOINFO("---------------------------------------------\n\n");
  PRTS("%d", __LINE__);

  tprint_set_slave(1); // all enable
  DEMOINFO("======== tprintfx specifies one core ========\n");
  tprintfx(2, "I'm a synergetic core, my id = %d\n", id);
  DEMOINFO("---------------------------------------------\n\n");
  PRTS("%d", __LINE__);

  DEMOINFO("======== tprintfz specifies core 0 ==========\n");
  tprintfz("I'm a synergetic core, my id = %d\n", id);
  DEMOINFO("---------------------------------------------\n\n");
  PRTS("%d", __LINE__);
#endif

#if 1
  DEMOINFO("======== tprintfm prints matrixes ===========\n");
  DEMOINFO("........ you must fortmat arguments .........\n");
  DEMOINFO("........ to make same width elements ........\n");
  DEMOINFO("........ DO NOT USE '.' AFTER ARG ...........\n");

  tprintfm("My id = %2d, id*3 = %3d\n", id, id*3);
  DEMOINFO("---------------------------------------------\n");
  tprint_set_slave(id % 3 == 0);
  tprintfm("My id = %2d, id*3 = %3d\n", id, id*3);
  DEMOINFO("---------------------------------------------\n\n");
#endif
}
