make:
	gcc -g problem_2.c -lpthread

clean:
	rm ./a.out

run: 
	rm ./a.out
	gcc -g problem_2.c -lpthread
	./a.out

runLog: 
	rm ./a.out
	gcc -g problem_2.c -lpthread
	./a.out -t $(time) -s $(seed) > log.txt &
	echo "Log file is ready."
