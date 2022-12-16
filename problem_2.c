#include "queue.c"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#ifdef __APPLE__
	#include <dispatch/dispatch.h>
	typedef dispatch_semaphore_t psem_t;
#else
	#include <semaphore.h> // sem_*
	typedef sem_t psem_t;
#endif



int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 30; // frequency of emergency gift requests from New Zealand

void* ElfA(void *arg); // the one that can paint
void* ElfB(void *arg); // the one that can assemble
void* Santa(void *arg); 
void* ControlThread(void *arg); // handles printing and queues (up to you)
void printTask(Task *t);

// Question is do we need seprate locks for each task Queue?

Queue *paintingQ; 
Queue *assemblyQ;
Queue *packageQ;
Queue *QAQ;
Queue *deliveryQ;


pthread_mutex_t lock; // Locks (lock => lock for Queues, lockID lastFinished)
pthread_mutex_t lockID;

// General variables

int second = 0;
int taskID = 1;

pthread_mutex_t lockSecond;
pthread_mutex_t lockTaskID;

// End

// Priror job queue (New Zeland Tasks)

Queue *paintingQPrior; 
Queue *assemblyQPrior;
Queue *packageQPrior;
Queue *QAQPrior;
Queue *deliveryQPrior;


pthread_mutex_t priorQLock;
pthread_mutex_t priorLockID;
// End

// pthread sleeper function
int pthread_sleep (int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
    {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL))
    {
        return -1;
    }
    struct timeval tp;
    //When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;
    
    pthread_mutex_lock(&mutex);
    int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);
    
    //Upon successful completion, a value of zero shall be returned
    return res;
}


int main(int argc,char **argv){
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
    }
    srand(seed); // feed the seed
    
    /* Queue usage example
        Queue *myQ = ConstructQueue(1000);
        Task t;
        t.ID = myID;
        t.type = 2;
        Enqueue(myQ, t);
        Task ret = Dequeue(myQ);
        DestructQueue(myQ);
    */

    // your code goes here
    // you can simulate gift request creation in here, 
    // but make sure to launch the threads first
    
    //Thread Creation for every worker and Queue Creation
    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&lockID, NULL);
    pthread_mutex_init(&lockSecond, NULL);
    pthread_mutex_init(&lockTaskID, NULL);
    
    pthread_mutex_init(&priorQLock, NULL);
    pthread_mutex_init(&priorLockID, NULL);

    paintingQ = ConstructQueue(5000);
    assemblyQ = ConstructQueue(5000);
    QAQ = ConstructQueue(5000);
    deliveryQ = ConstructQueue(5000);
    packageQ = ConstructQueue(1000);

    paintingQPrior = ConstructQueue(5000);
    assemblyQPrior = ConstructQueue(5000);
    QAQPrior = ConstructQueue(5000);
    deliveryQPrior = ConstructQueue(5000);
    packageQPrior = ConstructQueue(1000);



    pthread_t elf_a_thread;
    pthread_t elf_b_thread;
    pthread_t santa_thread;
    pthread_t control_thread;
    
    printf("TaskID  GiftID  GiftType  TaskType  RequestTime  TaskArrival  TT  Responsible\n");

    pthread_create(&elf_a_thread, NULL, ElfA, NULL);
    pthread_create(&elf_b_thread, NULL, ElfB, NULL);
    pthread_create(&santa_thread, NULL, Santa, NULL);
    pthread_create(&control_thread, NULL, ControlThread, NULL);

    pthread_sleep(simulationTime); 

    pthread_cancel(elf_a_thread);
    pthread_cancel(elf_b_thread);
    pthread_cancel(santa_thread);
    pthread_cancel(control_thread);

    //printf("HELOOO ÖMER VERYSEL. I killed it\n");
    
    return 0;
}

void* ElfA(void *arg){
    

    while(1) {
    
    	
    	/*
                        NEW ZELAND TASKS MAKE A HARD CHECK HERE
         
        */
        
        // Check package Queue, priority on it, it is made first
        Task *packageTPrior;
        pthread_mutex_lock(&priorQLock);
        if (isEmpty(packageQPrior)) { // If it is empty release lock
            pthread_mutex_unlock(&priorQLock);
        }
        else {
            packageTPrior = Dequeue(packageQPrior); // Dequeue one element
            pthread_mutex_unlock(&priorQLock);
            
            pthread_sleep(1); // Package time 
            packageTPrior->packageDone = 1;
            
            //Get the second we are in
            pthread_mutex_lock(&lockSecond);
            int curSec = second; 
            pthread_mutex_unlock(&lockSecond);
            
            // Print Log message
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dCN        %-13d%-13d%-4dA\n",taskID++ ,packageTPrior->ID, packageTPrior->type, packageTPrior->startTime, curSec, curSec-packageTPrior->startTime ); 
            pthread_mutex_unlock(&lockTaskID);
           
            // The time the task is enqueued to new task queue
            packageTPrior->startTime = curSec;
            
            //Enqueue the task to new queue
            pthread_mutex_lock(&priorQLock);
            Enqueue(deliveryQPrior, packageTPrior); 
            pthread_mutex_unlock(&priorQLock);
        }
        // Package Done 
        
        // Pritority one packageing if not empty go over it
        pthread_mutex_lock(&priorQLock);
        if (!isEmpty(packageQPrior)) {
            pthread_mutex_unlock(&priorQLock);
            continue;
        }
        pthread_mutex_unlock(&priorQLock);


        // Painting Task
        Task *paintingTPrior;
        pthread_mutex_lock(&priorQLock);
        if (isEmpty(paintingQPrior)) { // If empty release lock
            pthread_mutex_unlock(&priorQLock);
        }
        else {
            paintingTPrior = Dequeue(paintingQPrior); // Dequeue one element
            pthread_mutex_unlock(&priorQLock);
            
            pthread_sleep(3); // Painting time
            pthread_mutex_lock(&priorLockID);
            paintingTPrior->paintingDone = 1;
            pthread_mutex_unlock(&priorLockID);
            
            // Get the current time
            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);
            
            // Print log message
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dPN        %-13d%-13d%-4dA\n", taskID++, paintingTPrior->ID, paintingTPrior->type, paintingTPrior->startTime, curSec, curSec- paintingTPrior->startTime); 
            pthread_mutex_unlock(&lockTaskID);
            
            // The time the task is enqueued to new Queue
            paintingTPrior->startTime = curSec;

            // What if another thread pulled the same task, and modified it?
            // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
            // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
            // if it not then do not insert it into deliveryQ, let the other one do the above
            // make this in a way that there will be no equal case
            
            pthread_mutex_lock(&priorLockID);
            if (paintingTPrior->paintingDone == 1 &&  paintingTPrior->QADone == 1 &&   paintingTPrior->assemblyDone == 1 ) { // If all of them is done then enqueue
                pthread_mutex_lock(&priorQLock);
                Enqueue(packageQPrior, paintingTPrior); 
                pthread_mutex_unlock(&priorQLock);
            }
            else if (paintingTPrior->assemblyDone != 1 || paintingTPrior->QADone != 1) { // If not check if the others did, if not leave job to them
            }
            pthread_mutex_unlock(&priorLockID);
            
        }
        
        pthread_mutex_lock(&priorQLock);
        if (!isEmpty(packageQPrior) || !isEmpty(paintingQPrior) ) {
            pthread_mutex_unlock(&priorQLock); 
            continue;
        }
        pthread_mutex_unlock(&priorQLock);

    	/*
    								NORMAL TASKS
    	*/
        
        // Check package Queue, priority on it, it is made first
        Task *packageT;
        pthread_mutex_lock(&lock);
        if (isEmpty(packageQ)) { // If it is empty release lock
            pthread_mutex_unlock(&lock);
        }
        else {
            packageT = Dequeue(packageQ); // Dequeue one element
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(1); // Package time 
            packageT->packageDone = 1;
            
            //Get the second we are in
            pthread_mutex_lock(&lockSecond);
            int curSec = second; 
            pthread_mutex_unlock(&lockSecond);
            
            // Print Log message
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dC         %-13d%-13d%-4dA\n",taskID++ ,packageT->ID, packageT->type, packageT->startTime, curSec, curSec-packageT->startTime ); 
            pthread_mutex_unlock(&lockTaskID);
           
            // The time the task is enqueued to new task queue
            packageT->startTime = curSec;
            
            //Enqueue the task to new queue
            pthread_mutex_lock(&lock);
            Enqueue(deliveryQ, packageT); 
            pthread_mutex_unlock(&lock);
        }
        // Package Done
        
        // Pritority one packageing if not empty go over it
        pthread_mutex_lock(&lock);
        if (!isEmpty(packageQ)) {
            pthread_mutex_unlock(&lock);
            continue;
        }
        pthread_mutex_unlock(&lock);


        // Painting Task
        Task *paintingT;
        pthread_mutex_lock(&lock);
        if (isEmpty(paintingQ)) { // If empty release lock
            pthread_mutex_unlock(&lock);
        }
        else {
            paintingT = Dequeue(paintingQ); // Dequeue one element
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(3); // Painting time
            pthread_mutex_lock(&lockID);
            paintingT->paintingDone = 1;
            pthread_mutex_unlock(&lockID);
            
            // Get the current time
            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);
            
            // Print log message
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dP         %-13d%-13d%-4dA\n", taskID++, paintingT->ID, paintingT->type, paintingT->startTime, curSec, curSec- paintingT->startTime); 
            pthread_mutex_unlock(&lockTaskID);
            
            // The time the task is enqueued to new Queue
            paintingT->startTime = curSec;

            // What if another thread pulled the same task, and modified it?
            // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
            // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
            // if it not then do not insert it into deliveryQ, let the other one do the above
            // make this in a way that there will be no equal case
           
            pthread_mutex_lock(&lockID);
            if (paintingT->paintingDone == 1 &&  paintingT->QADone == 1 &&   paintingT->assemblyDone == 1 ) { // If all of them is done then enqueue
                pthread_mutex_lock(&lock);
                Enqueue(packageQ, paintingT); 
                pthread_mutex_unlock(&lock);
            }
            else if (paintingT->assemblyDone != 1 || paintingT->QADone != 1) { // If not check if the others did, if not leave job to them
            }
            pthread_mutex_unlock(&lockID);
            
        }

        
	
    }


}

void* ElfB(void *arg){
   

    while(1) {
    
    	 /*
                        NEW ZELAND TASKS MAKE A HARD CHECK HERE
         
        */
        // Check package Queue, priority on it, it is made first
        
        Task *packageTPrior;
        pthread_mutex_lock(&priorQLock);
        if (isEmpty(packageQPrior)) { // If it is empty release lock
            pthread_mutex_unlock(&priorQLock);
        }
        else {
            packageTPrior = Dequeue(packageQPrior); // Dequeue one element
            pthread_mutex_unlock(&priorQLock);
            
            pthread_sleep(1); // Package time 
            packageTPrior->packageDone = 1;
            
            //Get the second we are in
            pthread_mutex_lock(&lockSecond);
            int curSec = second; 
            pthread_mutex_unlock(&lockSecond);
            
            // Print Log message
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dCN        %-13d%-13d%-4dB\n",taskID++ ,packageTPrior->ID, packageTPrior->type, packageTPrior->startTime, curSec, curSec-packageTPrior->startTime ); 
            pthread_mutex_unlock(&lockTaskID);
           
            // The time the task is enqueued to new task queue
            packageTPrior->startTime = curSec;
            
            //Enqueue the task to new queue
            pthread_mutex_lock(&priorQLock);
            Enqueue(deliveryQPrior, packageTPrior); 
            pthread_mutex_unlock(&priorQLock);
        }
        // Package Done 
        
        // Pritority one packageing if not empty go over it
        pthread_mutex_lock(&priorQLock);
        if (!isEmpty(packageQPrior)) {
            pthread_mutex_unlock(&priorQLock);
            continue;
        }
        pthread_mutex_unlock(&priorQLock);


        // Assembly Task
        Task *assemblyTPrior;
        pthread_mutex_lock(&priorQLock);
        if (isEmpty(assemblyQPrior)) { // If empty release lock
            pthread_mutex_unlock(&priorQLock);
        }
        else {
            assemblyTPrior = Dequeue(assemblyQPrior); // Dequeue one element
            pthread_mutex_unlock(&priorQLock);
            
            pthread_sleep(2); // Painting time
            pthread_mutex_lock(&priorLockID);
            assemblyTPrior->assemblyDone = 1;
            pthread_mutex_unlock(&priorLockID);
            
            // Get the current time
            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);
            
            // Print log message
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dAN        %-13d%-13d%-4dB\n", taskID++, assemblyTPrior->ID, assemblyTPrior->type, assemblyTPrior->startTime, curSec, curSec-assemblyTPrior->startTime); 
            pthread_mutex_unlock(&lockTaskID);
            
            // The time the task is enqueued to new Queue
            assemblyTPrior->startTime = curSec;

            // What if another thread pulled the same task, and modified it?
            // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
            // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
            // if it not then do not insert it into deliveryQ, let the other one do the above
            // make this in a way that there will be no equal case
            pthread_mutex_lock(&priorLockID);
            if (assemblyTPrior->paintingDone == 1 &&  assemblyTPrior->QADone == 1 &&   assemblyTPrior->assemblyDone == 1 ) { // If all of them is done then enqueue
                pthread_mutex_lock(&priorQLock);
                Enqueue(packageQPrior, assemblyTPrior); 
                pthread_mutex_unlock(&priorQLock);
            }
            else if (assemblyTPrior->paintingDone != 1 || assemblyTPrior->QADone != 1) { // If not check if the others did, if not leave job to them
            }
            pthread_mutex_unlock(&priorLockID);

            
        }
    	
        pthread_mutex_lock(&priorQLock);
        if (!isEmpty(packageQPrior) || !isEmpty(assemblyQPrior) ) {
            pthread_mutex_unlock(&priorQLock);
            continue;
        }
        pthread_mutex_unlock(&priorQLock); 


        
        /*
        				NORMAL TASK
        */
        
        // Cehck package Queue, priority on it, it is made first
        Task *packageT;
        pthread_mutex_lock(&lock);
        if (isEmpty(packageQ)) {
            pthread_mutex_unlock(&lock);
        }
        else {
            packageT = Dequeue(packageQ);
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(1);
            packageT->packageDone = 1;

            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);

            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dC         %-13d%-13d%-4dB\n", taskID++, packageT->ID, packageT->type, packageT->startTime, curSec, curSec-packageT->startTime ); 
            pthread_mutex_unlock(&lockTaskID);
            
            packageT->startTime = curSec;
              
            pthread_mutex_lock(&lock);
            Enqueue(deliveryQ, packageT); 
            pthread_mutex_unlock(&lock);
        }
        // Package Done
        
        pthread_mutex_lock(&lock);
        if (!isEmpty(packageQ)) {
            pthread_mutex_unlock(&lock);
            continue;
        }
        pthread_mutex_unlock(&lock);


        // Painting Task
        Task *assemblyT;
        pthread_mutex_lock(&lock);
        if (isEmpty(assemblyQ)) {
            pthread_mutex_unlock(&lock);
        }
        else {
            assemblyT = Dequeue(assemblyQ);
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(2);
            pthread_mutex_lock(&lockID); // It was commented
            assemblyT->assemblyDone = 1;
            pthread_mutex_unlock(&lockID); // It was commented

            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);
            
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dA         %-13d%-13d%-4dB\n", taskID++, assemblyT->ID, assemblyT->type, assemblyT->startTime, curSec, curSec-assemblyT->startTime); 
            pthread_mutex_unlock(&lockTaskID);
         
            assemblyT->startTime = curSec;



            // What if another thread pulled the same task, and modified it?
            // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
            // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
            // if it not then do not insert it into deliveryQ, let the other one do the above
            // make this in a way that there will be no equal case
            pthread_mutex_lock(&lockID);
            if (assemblyT->paintingDone == 1 && assemblyT->QADone == 1 && assemblyT->assemblyDone == 1 ) {
                pthread_mutex_lock(&lock);
                Enqueue(packageQ, assemblyT); 
                pthread_mutex_unlock(&lock);
            }
            else if (assemblyT->paintingDone != 1 || assemblyT->QADone != 1) {
            }
            pthread_mutex_unlock(&lockID);

        }

    }

}

// manages Santa's tasks
void* Santa(void *arg){
    
    while(1) {
    
    	
        /*
                        NEW ZELAND TASKS MAKE A HARD CHECK HERE
         
        */
        
        
        // Check package Queue, priority on it, it is made first
        Task *deliveryTPrior;
        pthread_mutex_lock(&priorQLock);
        if (isEmpty(deliveryQPrior)) { // If it is empty release lock
            pthread_mutex_unlock(&priorQLock);
        }
        else {
            deliveryTPrior = Dequeue(deliveryQPrior); // Dequeue one element
            pthread_mutex_unlock(&priorQLock);
            
            pthread_sleep(1); // Delivery time 
            deliveryTPrior->deliveryDone = 1;
            
            //Get the second we are in
            pthread_mutex_lock(&lockSecond);
            int curSec = second; 
            pthread_mutex_unlock(&lockSecond);
            
            // Print Log message
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dDN        %-13d%-13d%-4dS\n",taskID++ ,deliveryTPrior->ID, deliveryTPrior->type, deliveryTPrior->startTime, curSec, curSec-deliveryTPrior->startTime ); 
            free(deliveryTPrior);
	        pthread_mutex_unlock(&lockTaskID);
        }
        // Delivery Done 
        
        // Pritority one packageing if not empty go over it
        pthread_mutex_lock(&priorQLock);
        if (!isEmpty(deliveryQPrior)) {
            pthread_mutex_unlock(&priorQLock);
            continue;
        }
        pthread_mutex_unlock(&priorQLock);


        // QA Task
        Task *QATPrior;
        pthread_mutex_lock(&priorQLock);
        if (isEmpty(QAQPrior)) { // If empty release lock
            pthread_mutex_unlock(&priorQLock);
        }
        else {
            QATPrior = Dequeue(QAQPrior); // Dequeue one element
            pthread_mutex_unlock(&priorQLock);
            
            pthread_sleep(1); //QA time
            pthread_mutex_lock(&priorLockID);
            QATPrior->QADone = 1;
            pthread_mutex_unlock(&priorLockID);
            
            // Get the current time
            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);
            
            // Print log message
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dQN        %-13d%-13d%-4dS\n", taskID++, QATPrior->ID, QATPrior->type, QATPrior->startTime, curSec, curSec-QATPrior->startTime); 
            pthread_mutex_unlock(&lockTaskID);
            
            // The time the task is enqueued to new Queue
            QATPrior->startTime = curSec;

            // What if another thread pulled the same task, and modified it?
            // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
            // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
            // if it not then do not insert it into deliveryQ, let the other one do the above
            // make this in a way that there will be no equal case
            pthread_mutex_lock(&priorLockID);
            if (QATPrior->paintingDone == 1 &&  QATPrior->QADone == 1 && QATPrior->assemblyDone == 1 ) { // If all of them is done then enqueue
                pthread_mutex_lock(&priorQLock);
                Enqueue(packageQPrior, QATPrior); 
                pthread_mutex_unlock(&priorQLock);
            }
            else if (QATPrior->paintingDone != 1 || QATPrior->QADone != 1) { // If not check if the others did, if not leave job to them
            }
            pthread_mutex_unlock(&priorLockID);
            
        }
        
        pthread_mutex_lock(&priorQLock);
        if (!isEmpty(deliveryQPrior) || !isEmpty(QAQPrior) ) {
            pthread_mutex_unlock(&priorQLock); 
            continue;
        }
        pthread_mutex_unlock(&priorQLock);


    
        
        /*
        				NORMAL TASK
        */
        
        
        // Check delivery Queue, priority on it, it is made first
        Task *deliveryT;
        pthread_mutex_lock(&lock);
        if (isEmpty(deliveryQ)) {
            pthread_mutex_unlock(&lock);
        }
        else {
            deliveryT = Dequeue(deliveryQ);
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(1);
            deliveryT->deliveryDone = 1;
            
            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);

            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dD         %-13d%-13d%-4dS\n", taskID++, deliveryT->ID, deliveryT->type, deliveryT->startTime, curSec, curSec-deliveryT->startTime); 
            free(deliveryT);
	    pthread_mutex_unlock(&lockTaskID);
        }
        // Delivery Done
        

        // To dequeue Delvivery till its empty
        pthread_mutex_lock(&lock);
        if (!isEmpty(deliveryQ) && QAQ->size < 3) {
            pthread_mutex_unlock(&lock);
            continue;
        }
        pthread_mutex_unlock(&lock);


        //Handle QA Queue
        Task *QAT;
        pthread_mutex_lock(&lock);
        if (isEmpty(QAQ)) {
            pthread_mutex_unlock(&lock);
        }
        else {
            pthread_mutex_unlock(&lock);
            
            while(QAQ->size >= 3) {

                pthread_mutex_lock(&lock);
                QAT = Dequeue(QAQ);
                pthread_mutex_unlock(&lock);
                
                pthread_sleep(1);
                pthread_mutex_lock(&lockID);
                QAT->QADone = 1;
                pthread_mutex_unlock(&lockID);
          

                pthread_mutex_lock(&lockSecond);
                int curSec = second;
                pthread_mutex_unlock(&lockSecond);

                pthread_mutex_lock(&lockTaskID);
                printf("%-8d%-8d%-10dQ         %-13d%-13d%-4dS\n", taskID++, QAT->ID, QAT->type, QAT->startTime, curSec, curSec-QAT->startTime); 
                pthread_mutex_unlock(&lockTaskID);
          
                QAT->startTime = curSec;



                // What if another thread pulled the same task, and modified it?
                // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
                // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
                // if it not then do not insert it into deliveryQ, let the other one do the above
                // make this in a way that there will be no equal case
                pthread_mutex_lock(&lockID);
                if (QAT->paintingDone == 1 && QAT->QADone == 1 && QAT->assemblyDone == 1 ) {
                    pthread_mutex_lock(&lock);
                    Enqueue(packageQ, QAT); 
                    pthread_mutex_unlock(&lock);
                }
                else if (QAT->paintingDone != 1 || QAT->assemblyDone != 1) {
                
                }
                pthread_mutex_unlock(&lockID);
            }
        }
 

       
            

    } 


}

// the function that controls queues and output
void* ControlThread(void *arg){
    int id = 0;
    while (1) {
        pthread_sleep(1); // Every second 
        int randomGift = rand()% 100; // Choose a random 0 <= randomGift <= 99
       
        pthread_mutex_lock(&lockSecond);
        second++;
        pthread_mutex_unlock(&lockSecond);

        // Create Task

        Task *task = malloc(sizeof(*task));
        task->ID = id;
        task->startTime = second;        
        id++;

        // Question is do we need seprate locks for each task Queue?

        if (0 <= randomGift && randomGift <= 39 ) { // only chocolate
            // only package and delivery
            task->type = 1;
            task->packageDone = 0;
            task->assemblyDone = 1;
            task->paintingDone = 1;
            task->deliveryDone = 0;
            task->QADone = 1;
           
            if (second % 30 == 29) {
                pthread_mutex_lock(&priorQLock);
                Enqueue(packageQPrior, task);
                pthread_mutex_unlock(&priorQLock);
                continue;
            }

            pthread_mutex_lock(&lock);
            Enqueue(packageQ, task);
            pthread_mutex_unlock(&lock);
        }
        else if (40 <=randomGift && randomGift <= 59) { // wooden toy and chocolate
            // painting, package and delivery
            task->type = 2;
            task->packageDone = 0;
            task->assemblyDone = 1;
            task->paintingDone = 0;
            task->deliveryDone = 0;
            task->QADone = 1;
           
            if (second % 30 == 29) {
                pthread_mutex_lock(&priorQLock);
                Enqueue(paintingQPrior, task);
                pthread_mutex_unlock(&priorQLock);
                continue;
            }

            pthread_mutex_lock(&lock);
            Enqueue(paintingQ, task);
            pthread_mutex_unlock(&lock);
        }
        else if (60 <=randomGift && randomGift <= 79) { // plastic toy and chocolate
            // assemble, package and delivery
            task->type = 3;
            task->packageDone = 0;
            task->assemblyDone = 0;
            task->paintingDone = 1;
            task->deliveryDone = 0;
            task->QADone = 1;

            if (second % 30 == 29) {
                pthread_mutex_lock(&priorQLock);
                Enqueue(assemblyQPrior, task);
                pthread_mutex_unlock(&priorQLock);
                continue;
            }

            pthread_mutex_lock(&lock);
            Enqueue(assemblyQ, task);
            pthread_mutex_unlock(&lock);
        }
        else if (80 <=randomGift && randomGift <= 84) { // GS5, wooden toy and chocolate
            // painting, QA, package and delivery
            task->type = 4;
            task->packageDone = 0;
            task->assemblyDone = 1;
            task->paintingDone = 0;
            task->deliveryDone = 0;
            task->QADone = 0;
   
            if (second % 30 == 29) {
                pthread_mutex_lock(&priorQLock);
                Enqueue(paintingQPrior, task);
                Enqueue(QAQPrior, task);
                pthread_mutex_unlock(&priorQLock);
                continue;
            }
            
            pthread_mutex_lock(&lock);
            Enqueue(paintingQ, task);
            Enqueue(QAQ, task);
            pthread_mutex_unlock(&lock);
        }
        else if (85 <=randomGift  && randomGift <= 89) { // GS5, plastic toy and chocolate 
            // assemble, QA, package and delivery
            task->type = 5;
            task->packageDone = 0;
            task->assemblyDone = 0;
            task->paintingDone = 1;
            task->deliveryDone = 0;
            task->QADone = 0;
       
            if (second % 30 == 29) {
                pthread_mutex_lock(&priorQLock);
                Enqueue(assemblyQPrior, task);
                Enqueue(QAQPrior, task);
                pthread_mutex_unlock(&priorQLock);
                continue;
            }

            pthread_mutex_lock(&lock);
            Enqueue(assemblyQ, task);
            Enqueue(QAQ, task);
            pthread_mutex_unlock(&lock);
            //printf("Package and Painting and QA need %d  \n ", task.ID);
        }
        else { // No present PART A

        }
        
                   
        
    }


}


void printTask(Task *t) {
    printf("*****************************\n");
    printf("Task id = %d\n", t->ID);
    if (t->type == 1) {
       printf("chocolate, package, delivery\n"); 
    }
    else if (t->type == 2){
       printf("chocolate, painting, package, delivery\n"); 
    }
    else if (t->type == 3){
       printf("chocolate, assembly, package, delivery\n"); 
    }
    else if (t->type == 4){
       printf("chocolate, painting, QA, package, delivery\n"); 
    }
    else {
       printf("chocolate, assembly, QA, package, delivery\n"); 
    }
    printf("Assembly Done = %d\n", t->assemblyDone);
    printf("Painting Done = %d\n", t->paintingDone);
    printf("Package Done = %d\n", t->packageDone);
    printf("QA Done = %d\n", t->QADone);
    printf("Delivery Done = %d\n", t->deliveryDone);
    printf("*****************************\n");
}






