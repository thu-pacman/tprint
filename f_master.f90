program main
implicit none
integer :: i, j, k
real(8) :: x
integer,external :: slave_func
i = 0
j = -1
k = 37
x = 2.7182
print *, i, j, k

call tprint_ctrl_on(1,1,-1,-1);
call athread_init()

call tprintf("MASTER: %s|%03d|%-5d|%4.2lf\0", "hello", k, 45, x)
call athread_spawn(slave_func, 1)
call athread_join()

call tprint_set_smask(X'01234567', X'89ABCDEF');
call tprintf("MASTER: %s|%03d|%-5d|%4.2lf\0", "hello", k, 45, x)
call athread_spawn(slave_func, 1)
call athread_join()

call athread_halt()
end program 
