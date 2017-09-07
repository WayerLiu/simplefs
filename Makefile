.PHONY: simplefs 
simplefs: 
	gcc -DEVAL -static -std=c99 -O2 -o simplefs simplefs.c -lm 


