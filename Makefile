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

clean:
	make -C libs clean
	make -C helloworld clean
	make -C kvstore clean
	make -C filedemo clean
	make -C multithread clean
	make -C bzip2 clean
	make -C perlbench clean

.PHONY: all clean libs helloworld kvstore filedemo multithread bzip2 perlbench
