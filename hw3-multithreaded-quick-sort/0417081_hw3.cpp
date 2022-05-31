#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

//#define DEBUG

int numCnt;
int* inputArr;
int* arr; // for process

// [0] for signal T1 to start
// [1] means T1 has done, signal T2, T3 to start
// [2] means T2 has done, for signal T4, T5 to start
// [3] means T3 has done, for signal T6, T7 to start
// [4] means T4 has done, for signal T8, T9 to start
// [5] means T5 has done, for signal T10, T11 to start
// [6] means T6 has done, for signal T12, T13 to start
// [7] means T7 has done, for signal T14, T15 to start
// [8-15] means T8-15 has done, when all done, signal T1 to report completion
// [16] means all above done, T1 signal the main thread for competion
sem_t semThreadHasDone[17];

int *left[15];  // array left bound for T1-T15
int *right[15]; // array right bound for T1-T15


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
  #ifdef DEBUG
  if(rp != lp) {
    printf("rp != lp\n");
  }
  #endif
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


void * threadWork( void *ptr ) {
  int *threadNum = (int *) ptr; // from 1~15

  #ifdef DEBUG
  printf("Thread create %d\n", *threadNum);
  #endif

  // Wait there own semaphore signal to start thread
  switch(*threadNum) {
    case 1:
      sem_wait(&semThreadHasDone[0]);
      break;
    case 2:
    case 3:
      sem_wait(&semThreadHasDone[1]);
      break;
    case 4:
    case 5:
      sem_wait(&semThreadHasDone[2]);
      break;
    case 6:
    case 7:
      sem_wait(&semThreadHasDone[3]);
      break;
    case 8:
    case 9:
      sem_wait(&semThreadHasDone[4]);
      break;
    case 10:
    case 11:
      sem_wait(&semThreadHasDone[5]);
      break;
    case 12:
    case 13:
      sem_wait(&semThreadHasDone[6]);
      break;
    case 14:
    case 15:
      sem_wait(&semThreadHasDone[7]);
      break;
  }

  // Thread work start
  #ifdef DEBUG
  printf("Thread %d started\n", *threadNum);
  #endif


  #ifdef DEBUG
  FILE *fp = fopen("debug.txt", "a");
  fprintf(fp, "[T%d] start ", *threadNum);
  for(int *i = left[*threadNum-1]; i <= right[*threadNum-1]; ++i) {
    fprintf(fp, "%d ", *i);
  }
  fprintf(fp, "\n");
  fclose(fp);
  #endif

  if (*threadNum <= 7) {
    // T1 to T7
    int *middle = partition(left[*threadNum - 1], right[*threadNum - 1]);
    left[*threadNum * 2 - 1] = left[*threadNum - 1];
    right[*threadNum * 2 - 1] = middle;
    left[*threadNum * 2] = middle + 1; // posiblly overflow
    right[*threadNum * 2] = right[*threadNum - 1];
  } else {
    // T8 to T15
    bubbleSort(left[*threadNum - 1], right[*threadNum - 1]);
  }

  #ifdef DEBUG
  fp = fopen("debug.txt", "a");
  fprintf(fp, "[T%d] end ", *threadNum);
  for(int *i = left[*threadNum-1]; i <= right[*threadNum-1]; ++i) {
    fprintf(fp, "%d ", *i);
  }
  fprintf(fp, "\n");
  fclose(fp);
  #endif


  // Thread done, send its own signal
  sem_post(&semThreadHasDone[*threadNum]);
  sem_post(&semThreadHasDone[*threadNum]);

  if (*threadNum == 1) {
    // T1 should wait all T8-T15 done, and report completion
    sem_wait(&semThreadHasDone[8]);
    sem_wait(&semThreadHasDone[9]);
    sem_wait(&semThreadHasDone[10]);
    sem_wait(&semThreadHasDone[11]);
    sem_wait(&semThreadHasDone[12]);
    sem_wait(&semThreadHasDone[13]);
    sem_wait(&semThreadHasDone[14]);
    sem_wait(&semThreadHasDone[15]);

    sem_post(&semThreadHasDone[16]);
  }

  #ifdef DEBUG
  printf("Thread done %d\n", *threadNum);
  #endif

  pthread_exit(NULL);
}


void singleThread() {
	// directly use inputArr for processing
	left[0] = inputArr;
	right[0] = inputArr + numCnt - 1;
	
	// Do partition (work of T1 to T7)
	for (int i = 0; i < 7; ++i) {
		int *middle = partition(left[i], right[i]);
		left[i * 2 + 1] = left[i];
		right[i * 2 + 1] = middle;
		left[i * 2 + 2] = middle + 1;
		right[i * 2 + 2] = right[i];
	}

	// Do bubble sort (work of T8 to T15)
	for (int i = 7; i < 15; ++i) {
		bubbleSort(left[i], right[i]);
	}

}


int main() {

  char inputFileName[20];
  printf("Input file: ");
  scanf("%s", inputFileName);

  // Load from file
  FILE* fp = fopen(inputFileName, "r");
  fscanf(fp, "%d", &numCnt);
  arr = (int *) malloc(sizeof(int) * numCnt);
  inputArr = (int *) malloc(sizeof(int) * numCnt);
  for ( int i = 0; i < numCnt; ++i ) {
    fscanf(fp, "%d", inputArr + i);
		arr[i] = inputArr[i];
  }
  fclose(fp);


  // Create semaphores
  for (int i = 0; i < 17; ++i) {
    sem_init(semThreadHasDone + i, 0, 0);
  }


  // Create threads
  // [0] is T1, [1] is T2, ... [14] is T15
  pthread_t threads[15];
  int threadNum[15];
  for ( int i = 0; i < 15; ++i ) {
    threadNum[i] = i + 1;
    pthread_create(threads + i, NULL, threadWork, (void*)(threadNum + i));
  }


	printf("\nStart sorting using multi-thread\n");

	// Record start time
	struct timeval start, end;
	gettimeofday(&start, 0);


  // Start processing, signal T1 to start
  left[0] = arr;
  right[0] = arr + numCnt - 1;
  sem_post(&semThreadHasDone[0]);


  // Wait all thread done
  sem_wait(&semThreadHasDone[16]);

  // Output result to file
  fp = fopen("output1.txt", "w");
  for ( int i = 0; i < numCnt; ++i ) {
    fprintf(fp, "%d ", arr[i]);
  }
  printf("Sort done! result in output1.txt\n");


  // Print elapsed time
	gettimeofday(&end, 0);
	int sec = end.tv_sec - start.tv_sec;
	int usec = end.tv_usec - start.tv_usec;

	printf("Elapsed time (multi-thread): %lf sec\n", sec + (usec / 1000000.0));

  #ifdef DEBUG
  bool checkOK = true;
  for ( int i = 1; i < numCnt; ++i ) {
    if ( arr[i-1] > arr[i] ) {
      printf("[%d] %d > [%d] %d\n", i-1, arr[i-1], i, arr[i]);
      checkOK = false;
    }
  }
  if ( checkOK ) {
    printf("Result check OK!\n");
  } else {
    printf("Result check is not correct QQ\n");
  }
  #endif


	// Do the same thing using only single thread

	printf("\nStart sorting using single-thread\n");

	// Record start time
	gettimeofday(&start, 0);

	// Start work
	singleThread();

  // Output result to file
  fp = fopen("output2.txt", "w");
  for ( int i = 0; i < numCnt; ++i ) {
    fprintf(fp, "%d ", inputArr[i]);
  }
  printf("Sort done! result in output2.txt\n");


  // Print elapsed time
	gettimeofday(&end, 0);
	sec = end.tv_sec - start.tv_sec;
	usec = end.tv_usec - start.tv_usec;

	printf("Elapsed time (single-thread): %lf sec\n", sec + (usec / 1000000.0));

  #ifdef DEBUG
  checkOK = true;
  for ( int i = 1; i < numCnt; ++i ) {
    if ( inputArr[i-1] > inputArr[i] ) {
      printf("[%d] %d > [%d] %d\n", i-1, inputArr[i-1], i, inputArr[i]);
      checkOK = false;
    }
  }
  if ( checkOK ) {
    printf("Result check OK!\n");
  } else {
    printf("Result check is not correct QQ\n");
  }
  #endif



  // Clean up
  free(arr);
	free(inputArr);
  for ( int i = 0; i < 17; ++i ) {
    sem_destroy(&semThreadHasDone[i]);
  }
	//for ( int i = 0; i < 15; ++i ) {
	//	pthread_cancel(threads[i]);
//	}

  return 0;

}
