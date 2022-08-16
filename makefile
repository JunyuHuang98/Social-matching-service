 all: central.c serverT.c serverS.c serverP.c clientA.c clientB.c
		gcc -o central central.c
		gcc -o serverT serverT.c
		gcc -o serverS serverS.c
		gcc -o serverP serverP.c
		gcc -o clientA clientA.c
		gcc -o clientB clientB.c -lm  #-lm is to compile math.h used for rounding

central:
		./central

serverT:
		./serverT

serverS:
		./serverS

serverP:
		./serverP

.PHONY: central serverT serverS serverP
