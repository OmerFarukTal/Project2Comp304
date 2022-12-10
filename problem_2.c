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
void printTask(Task t);

// Semaphore, locks, Queues

pthread_mutex_t lock;
pthread_mutex_t lockID;
pthread_mutex_t lockSecond;
pthread_mutex_t lockTaskID;

// Question is do we need seprate locks for each task Queue?

Queue *paintingQ; 
Queue *assemblyQ;
Queue *packageQ;
Queue *QAQ;
Queue *deliveryQ;
   
int lastFinishedPaintingID, lastFinishedAssemblyID, lastFinishedQAID = 0;
int second = 0;
int taskID = 1;
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
        
    paintingQ = ConstructQueue(5000);
    assemblyQ = ConstructQueue(5000);
    QAQ = ConstructQueue(5000);
    deliveryQ = ConstructQueue(5000);
    packageQ = ConstructQueue(1000);

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

    printf("HELOOO ÖMER VERYSEL. I killed it\n");
    
    return 0;
}

void* ElfA(void *arg){
    

    while(1) {
        
        // Check package Queue, priority on it, it is made first
        Task packageT;
        pthread_mutex_lock(&lock);
        if (isEmpty(packageQ)) { // If it is empty release lock
            pthread_mutex_unlock(&lock);
        }
        else {
            packageT = Dequeue(packageQ); // Dequeue one element
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(1); // Package time 
            packageT.packageDone = 1;
            
            //Get the second we are in
            pthread_mutex_lock(&lockSecond);
            int curSec = second; 
            pthread_mutex_unlock(&lockSecond);
            
            // Print Log message
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dC         %-13d%-13d%-2d  A\n",taskID++ ,packageT.ID, packageT.type, packageT.startTime, curSec, curSec-packageT.startTime ); 
            pthread_mutex_unlock(&lockTaskID);
           
            // The time the task is enqueued to new task queue
            packageT.startTime = curSec;
            
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
        Task paintingT;
        pthread_mutex_lock(&lock);
        if (isEmpty(paintingQ)) { // If empty release lock
            pthread_mutex_unlock(&lock);
        }
        else {
            paintingT = Dequeue(paintingQ); // Dequeue one element
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(3); // Painting time
            paintingT.paintingDone = 1;
            
            // Get the current time
            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);
            
            // Print log message
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dP         %-13d%-13d%-2d  A\n", taskID++, paintingT.ID, paintingT.type, paintingT.startTime, curSec, curSec- paintingT.startTime); 
            pthread_mutex_unlock(&lockTaskID);
            
            // The time the task is enqueued to new Queue
            paintingT.startTime = curSec;

            pthread_mutex_lock(&lockID);
            lastFinishedPaintingID = paintingT.ID;
            pthread_mutex_unlock(&lockID);

            // What if another thread pulled the same task, and modified it?
            // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
            // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
            // if it not then do not insert it into deliveryQ, let the other one do the above
            // make this in a way that there will be no equal case
            
            if (paintingT.paintingDone == 1 &&  paintingT.QADone == 1 &&   paintingT.assemblyDone == 1 ) { // If all of them is done then enqueue
                pthread_mutex_lock(&lock);
                Enqueue(packageQ, paintingT); 
                pthread_mutex_unlock(&lock);
            }
            else if (paintingT.assemblyDone != 1 || paintingT.QADone != 1) { // If not check if the others did, if not leave job to them
                pthread_mutex_lock(&lockID);
                if (lastFinishedAssemblyID >= lastFinishedPaintingID &&  lastFinishedQAID >= lastFinishedPaintingID) {
                    paintingT.assemblyDone = 1; paintingT.QADone = 1;
                    pthread_mutex_lock(&lock);
                    Enqueue(packageQ, paintingT); 
                    pthread_mutex_unlock(&lock);
                }
                pthread_mutex_unlock(&lockID);


            }
            // What if the QA is not done, since it either requires painting or assembly it is not a big deal for assembly/painting pair
            // The issue with QA/painting must be resolved (Task type 4-5)
            
        }

    }


}

void* ElfB(void *arg){
   

    while(1) {
        
        // Cehck package Queue, priority on it, it is made first
        Task packageT;
        pthread_mutex_lock(&lock);
        if (isEmpty(packageQ)) {
            pthread_mutex_unlock(&lock);
        }
        else {
            packageT = Dequeue(packageQ);
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(1);
            packageT.packageDone = 1;

            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);

            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dC         %-13d%-13d%-2d  B\n", taskID++, packageT.ID, packageT.type, packageT.startTime, curSec, curSec-packageT.startTime ); 
            pthread_mutex_unlock(&lockTaskID);
            
            packageT.startTime = curSec;
              
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
        Task assemblyT;
        pthread_mutex_lock(&lock);
        if (isEmpty(assemblyQ)) {
            pthread_mutex_unlock(&lock);
        }
        else {
            assemblyT = Dequeue(assemblyQ);
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(2);
            assemblyT.assemblyDone = 1;

            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);
            
            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dA         %-13d%-13d%-2d  B\n", taskID++, assemblyT.ID, assemblyT.type, assemblyT.startTime, curSec, curSec-assemblyT.startTime); 
            pthread_mutex_unlock(&lockTaskID);
         
            assemblyT.startTime = curSec;

            //pthread_mutex_lock(&lockID);
            lastFinishedAssemblyID = assemblyT.ID;
            //pthread_mutex_unlock(&lockID);


            // What if another thread pulled the same task, and modified it?
            // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
            // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
            // if it not then do not insert it into deliveryQ, let the other one do the above
            // make this in a way that there will be no equal case

            if (assemblyT.paintingDone == 1 && assemblyT.QADone == 1 && assemblyT.assemblyDone == 1 ) {
                pthread_mutex_lock(&lock);
                Enqueue(packageQ, assemblyT); 
                pthread_mutex_unlock(&lock);
            }
            else if (assemblyT.paintingDone != 1 || assemblyT.QADone != 1) {
                pthread_mutex_lock(&lockID);
                if (lastFinishedPaintingID >= lastFinishedAssemblyID &&  lastFinishedQAID >= lastFinishedAssemblyID) {
                    assemblyT.paintingDone = 1; assemblyT.QADone = 1;
                    pthread_mutex_lock(&lock);
                    Enqueue(packageQ, assemblyT); 
                    pthread_mutex_unlock(&lock);
                }
                pthread_mutex_unlock(&lockID);
            }

            // What if the QA is not done, since it either requires painting or assembly it is not a big deal for assembly/painting pair
            // The issue with QA/asembly must be resolved (Task type 4-5)

        }

    }

}

// manages Santa's tasks
void* Santa(void *arg){
    
    while(1) {
        
        // Check delivery Queue, priority on it, it is made first
        Task deliveryT;
        pthread_mutex_lock(&lock);
        if (isEmpty(deliveryQ)) {
            pthread_mutex_unlock(&lock);
        }
        else {
            deliveryT = Dequeue(deliveryQ);
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(1);
            deliveryT.deliveryDone = 1;
            
            pthread_mutex_lock(&lockSecond);
            int curSec = second;
            pthread_mutex_unlock(&lockSecond);

            pthread_mutex_lock(&lockTaskID);
            printf("%-8d%-8d%-10dD         %-13d%-13d%-2d  S\n", taskID++, deliveryT.ID, deliveryT.type, deliveryT.startTime, curSec, curSec-deliveryT.startTime); 
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
        Task QAT;
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
                QAT.QADone = 1;
          

                pthread_mutex_lock(&lockSecond);
                int curSec = second;
                pthread_mutex_unlock(&lockSecond);

                pthread_mutex_lock(&lockTaskID);
                printf("%-8d%-8d%-10dQ         %-13d%-13d%-2d  S\n", taskID++, QAT.ID, QAT.type, QAT.startTime, curSec, curSec-QAT.startTime); 
                pthread_mutex_unlock(&lockTaskID);
          
                QAT.startTime = curSec;

                pthread_mutex_lock(&lockID);
                lastFinishedQAID = QAT.ID;
                pthread_mutex_unlock(&lockID);


                // What if another thread pulled the same task, and modified it?
                // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
                // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
                // if it not then do not insert it into deliveryQ, let the other one do the above
                // make this in a way that there will be no equal case

                if (QAT.paintingDone == 1 && QAT.QADone == 1 && QAT.assemblyDone == 1 ) {
                    pthread_mutex_lock(&lock);
                    Enqueue(packageQ, QAT); 
                    pthread_mutex_unlock(&lock);
                }
                else if (QAT.paintingDone != 1 || QAT.assemblyDone != 1) {
                    pthread_mutex_lock(&lockID);
                    if (lastFinishedAssemblyID >= lastFinishedQAID &&  lastFinishedPaintingID >= lastFinishedQAID) {
                        QAT.assemblyDone = 1; QAT.paintingDone = 1;
                        pthread_mutex_lock(&lock);
                        Enqueue(packageQ, QAT); 
                        pthread_mutex_unlock(&lock);
                    }
                    pthread_mutex_unlock(&lockID);

                }
                // What if the QA is not done, since it either requires painting or assembly it is not a big deal for assembly/painting pair
                // The issue with QA/asembly must be resolved (Task type 4-5)
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

        Task task;
        task.ID = id;
        task.startTime = id;        

        // Question is do we need seprate locks for each task Queue?

        if (0 <= randomGift && randomGift <= 39 ) { // only chocolate
            // only package and delivery
            task.type = 1;
            task.packageDone = 0;
            task.assemblyDone = 1;
            task.paintingDone = 1;
            task.deliveryDone = 0;
            task.QADone = 1;
            
            pthread_mutex_lock(&lock);
            Enqueue(packageQ, task);
            pthread_mutex_unlock(&lock);
            //printf("Only package need %d   \n", task.ID);
        }
        else if (40 <=randomGift && randomGift <= 59) { // wooden toy and chocolate
            // painting, package and delivery
            task.type = 2;
            task.packageDone = 0;
            task.assemblyDone = 1;
            task.paintingDone = 0;
            task.deliveryDone = 0;
            task.QADone = 1;
            
            pthread_mutex_lock(&lock);
            Enqueue(paintingQ, task);
            pthread_mutex_unlock(&lock);
            //printf("Package and Paint need %d   \n", task.ID);
        }
        else if (60 <=randomGift && randomGift <= 79) { // plastic toy and chocolate
            // assemble, package and delivery
            task.type = 3;
            task.packageDone = 0;
            task.assemblyDone = 0;
            task.paintingDone = 1;
            task.deliveryDone = 0;
            task.QADone = 1;
            
            pthread_mutex_lock(&lock);
            Enqueue(assemblyQ, task);
            pthread_mutex_unlock(&lock);
            //printf("Package and Assembly need %d   \n", task.ID);
        }
        else if (80 <=randomGift && randomGift <= 84) { // GS5, wooden toy and chocolate
            // painting, QA, package and delivery
            task.type = 4;
            task.packageDone = 0;
            task.assemblyDone = 1;
            task.paintingDone = 0;
            task.deliveryDone = 0;
            task.QADone = 0;
        
            pthread_mutex_lock(&lock);
            Enqueue(paintingQ, task);
            Enqueue(QAQ, task);
            pthread_mutex_unlock(&lock);
            //printf("Package and Assembly and QA need %d   \n", task.ID);
        }
        else if (85 <=randomGift  && randomGift <= 89) { // GS5, plastic toy and chocolate 
            // assemble, QA, package and delivery
            task.type = 5;
            task.packageDone = 0;
            task.assemblyDone = 0;
            task.paintingDone = 1;
            task.deliveryDone = 0;
            task.QADone = 0;
        
            pthread_mutex_lock(&lock);
            Enqueue(assemblyQ, task);
            Enqueue(QAQ, task);
            pthread_mutex_unlock(&lock);
            //printf("Package and Painting and QA need %d  \n ", task.ID);
        }
        else { // No present PART A

        }
        

        id++;
        //printf("CREATED:\n");
        //printTask(task);
        //printf("Task added id=%d type=%d\n", task.ID, task.type);
        
    }


}







void printTask(Task t) {
    printf("*****************************\n");
    printf("Task id = %d\n", t.ID);
    if (t.type == 1) {
       printf("chocolate, package, delivery\n"); 
    }
    else if (t.type == 2){
       printf("chocolate, painting, package, delivery\n"); 
    }
    else if (t.type == 3){
       printf("chocolate, assembly, package, delivery\n"); 
    }
    else if (t.type == 4){
       printf("chocolate, painting, QA, package, delivery\n"); 
    }
    else {
       printf("chocolate, assembly, QA, package, delivery\n"); 
    }
    printf("Assembly Done = %d\n", t.assemblyDone);
    printf("Painting Done = %d\n", t.paintingDone);
    printf("Package Done = %d\n", t.packageDone);
    printf("QA Done = %d\n", t.QADone);
    printf("Delivery Done = %d\n", t.deliveryDone);
    printf("*****************************\n");
}






