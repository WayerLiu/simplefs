.PHONY: simplefs 
simplefs: 
	gcc -DEVAL -static -std=c99 -pg -g -O0 -o simplefs simplefs.c -lm 


