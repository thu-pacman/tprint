# Tprint Library
## Introduction
Tprint is a printing library for programs running on SW-5 architecture, 
which is used by the 2017Jun Top#1 supercomputer Sunway-TaihuLight.
Currently, the library provides full functional C APIs and limited fortran APIs.

A core group (CG) is a biasc computational element in SW-5 arch. 
A CG has 1 MPE (master core) and 64 CPE (co-process core), and **athread** library is used for parallelism.
However, athread library does not handle the neccessary synchronization for printing, 
so the default `printf` statement will produce unreadable result.

Tprint library provides synchronization to ensure meaningful output.
Tprint also provides a variety of functions, including color, selected threads, and 8x8 matrix output format.


## Compile and Test
see **Makefile** for how to build it.

Type `make run` to see C example.

Type `make sub_f` for fortran example.

## Use
* `#include "tprint.h"`
* Link your program with `libtprint.a`

The usage of APIs is described by the comments in `tprint.h`.

## Example Ouput
    ======== tprintfm prints matrixes ===========
    ........ you must fortmat arguments .........
    ........ to make same width elements ........
    ........ DO NOT USE '.' AFTER ARG ...........
              0   1   2   3   4   5   6   7             0    3    6    9   12   15   18   21  
              8   9  10  11  12  13  14  15            24   27   30   33   36   39   42   45  
             16  17  18  19  20  21  22  23            48   51   54   57   60   63   66   69  
             24  25  26  27  28  29  30  31            72   75   78   81   84   87   90   93  
             32  33  34  35  36  37  38  39            96   99  102  105  108  111  114  117  
             40  41  42  43  44  45  46  47           120  123  126  129  132  135  138  141  
             48  49  50  51  52  53  54  55           144  147  150  153  156  159  162  165  
    My id =  56  57  58  59  60  61  62  63 , id*3 =  168  171  174  177  180  183  186  189 
    ---------------------------------------------

