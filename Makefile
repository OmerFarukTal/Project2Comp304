make:
	gcc problem_2.c -lpthread

clean:
	rm ./a.out

run: 
	rm ./a.out
	gcc problem_2.c -lpthread
	./a.out

