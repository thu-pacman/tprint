.PHONY: clean all
HCC = sw5cc.new -host
SCC = sw5cc.new -slave -DTPRT_SLAVE
HFC = sw5f90.new -host
SFC = sw5f90.new -slave
CFLAGS = -O3
FFLAGS = -O3
LDFLAGS = -L. -ltprint
AR = swar

all: libtprint.a ftest.out ctest.out

libtprint.a: tprint_common.c tprint_c_api.c tprint_fortran_api.c
	$(HCC) $(CFLAGS) tprint_common.c -c -o a.o
	$(HCC) $(CFLAGS) tprint_c_api.c -c -o b.o
	$(HCC) $(CFLAGS) tprint_fortran_api.c -c -o c.o
	$(SCC) $(CFLAGS) tprint_common.c -c -o d.o
	$(SCC) $(CFLAGS) tprint_c_api.c -c -o e.o
	$(SCC) $(CFLAGS) tprint_fortran_api.c -c -o f.o
	$(AR) -crv $@ a.o b.o c.o d.o e.o f.o

ftest.out: f_master.f90 f_slave.f90
	$(HFC) $(FFLAGS) f_master.f90 -c -o f_master.o
	$(SFC) $(FFLAGS) f_slave.f90 -c -o f_slave.o
	$(HFC) -hybrid f_master.o f_slave.o -o ftest.out $(LDFLAGS)

ctest.out: c_master.c c_slave.c
	$(HCC) $(FFLAGS) c_master.c -c -o c_master.o
	$(SCC) $(FFLAGS) c_slave.c -c -o c_slave.o
	$(HCC) -hybrid c_master.o c_slave.o -o ctest.out $(LDFLAGS)

clean:
	rm *.out *.o *.a -f

sub_f:
	bsub -I -b -q q_sw_expr -n 1 -np 1 -cgsp 64 ./ftest.out

sub_c:
	bsub -I -b -q q_sw_expr -n 1 -np 1 -cgsp 64 ./ctest.out

sub:
	make sub_f && make sub_c

test:
	make ctest.out && make sub_c

run: 
	make clean && make libtprint.a ctest.out && make sub_c
