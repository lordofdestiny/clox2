all: main main2 main3

libalpha.so: alpha.c
	gcc -shared -o libalpha.so alpha.c

libtest.so: test.c
	gcc -shared -o libtest.so test.c

main: main.c libtest.so
	gcc -o main main.c -L. -Wl,-rpath=. -ltest

main2: main2.c libtest.so
	gcc -o main2 main2.c -rdynamic -DWITH_SAY_HELLO

main3: main2.c libtest.so libalpha.so
	gcc -o main3 main2.c -L. -Wl,--no-as-needed,-rpath=. -lalpha -ltest

clean:
	rm -f main main2 main3 *.so *.zip

zipme:
	zip linking.zip alpha.c main.c main2.c test.c Makefile

.PHONY: all clean zipme
