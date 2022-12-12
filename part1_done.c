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

// Semaphore, locks, Queues

pthread_mutex_t lock;
pthread_mutex_t lockID;
pthread_mutex_t lockPaintingID, lockAssemblyID, lockQAID;

// Question is do we need seprate locks for each task Queue?

Queue *paintingQ; 
Queue *assemblyQ;
Queue *packageQ;
Queue *QAQ;
Queue *deliveryQ;
   
int lastFinishedPaintingID, lastFinishedAssemblyID, lastFinishedQAID = 0;

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
    pthread_mutex_init(&lockPaintingID, NULL);
    pthread_mutex_init(&lockAssemblyID, NULL);
    pthread_mutex_init(&lockQAID, NULL);
    
    paintingQ = ConstructQueue(1000);
    assemblyQ = ConstructQueue(1000);
    QAQ = ConstructQueue(1000);
    deliveryQ = ConstructQueue(1000);
    packageQ = ConstructQueue(1000);

    pthread_t elf_a_thread;
    pthread_t elf_b_thread;
    pthread_t santa_thread;
    pthread_t control_thread;
    /*
    pthread_create(&elf_a_thread, NULL, ElfA, (void *) myQ);
    pthread_create(&elf_b_thread, NULL, ElfB, (void *) myQ);
    pthread_create(&santa_thread, NULL, Santa, (void *) myQ);
    pthread_create(&control_thread, NULL, ControlThread, (void *) myQ);
    */ 
    pthread_create(&elf_a_thread, NULL, ElfA, NULL);
    pthread_create(&elf_b_thread, NULL, ElfB, NULL);
    pthread_create(&santa_thread, NULL, Santa, NULL);
    pthread_create(&control_thread, NULL, ControlThread, NULL);
   
   

    
    // Wait Each thread to finish it's job
    pthread_join(elf_a_thread, NULL);
    pthread_join(elf_b_thread, NULL);
    pthread_join(santa_thread, NULL);
    pthread_join(control_thread, NULL);


    return 0;
}

void* ElfA(void *arg){
    
    while(1) {
        
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
            printf("elf A Packaging done id = %d\n", packageT->ID); 
            pthread_mutex_lock(&lock);
            Enqueue(deliveryQ, packageT); 
            pthread_mutex_unlock(&lock);
        }
        // Package Done
       

        // Painting Task
        Task *paintingT;
        pthread_mutex_lock(&lock);
        if (isEmpty(paintingQ)) {
            pthread_mutex_unlock(&lock);
        }
        else {
            paintingT = Dequeue(paintingQ);
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(3);
            paintingT->paintingDone = 1;
            printf("Painting done id = %d\n", paintingT->ID); 
            
            /*
            pthread_mutex_lock(&lockPaintingID);
            lastFinishedPaintingID = paintingT.ID;
            pthread_mutex_unlock(&lockPaintingID);
            */
            pthread_mutex_lock(&lockID);
            lastFinishedPaintingID = paintingT->ID;
            pthread_mutex_unlock(&lockID);

            // What if another thread pulled the same task, and modified it?
            // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
            // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
            // if it not then do not insert it into deliveryQ, let the other one do the above
            // make this in a way that there will be no equal case
            
            if (paintingT->paintingDone == 1 &&  paintingT->QADone == 1 &&   paintingT->assemblyDone == 1 ) {
                pthread_mutex_lock(&lock);
                Enqueue(packageQ, paintingT); 
                pthread_mutex_unlock(&lock);
            }
            else if (paintingT->assemblyDone != 1 || paintingT->QADone != 1) {
                /*
                pthread_mutex_lock(&lockQAID);
                pthread_mutex_lock(&lockQAID);
                pthread_mutex_lock(&lockQAID);
                if (lastFinishedAssemblyID >= lastFinishedPaintingID &&  lastFinishedQAID >= lastFinishedPaintingID) {
                    paintingT.assemblyDone = 1; paintingT.QADone = 1;
                    pthread_mutex_lock(&lock);
                    Enqueue(packageQ, paintingT); 
                    pthread_mutex_unlock(&lock);
                }
                pthread_mutex_unlock(&lockQAID);
                pthread_mutex_unlock(&lockQAID);
                pthread_mutex_unlock(&lockQAID);
                */
                pthread_mutex_lock(&lockID);
                if (lastFinishedAssemblyID >= lastFinishedPaintingID &&  lastFinishedQAID >= lastFinishedPaintingID) {
                    paintingT->assemblyDone = 1; paintingT->QADone = 1;
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
            printf("elf B Packaging done id = %d\n", packageT->ID); 
            pthread_mutex_lock(&lock);
            Enqueue(deliveryQ, packageT); 
            pthread_mutex_unlock(&lock);
        }
        // Package Done
       

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
            assemblyT->assemblyDone = 1;
            printf("Assembly done id = %d\n", assemblyT->ID); 
          
            /*
            pthread_mutex_lock(&lockAssemblyID);
            lastFinishedAssemblyID = assemblyT.ID;
            pthread_mutex_unlock(&lockAssemblyID);
            */
            pthread_mutex_lock(&lockID);
            lastFinishedAssemblyID = assemblyT->ID;
            pthread_mutex_unlock(&lockID);


            // What if another thread pulled the same task, and modified it?
            // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
            // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
            // if it not then do not insert it into deliveryQ, let the other one do the above
            // make this in a way that there will be no equal case

            if (assemblyT->paintingDone == 1 && assemblyT->QADone == 1 && assemblyT->assemblyDone == 1 ) {
                pthread_mutex_lock(&lock);
                Enqueue(packageQ, assemblyT); 
                pthread_mutex_unlock(&lock);
            }
            else if (assemblyT->paintingDone != 1 || assemblyT->QADone != 1) {
                /*
                pthread_mutex_lock(&lockQAID);
                pthread_mutex_lock(&lockQAID);
                pthread_mutex_lock(&lockQAID);
                if (lastFinishedPaintingID >= lastFinishedAssemblyID &&  lastFinishedQAID >= lastFinishedAssemblyID) {
                    assemblyT.paintingDone = 1; assemblyT.QADone = 1;
                    pthread_mutex_lock(&lock);
                    Enqueue(packageQ, assemblyT); 
                    pthread_mutex_unlock(&lock);
                }
                pthread_mutex_unlock(&lockQAID);
                pthread_mutex_unlock(&lockQAID);
                pthread_mutex_unlock(&lockQAID);
                */
                pthread_mutex_lock(&lockID);
                if (lastFinishedPaintingID >= lastFinishedAssemblyID &&  lastFinishedQAID >= lastFinishedAssemblyID) {
                    assemblyT->paintingDone = 1; assemblyT->QADone = 1;
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
            printf("Delivery done id = %d\n", deliveryT->ID); 
            free(deliveryT);
	}
        // Delivery Done

        Task *QAT;
        pthread_mutex_lock(&lock);
        if (isEmpty(QAQ)) {
            pthread_mutex_unlock(&lock);
        }
        else {
            QAT = Dequeue(QAQ);
            pthread_mutex_unlock(&lock);
            
            pthread_sleep(1);
            QAT->QADone = 1;
            printf("QA done id = %d\n", QAT->ID); 
           
            /*
            pthread_mutex_lock(&lockQAID);
            lastFinishedQAID = QAT.ID;
            pthread_mutex_unlock(&lockQAID);
            */
            pthread_mutex_lock(&lockID);
            lastFinishedQAID = QAT->ID;
            pthread_mutex_unlock(&lockID);


            // What if another thread pulled the same task, and modified it?
            // Thesoultion may be to save the id of last task they modified, elfBLastPaintingID
            // if it exceded elfA, then it modified it, you can make the relevant Done flag 1 and insert it 
            // if it not then do not insert it into deliveryQ, let the other one do the above
            // make this in a way that there will be no equal case

            if (QAT->paintingDone == 1 && QAT->QADone == 1 && QAT->assemblyDone == 1 ) {
                pthread_mutex_lock(&lock);
                Enqueue(packageQ, QAT); 
                pthread_mutex_unlock(&lock);
            }
            else if (QAT->paintingDone != 1 || QAT->assemblyDone != 1) {
                /*
                pthread_mutex_lock(&lockQAID);
                pthread_mutex_lock(&lockQAID);
                pthread_mutex_lock(&lockQAID);
                if (lastFinishedAssemblyID >= lastFinishedQAID &&  lastFinishedPaintingID >= lastFinishedQAID) {
                    QAT.assemblyDone = 1; QAT.paintingDone = 1;
                    pthread_mutex_lock(&lock);
                    Enqueue(packageQ, QAT); 
                    pthread_mutex_unlock(&lock);
                }
                pthread_mutex_unlock(&lockQAID);
                pthread_mutex_unlock(&lockQAID);
                pthread_mutex_unlock(&lockQAID);
                */
                pthread_mutex_lock(&lockID);
                if (lastFinishedAssemblyID >= lastFinishedQAID &&  lastFinishedPaintingID >= lastFinishedQAID) {
                    QAT->assemblyDone = 1; QAT->paintingDone = 1;
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

// the function that controls queues and output
void* ControlThread(void *arg){
    int id = 0;
    while (1) {
        pthread_sleep(1); // Every second 
        int randomGift = rand()% 100; // Choose a random 0 <= randomGift <= 99
        
        // Create Task

        Task *task= malloc(sizeof(*task));
        task->ID = id;
        

        // Question is do we need seprate locks for each task Queue?

        if (0 <= randomGift && randomGift <= 39 ) { // only chocolate
            // only package and delivery
            task->type = 1;
            task->packageDone = 0;
            task->assemblyDone = 1;
            task->paintingDone = 1;
            task->deliveryDone = 0;
            task->QADone = 1;
            
            pthread_mutex_lock(&lock);
            Enqueue(packageQ, task);
            pthread_mutex_unlock(&lock);
            //printf("Only package need %d   \n", task.ID);
        }
        else if (40 <=randomGift && randomGift <= 59) { // wooden toy and chocolate
            // painting, package and delivery
            task->type = 2;
            task->packageDone = 0;
            task->assemblyDone = 1;
            task->paintingDone = 0;
            task->deliveryDone = 0;
            task->QADone = 1;
            
            pthread_mutex_lock(&lock);
            Enqueue(paintingQ, task);
            pthread_mutex_unlock(&lock);
            //printf("Package and Paint need %d   \n", task.ID);
        }
        else if (60 <=randomGift && randomGift <= 79) { // plastic toy and chocolate
            // assemble, package and delivery
            task->type = 3;
            task->packageDone = 0;
            task->assemblyDone = 0;
            task->paintingDone = 1;
            task->deliveryDone = 0;
            task->QADone = 1;
            
            pthread_mutex_lock(&lock);
            Enqueue(assemblyQ, task);
            pthread_mutex_unlock(&lock);
            //printf("Package and Assembly need %d   \n", task.ID);
        }
        else if (80 <=randomGift && randomGift <= 84) { // GS5, wooden toy and chocolate
            // painting, QA, package and delivery
            task->type = 4;
            task->packageDone = 0;
            task->assemblyDone = 1;
            task->paintingDone = 0;
            task->deliveryDone = 0;
            task->QADone = 0;
        
            pthread_mutex_lock(&lock);
            Enqueue(paintingQ, task);
            Enqueue(QAQ, task);
            pthread_mutex_unlock(&lock);
            //printf("Package and Assembly and QA need %d   \n", task.ID);
        }
        else if (85 <=randomGift  && randomGift <= 89) { // GS5, plastic toy and chocolate 
            // assemble, QA, package and delivery
            task->type = 5;
            task->packageDone = 0;
            task->assemblyDone = 0;
            task->paintingDone = 1;
            task->deliveryDone = 0;
            task->QADone = 0;
        
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






