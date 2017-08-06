subroutine func()
  implicit none
  integer :: a, b, tid
  real(8) :: f
!$omp threadprivate(/local_g1/)
  a = 3
  b = 9
  call tprintfm("Print the id = %2d !\n", a);
end
