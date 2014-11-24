#PUBLICDIR= /usr0/cs564/public/project
SRC= buf.c hash.c pf.c simulator.c
OBJ= buf.o hash.o pf.o simulator.o
HDR = pftypes.h pf.h simulator.h raid01.h raid0.h	

pflayer.o: $(OBJ)
	ld -r -o pflayer.o $(OBJ)

tests: testhash testpf

testpf: testpf.o pflayer.o
	cc -o testpf testpf.o pflayer.o

testhash: testhash.o pflayer.o
	cc -o testhash testhash.o pflayer.o

$(OBJ): $(HDR)

testhash.o: $(HDR)

testpf.o: $(HDR)
testraid.o: $(HDR)
testbackup.o: $(HDR)
testfailure.o: $(HDR)

lint: 
	lint $(SRC)

install: pflayer.o 

testraid: testraid.o pflayer.o
	cc -o testraid testraid.o pflayer.o

testbackup: testbackup.o pflayer.o
	cc -o testbackup testbackup.o pflayer.o

testfailure: testfailure.o pflayer.o
	cc -o testfailure testfailure.o pflayer.o

