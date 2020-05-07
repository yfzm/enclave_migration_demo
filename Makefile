all: libs helloworld kvstore
	make -C libs

libs:
	make -C libs

helloworld: libs
	make -C helloworld

kvstore: libs
	make -C kvstore

clean:
	make -C libs clean
	make -C helloworld clean

.PHONY: all clean libs helloworld kvstore
