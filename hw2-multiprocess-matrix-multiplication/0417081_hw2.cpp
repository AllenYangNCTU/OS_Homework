#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <errno.h>
using namespace std;

#define MATRIX_A_KEY 0
#define MATRIX_B_KEY 1
#define MATRIX_C_KEY 2

int main() {

  // Get dimension of matrix (100~800)
  int dim;
  cout << "Input the matrix dimension: " << flush;
  cin >> dim;
  cout << endl;

  unsigned int *matrixA;
  unsigned int *matrixB;
  unsigned int *matrixC;
  
  // Create shared memory block for matrixA, B, C
  int shmidA = shmget(MATRIX_A_KEY,
                      sizeof(unsigned int) * dim * dim,
                      IPC_CREAT | 0666);
  if (shmidA == -1) {
    cerr << "Error: fail to create shared memory block" << endl;
    cerr << strerror(errno) << endl;
  }

  int shmidB = shmget(MATRIX_B_KEY,
                      sizeof(unsigned int) * dim * dim,
                      IPC_CREAT | 0666);
  if (shmidB == -1) {
    cerr << "Error: fail to create shared memory block" << endl;
    cerr << strerror(errno) << endl;
  }
  int shmidC = shmget(MATRIX_C_KEY,
                      sizeof(unsigned int) * dim * dim,
                      IPC_CREAT | 0666);
  if (shmidC == -1) {
    cerr << "Error: fail to create shared memory block" << endl;
    cerr << strerror(errno) << endl;
  }
  
  // Init value of matrixA, B
  matrixA = (unsigned int *) shmat(shmidA, NULL, 0);
  matrixB = (unsigned int *) shmat(shmidB, NULL, 0);
  if (matrixA == (unsigned int*)-1 || matrixB == (unsigned int*)-1) {
    cerr << "Error: fail to attach matrixA, B" << endl;
    exit(1);
  }
  int length = dim * dim;
  for (int i = length-1; i != -1; --i) {
    matrixA[i] = i;
    matrixB[i] = i;
  }
  shmdt(matrixA);
  shmdt(matrixB);
  

  // Fork worker processes
  
  // Loop through 16 cases (use 1~16 processes to calculate)
  for (int numProcess = 1; numProcess <= 16; ++numProcess) {

    cout << "Multiplying matrices using " << numProcess 
         << ((numProcess == 1) ? " process" : " processes") << endl;

    struct timeval start, end;
    gettimeofday(&start, 0);

    int rowSliceLength = dim / numProcess;
    // Loop through each process
    for (int processCnt = 1; processCnt <= numProcess; ++processCnt) {
      int childpid;
      if ((childpid = fork()) == -1) {
        cerr << "Error: fail to fork" << endl;
        exit(1);
      } else if (childpid == 0) {
        // child worker
        matrixA = (unsigned int *) shmat(shmidA, NULL, 0);
        matrixB = (unsigned int *) shmat(shmidB, NULL, 0);
        matrixC = (unsigned int *) shmat(shmidC, NULL, 0);
        
        // Do matrix multiplication
        int cRowBottom = (processCnt - 1) * rowSliceLength;
        int cRowTop = (processCnt == numProcess) ?
                      dim : processCnt * rowSliceLength;
        for (int cRow = cRowBottom; cRow < cRowTop; ++cRow) {
          for (int cCol = 0; cCol < dim; ++cCol) {
            unsigned int sum = 0;
            for (int i = 0; i < dim; ++i) {
              sum += matrixA[cRow * dim + i] * matrixB[i * dim + cCol];
            }
            matrixC[cRow * dim + cCol] = sum;
          }
        }

        shmdt(matrixA);
        shmdt(matrixB);
        shmdt(matrixC);
        exit(0);
      } else {
        // parent
        // Does nothing here, just keep forking!
      }
    } // End for loop through each process
    
    // Wait each worker process done
    for (int i = 0; i < numProcess; ++i) {
      wait(NULL);
    }

    // Calculate checksum
    matrixC = (unsigned int*) shmat(shmidC, NULL, 0);
    if (matrixC == (unsigned int*)-1) {
      cerr << "Error: fail to attach matrixC" << endl;
      exit(1);
    }
    unsigned int checksum = 0;
    for (int i = dim * dim - 1; i != -1; --i) {
      checksum += matrixC[i];
    }
    shmdt(matrixC);

    // Stop timeing
    gettimeofday(&end, 0);
    int sec = end.tv_sec - start.tv_sec;
    int usec = end.tv_usec - start.tv_usec;

    cout << "Elapsed time: " << sec + (usec / 1000000.0)
         << " sec, Checksum: " << checksum << endl;

  } // End for lopp through each case

  // Release shared memory
  if (shmctl(shmidA, IPC_RMID, 0) == -1 ||
      shmctl(shmidB, IPC_RMID, 0) == -1 ||
      shmctl(shmidC, IPC_RMID, 0) == -1) {
    cerr << "Error: fail to release shared memory" << endl;
    exit(1);
  }

  return 0;
}
