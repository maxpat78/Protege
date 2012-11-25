REPAIR
======

This little C utility was developed in late 2002 to protect data carried in a few floppy disks,
splitted in ZIP archives (sequentially writing a big file on floppy usually produces some bad 
sectors).

It creates a Recovery Block from an input file, XORing & check-summing original file blocks, 
then it tries to detect and repair the original if it contains more, less or corrupted data.

This technique was observed in early editions of RAR archiver.

The package contains some makefiles and batches to compile the utility, both in Linux/GCC and
Visual C++. There is also a CRACK.pl script useful to test the repairing capabilities.


Syntax
======

    Protect or repair a file by a recovery block.
    
    PROTEGE [/R] [/F] [/O:file] [/S:n] [/P:n] file
    
    /R  analyze and try to recover a file
    /F  force partial recovery of a broken sector
    /S  recovery block size (512 byte sectors)
    /P  recovery block size (percentage of input file)
        (tipically, 1%; maximum: 10%)
    /O  recovery file
        (if not specified, appends .SEC to the original)



CRACK.pl syntax
===============

    Repeatedly corrupt & repair a file, verifying results.
    
    CRACK.pl <file> <-RC|-RS|-RI> <times>
    
    <file>	file to operate on (a <file>.BAK backup is always saved)
    -RC		corrupt a random amount of bytes at random places
    -RS		shrink by a random amount of bytes, from a random position
    -RI		increase by a random amount of bytes, from a random position
    <times> number of test repetitions 

        OR
    
    Destroy, remove or insert a random or fixed amount of bytes at a random
    or fixed position in a file.

    CRACK.pl <file> [-C|-S|-I] <pos> <len>

    <file>	file to operate on (a <file>.BAK backup is always saved)
    -C		corrupt
    -S		shrink
    -I		increase
    <pos>	initial damage position
    <len>	damage length


* The code is given to the Public Domain. *
