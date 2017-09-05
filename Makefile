.PHONY: simplefs 
simpilefs: 
	gcc -DEVAL -static -std=c99 -O2 -o simplefs simplefs.c -lm


