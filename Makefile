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

clean:
	make -C libs clean
	make -C helloworld clean

.PHONY: all clean libs helloworld kvstore filedemo
