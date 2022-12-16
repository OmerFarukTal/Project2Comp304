COMP 304 Christmas: Project 2
Ömer Veysel ÇAĞATAN – ocagatan19	Ömer Faruk TAL – otal19

A simple task which aims to organize Christmas Gifts with multi-threaded and synchronized system.
Makefile:
make: compile file, filename = a.out
clean: remove the compile file (a.out)
run: clean previous a.out file, recompile it, then run with regular parameters
runLog: clean precious a.out file, recompile it, then run with user defined parameters, write them into log.txt in background, print message to user when it is done.
Note=> If “make run”  does not work, then use “make make” then “make run”. Probably the a.out file is missing.  
Example=> “make runLog time=120 seed=10”
Queue.c:
Task struct is modified to have flag variables for each task required by the santa and elfs. Every Task also has a startTime value for logs. Initially rquired task according to the gift type are 0, when they are handled by santa or elfs flag variables become 1.
Nodes_t are changed to store a pointer instead of the task itself. With this change the same gift which is in different task queues can be updated synchronously. 

Problem_2.c:
Main:
Main is responsible for creating the the threads and destroying them. After creating thread main thread sleeps for simulationTime and then it destroys all of the threads created.
ControlThread:
Control thread has a while loop where it sleeps for 1 seconds. Then using the rand() function it generates a random number between 0 and 100 which used for randomized task creation.
Task->***Done parameters are initialized according to needs of the given type. Tasks are enqueued into painting, assembly, QA and package queues regarding their needs. At each 30 second created task is pushed into New Zealand Queue. When a task is enqueued, it is done with a lock since santa and elves are dequeuing and check if the queue is empty.
Control thread also responsible for updating second which is handled with a lock mechanism since the other threads are also accessing it.
Santa and Elf:
Santa and elves have a shared variable which all updates and read, taskID. taskID is used in the log part where we keep track pf printed message number. They all access the taskId with a lock. 
When santa and elves are dealing with taks pointers they also use a locking mechanism, since when they update variable (say paintingDone), other thread can access and read it.

Santa: 
Santa first deals with the New Zealand tasks, since it prioritizes the delivery santa first checks if delivery queue is empty or not with a lock. If it is empty it releases the lock and continues with QA queue. If not it dequeue one task from the delivery queue and sleeps 1 second and changes taks->deliveryDone for safety. It then checks the current time with lock and calculate turn around time with task->startTime and print log accordingly. 
Since the delivery is prioritized, before entering into QA Tasks santa checks if there is not still element in the delivery queue or if there are more than 3 elements in the QA queue. When one of the condition is met, the QA tasks are handled; otherwise santa continues to handle delivery tasks.
Then santa check for QA task where it also check if it is empty or not with a lock. If it is empty it release lock if not it dequeue one element and then it release the locks. After it sleeps 1 second, updates task->QADone to be 1. Then it updates the startTime of the task with current second.
A task can only enter from QA, assembly and painting queue to package queue only if the tasks QADone, paintingDone and assemblyDone flags are equals to 1. So santa checks if other tasks are handled or not. If they are handled, santa sent the task into package queue to be packaged by elfs. Otherwise, if at least one the tasks are not done; santa leave this job to elves. 
After dealing with New Zealand tasks santa again checks for them, if there is still a element in this queue santa again deals with New Zealand tasks, otherwise it continue with normal tasks where it does the same job.

ElfA and ElfB:
Elves first deals with the New Zealand tasks, since they prioritize the package, elves first checks if package queue is empty or not with a lock. If it is empty they release the lock and continues with painting/assembly queue. If not it dequeue one task from the package queue and sleeps 1 second and changes task->packageDone. They then check the current time with lock and calculate turn around time with task->startTime and print log accordingly. After printing the lock since the packaging is done the elves enqueue this task into delivery queue, which santa uses to handle delivery jobs.
Since the packeage is prioritized for elves, before entering into painting or assembly Tasks elves checks if there is not still element in the package queue. If it is empty elves continue with the assembly and painting tasks, otherwise elves continues to handle painting/assembly tasks.
Then elves check for painting/assembly task where they also check if it is empty or not with a lock. If it is empty it release lock if not it dequeue one element and then it release the locks. After it sleeps 1 second, updates task->paintingDone or taks->assemblyDone to be 1. Then they updates the startTime of the task with current second.
As it is explained before for a task to be enqueued into package queue the all of the painting, assembly and QA tasks need to be done. So, elves checks for that condition, if contidion met they enqueue the task int package queue else
After dealing with New Zealand tasks elves again checks for them, if there is still an element in this queue elves again deals with New Zealand tasks, otherwise they continue with normal tasks where they do the same job.




