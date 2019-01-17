# Parallel-Snappy-Decompression
This is a modify Snappy compression/decompression algorithm from Google
(https://github.com/andikleen/snappy-c).

Two main changes are:
1) Optimize the sequential dependency in the decompression by adding selection of
copy position during the compression procedure. The modified compression algorithm
is Snappy compatable. A threshold is added to decide whether the hash table can be
updated.

Changes of orignal files: snappy.c

The optimized version can be switched off by turn of the CHAINTHRESH parameter during
compile time.

2) Parallel decompressing multiple files.
Compression: multiple files are compressed and merge into a single file with some
meta data added at the begin of file stating the starting byte of each copmressed
file.
Decompression: decompressed the merged compressed file into multiple uncompressed files.
The multi-thread version is implemented in 'pthread'.
New files:
    mulcom.c    : compress multiple files are merge all the compressed files into one file
    muldec.c    : decompress the compressed-and-merge file into original files
    pmd.c       : multi-thread version of muldec.c
    lock.c      : lock for multi-thread in different platform (especially for IBM POWER)

The algorithm has been test in:
1. Intel Xeon E5-2670 v2 processors
2. IBM POWER 8
3. IBM POWER 9

Authors
Jian Fang (fangjian.alpc@gmail.com)
Jianyu Chen (chenjy0046@gmail.com)

