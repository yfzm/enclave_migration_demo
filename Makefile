all: libs helloworld kvstore filedemo
	make -C libs

libs:
	make -C libs

helloworld: libs
	make -C helloworld

kvstore: libs
	make -C kvstore

filedemo: libs
	make -C filedemo

multithread: libs
	make -C multithread

bzip2: libs
	make -C bzip2

perlbench: libs
	make -C perlbench

mcf: libs
	make -C mcf

milc: libs
	make -C milc

lbm: libs
	make -C lbm

libquantum: libs
	make -C libquantum

sjeng: libs
	make -C sjeng

hmmer: libs
	make -C hmmer

clean:
	make -C libs clean
	make -C helloworld clean
	make -C kvstore clean
	make -C filedemo clean
	make -C multithread clean
	make -C bzip2 clean
	make -C perlbench clean
	make -C mcf clean
	make -C milc clean
	make -C lbm clean
	make -C libquantum clean
	make -C sjeng clean
	make -C hmmer clean

.PHONY: all clean libs helloworld kvstore filedemo multithread bzip2 \
	perlbench mcf milc lbm libquantum sjeng hmmer
