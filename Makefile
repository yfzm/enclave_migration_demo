all:
	make -C libs

helloworld:
	make -C helloworld

clean:
	make -C libs clean
	make -C helloworld clean

.PHONY: all clean helloworld
