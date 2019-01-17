#!\bin\sh

# download the project and make
echo "-- Downloading the software from github"
git clone https://github.com/jian-fang/Parallel-Snappy-Decompression.git
echo "-- Downloading Done!"
echo
echo "-- Building software"
cd Parallel-Snappy-Decompression/src
make clean
make all
echo "-- Building Done!"
echo

inputfile=$1

# build the benchmark file
# the last parameter is the output file
echo "-- Makeing decompression benchmark file"
./mulcom ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile ../testdata/$inputfile \
../testdata/input.csnp
echo "-- Benckmark file Done!"
echo

echo "-- Setting core mapping for IBM POWER8 in tacc"
echo "160 0 8 16 24 32 1 9 17 25 33 2 10 18 26 34 3 11 19 27 35 4 12 20 28 36 5 13 21 29 37 6 14 22 30 38 7 15 23 31 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159" > mapping.txt
echo "-- Core mapping Done!"
echo

# running the decompression test
echo "-- Running decompression test"
echo "--------------------- 1 thread ---------------------" > ../../result.log
./pmd 1 ../testdata/input.csnp ../testdata/output >> ../../result.log
echo "--------------------- 5 threads---------------------" >> ../../result.log
./pmd 5 ../testdata/input.csnp ../testdata/output >> ../../result.log
echo "--------------------- 10 threads---------------------" >> ../../result.log
./pmd 10 ../testdata/input.csnp ../testdata/output >> ../../result.log
echo "--------------------- 20 threads---------------------" >> ../../result.log
./pmd 20 ../testdata/input.csnp ../testdata/output >> ../../result.log
echo "--------------------- 40 threads SMT8---------------------" >> ../../result.log
./pmd 40 ../testdata/input.csnp ../testdata/output >> ../../result.log
echo "--------------------- 160 threads ---------------------" >> ../../result.log
./pmd 160 ../testdata/input.csnp ../testdata/output >> ../../result.log
echo "-- Decompression test Done!"
echo

# checking decompression result
echo "-- Checking decompression result"
cd ../testdata
diff -q output0.txt $inputfile && echo "The decompression output matched the original file!" || echo "ERROR: The decompression output doesn't match the original file!" >> ../../result.log
echo "-- Result checking Done!"
echo

echo "Please check the result.log file for the result"
