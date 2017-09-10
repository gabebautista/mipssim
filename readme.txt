Files
=====

mips_load.c	load instructions from a file.  
mipssim.c	Simulator. It is the file you will be working on. 
mipssim.h	Header file.

Compile and Run
========

To compile the project, type 

make

Then, you should be able to run the simulator. Try

./mipssim -c -l example.s
./mipssim -c -r example.s
./mipssim -cr example.s

The meaning of the options: 

-c  Print out information every cycle. 
-r  Show registers too.
-l  Print out the instruction list before starting the simulation. 

If you decide to use Java, you can parse the output of this program
(with -l option). So you do not need to deal with labels.   
