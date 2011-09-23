all:
	make -C src
	cp src/cooleyesBridge.prx ../..
	psp-build-exports -s src/cooleyesBridge.exp
	cp -f cooleyesBridge.S ../../src
	cp -f cooleyesBridge.h ../../include

clean:
	make -C src clean
	rm -f cooleyesBridge.prx
	rm -f cooleyesBridge.S
	rm -f *.bak
	rm -f src/*.bak

