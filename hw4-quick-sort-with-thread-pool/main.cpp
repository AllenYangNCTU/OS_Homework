#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <queue>
using namespace std;

//#define DEBUG


void* taskWork( int arg );

// ========== CLASSES ==========

// ----- Task -----

class Task {
  public:
    //Task(void (*func)(void *), void *arg);
    //Task(void* (*func)(void *), int arg);
    Task(void* (*func)(int), int arg);
    void run();
    //~Task()
  private:
    //void* (*function)(void *);
    void* (*function)(int);
    int argument;
    //void *argument;
};

//Task::Task(void* (*func)(void *), int arg) {
Task::Task(void* (*func)(int), int arg) {
  this->function = func;
  this->argument = arg;
}

void Task::run() {
//  (*(this->function))((void*)&(this->argument));
  taskWork(argument);
}

//Task::~Task() {
//  delete arg;
//}


// ----- ThreadPool -----

class ThreadPool {
  public:
//    ThreadPool(int numOfThreads);
    ThreadPool(int numOfThreads, int numActiveThreads);
    void pushTask(Task task);
    void addOneActiveThread(); // only for this lab
    ~ThreadPool();
  private:
    int numOfThreads;
//    int capacityLimit; // from 1 to 8, only for this lab
    int numActiveThreads; // from 1 to 8, only for this lab
//    sem_t threadCapacity; // initial value is capacityLimit
    static sem_t mutex; // protect threadPool, initial value is 1
    static sem_t taskAvailable; // num of available tasks, initial value is 0
    static queue<Task> tasks; // task queue
    pthread_t *threads; // all 8 threads of threadPool
    static void *threadWork(void *arg); // all threads will run this function
};

//ThreadPool::ThreadPool(int numOfThreads) 
//  : ThreadPool(numOfThreads, numOfThreads)
//{ }

ThreadPool::ThreadPool(int numOfThreads, int numActiveThreads)
  : numOfThreads(numOfThreads),
    numActiveThreads(numActiveThreads)
{
  sem_init(&(ThreadPool::mutex), 0, 1);
  sem_init(&(ThreadPool::taskAvailable), 0, 0);
  this->threads = new pthread_t[numOfThreads];
  for(int i = 0; i < this->numActiveThreads; ++i) {
    //pthread_create((this->threads)+i, NULL, this->threadWork, (void*)(new int(i)));
    pthread_create(&(this->threads)[i], NULL, ThreadPool::threadWork, (void*)(new int(i+1)));
  }
}

void ThreadPool::pushTask(Task task) {
  (ThreadPool::tasks).push(task);
  sem_post(&(ThreadPool::taskAvailable));
}

// only for this lab
// before call this function should ensure all tasks has done
void ThreadPool::addOneActiveThread() {
  pthread_create(&(this->threads)[this->numActiveThreads], NULL, ThreadPool::threadWork, (void*)(new int(this->numActiveThreads+1)));
  ++(this->numActiveThreads);
}

ThreadPool::~ThreadPool() {
  for(int i = 0; i < this->numActiveThreads; ++i) {
    pthread_cancel((this->threads)[i]);
  }
  delete [] threads;
  sem_destroy(&(ThreadPool::mutex));
  sem_destroy(&(ThreadPool::taskAvailable));
}  

void *ThreadPool::threadWork(void *arg) {
  int index = *((int*)arg);
  delete (int*)arg;
  while(true) {
    sem_wait(&(ThreadPool::taskAvailable));
    #ifdef DEBUG
    printf("Thread %d get job\n", index);
    #endif
    sem_wait(&(ThreadPool::mutex));
    Task task(ThreadPool::tasks.front());
    ThreadPool::tasks.pop();
    sem_post(&(ThreadPool::mutex));
    task.run();
  }
}

sem_t ThreadPool::mutex;
sem_t ThreadPool::taskAvailable; 
queue<Task> ThreadPool::tasks;

// ========== GLOBAL VARIABLES ==========

int numCnt; // num of array size
int* inputArr; // store all nums from input.txt, will not be changed during process
int* processingArr; // all nums from inputArr, will be changed and sorted by process

int *left[15];  // array left bound for T1-T15
int *right[15]; // array right bound for T1-T15

int *mostLeft; // most left element of processingArr

ThreadPool threadpool(8, 1);

sem_t sortDone; // initial value is 0
int numTaskDone(0); // 0~15, number of done tasks, every task done will increase this value by 1, when 15 means sort done, initial value is 0

// ========== FUNCTIONS ==========

// return middle
// called by T1 to T7
int *partition( int* lp, int* rp ) {
  int *initialLp = lp;
  if ( lp >= rp ) return lp;
  int pivot = *lp; // Choose the most left element to be the pivot
  while ( lp < rp ) {
    while ( *lp <= pivot && lp < rp ) {
      ++lp;
    }
    while ( *rp > pivot && rp > lp ) {
      --rp;
    }
    int tmp = *lp;
    *lp = *rp;
    *rp = tmp;
  }
  if(rp != lp) {
    printf("rp != lp\n");
  }
  if(*lp > pivot && lp > initialLp) {
    return lp-1;
  } else {
    return lp;
  }
}

// called by T8 to T15
void bubbleSort( int * lp, int *rp ) {
  if ( lp >= rp ) return;
  for ( int *i = lp; i < rp; ++i ) {
    for ( int *j = i + 1; j <= rp; ++j ) {
      if ( *i > *j ) {
        int tmp = *i;
        *i = *j;
        *j = tmp;
      }
    }
  }
}

void printStatus() {
  printf("Status:\n");
  for(int i = 0; i < 15; ++i) {
    printf("[%dL] %ld\t[%dR]%ld\n", i+1, left[i]-mostLeft, i+1, right[i]-mostLeft);
  }
}


//void* taskWork( void *arg ) {
void* taskWork( int arg ) {
  //int taskNum = *((int*)arg); // 1~15
  int taskNum = arg; // 1~15

  // Task work start
  #ifdef DEBUG
  printf("Task %d started\n", taskNum);
  #endif

  if (taskNum <= 7) {
    // T1 to T7
//    int *middle = partition(left[taskNum - 1], right[taskNum - 1]);
    int *middle = partition(left[taskNum-1], right[taskNum-1]);
    left[taskNum * 2 - 1] = left[taskNum - 1];
    right[taskNum * 2 - 1] = middle;

    left[taskNum * 2] = (middle + 1 > right[taskNum-1]) ? middle : middle + 1;
    right[taskNum * 2] = right[taskNum - 1];
  } else {
    // T8 to T15
    bubbleSort(left[taskNum - 1], right[taskNum - 1]);
  }

  // Task done
  #ifdef DEBUG
  printf("Task done %d\n", taskNum);
  printStatus();
  #endif

  if (taskNum <= 7) { // if it is T1~T7
    // Push next 2 task into task queue of threadpool
    Task task1(taskWork, taskNum * 2);
    Task task2(taskWork, taskNum * 2 + 1);
    //Task task1(taskWork, new int(taskNum * 2));
    //Task task2(taskWork, new int(taskNum * 2 + 1));
    threadpool.pushTask(task1);
    threadpool.pushTask(task2);
  }

  if (++numTaskDone == 15) {
    numTaskDone = 0;
    sem_post(&sortDone);
  }
  return NULL;
}


void readInput() {
  char inputFileName[20];
  printf("Input file: ");
  scanf("%s", inputFileName);

  // Load from file
  FILE* fp = fopen(inputFileName, "r");
  fscanf(fp, "%d", &numCnt);
  processingArr = (int *) malloc(sizeof(int) * numCnt);
  inputArr = (int *) malloc(sizeof(int) * numCnt);
  for ( int i = 0; i < numCnt; ++i ) {
    fscanf(fp, "%d", inputArr + i);
		processingArr[i] = inputArr[i];
  }
  fclose(fp);
}



int main() {

  readInput();
  sem_init(&sortDone, 0, 0);

  mostLeft = processingArr;
  for ( int numActiveThread = 1; numActiveThread <= 8; ++numActiveThread ) {
    if (numActiveThread != 1) {
      threadpool.addOneActiveThread();
    }
    printf("\nStart sorting using a thread pool of %d thread.\n", numActiveThread);

    for ( int i = 0; i < numCnt; ++i ) {
      processingArr[i] = inputArr[i];
    }

    // Record start time
    struct timeval start, end;
    gettimeofday(&start, 0);

    left[0] = processingArr;
    right[0] = processingArr + numCnt - 1;

    // Start processing, push first task into task queue of threadpool
    Task task(taskWork, 1);
    threadpool.pushTask(task);
 
    // Wait all sort done
    sem_wait(&sortDone);

    // Output result to file
    char filename[20];
    sprintf(filename, "output_%d.txt", numActiveThread);
    FILE* fp = fopen(filename, "w");
    for ( int i = 0; i < numCnt; ++i ) {
      fprintf(fp, "%d ", processingArr[i]);
    }
    printf("Sort done! result in %s\n", filename);


    // Print elapsed time
    gettimeofday(&end, 0);
    int sec = end.tv_sec - start.tv_sec;
    int usec = end.tv_usec - start.tv_usec;

    printf("Elapsed time: %lf sec\n", sec + (usec / 1000000.0));

    #ifdef DEBUG
    bool checkOK = true;
    for ( int i = 1; i < numCnt; ++i ) {
      if ( processingArr[i-1] > processingArr[i] ) {
        printf("[%d] %d > [%d] %d\n", i-1, processingArr[i-1], i, processingArr[i]);
        checkOK = false;
      }
    }
    if ( checkOK ) {
      printf("Result check OK!\n");
    } else {
      printf("Result check is not correct QQ\n");
    }
    #endif
    
  }

  // Clean up
  free(processingArr);
	free(inputArr);

  return 0;
}
