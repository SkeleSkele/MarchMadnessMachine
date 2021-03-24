mmmc: mmmc.c
	gcc -o mmmc mmmc.c -lm -Wall -std=c11

run: mmmc
	./mmmc -t 10000000 -i 45 -v input.txt

clean:
	rm -f mmmc
