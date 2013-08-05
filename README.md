silokatana
==========

Fast key-value Store Based and Storage Engine for Shinobi Dev

Bahan Racikannya : 

1. Log-Structure Merge Tree Data Structure

2. SAX-Hash Algorithm

3. DJB Hash Algorithm

4. Bernstein Bloom Filter for Hiraishin Jutsu

5. SILO method bottling data

6. Datareq Skip Indexing for Jikukan Kekai

7. Level Cache Marking Jutsu Indexing Data for Jutsu-Shiki (Markingnya si Minato itu looooh)  


HOW TO :
========

make all = Create all

make silo-benchmark = Create benchmark file binary

make clean = Delete all object

Yaaaah silahken diliat sajalah Makefilenya


STEP BY STEP :
==============

1. Open your terminal first

2. git clone https://github.com/stnmrshx/silokatana.git

3. cd silokatana/

4. make silo-benchmark 'OR' make all

5. ./silo-benchmark write 1000000
6. ./silo-benchmark read 1000000

5. To remove obj files and storage just type 'make clean && make cleandb'


BENCHMARKnya :
==============
./silo-benchmark write 1000000 - Nulis key sejuta a.k.a 1000K

./silo-benchmark read 1000000 - Baca key sejuta a.k.a 1000K

Ya masih dalam tahap prototype, ntar ditambah2 lagi kok



TODO LIST
==========
1. Create DBServer
2. Create Binding Storage
3. Poto-poto Unyu
