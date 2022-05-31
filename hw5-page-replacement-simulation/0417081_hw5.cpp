#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <sys/time.h>

using namespace std;

// Doubly linked list implementation
class Node {
  public:
    Node* next;
    Node* prev;
    int data;
};

class LinkedList {
  public:
    int length;
    Node* head;
    Node* tail;
    
    LinkedList();
    ~LinkedList();

    void pop_head();
    void pop_tail();
    void push_tail(int data);
    void erase(Node* node);
    void move_to_tail(Node* node);
};

LinkedList::LinkedList(){
  this->length = 0;
  this->head = NULL;
  this->tail = NULL;
}

LinkedList::~LinkedList(){
  while (this->length != 0) {
    pop_head();
  }
}

void LinkedList::push_tail(int data){
  Node* node = new Node();
  node->data = data;
  node->next = NULL;
  node->prev = this->tail;
  
  if ( this->tail ) {
    this->tail->next = node;
  }

  if ( this->length++ == 0 ) {
    this->head = node;
  }
  this->tail = node;
}


void LinkedList::pop_head(){

  if ( this->length == 0 ) { // list is empty
    return;
  } else if ( this->head->next ) { // there are more than 1 node
    this->head = this->head->next;
    delete this->head->prev;
    this->head->prev = NULL;
  } else { // there is only 1 node left
    delete this->head;
    this->length = 0;
    this->head = NULL;
    this->tail = NULL;
  }
}

void LinkedList::pop_tail(){

  if ( this->length == 0 ) { // list is empty
    return;
  } else if ( this->tail->prev ) { // there are more than 1 node
    this->tail = this->tail->prev;
    delete this->tail->next;
    this->tail->next = NULL;
  } else { // there is only 1 node left
    delete this->tail;
    this->length = 0;
    this->head = NULL;
    this->tail = NULL;
  }
}


void LinkedList::erase(Node* node) {
  // WARNING: The function does NOT check whether
  // this node is on this linked-list!!
  if ( node == this->head ) {
    pop_head();
  } else if ( node == this->tail ) {
    pop_tail();
  } else {
    node->next->prev = node->prev;
    node->prev->next = node->next;
    delete node;
    this->length--;
  }
}


void LinkedList::move_to_tail(Node* node) {
  // WARNING: The function does NOT check whether
  // this node is on this linked-list!!
  if ( node == this->tail ) { // node is already tail
    return;
  } else if (node == this->head) { // node is head
    this->head = this->head->next;
    this->head->prev = NULL;
  } else { // node is in the middle
    node->next->prev = node->prev;
    node->prev->next = node->next;
  }
  node->next = NULL;
  node->prev = this->tail;
  this->tail->next = node;
  this->tail = node;
}



int main() {

  // Prompt for trace file name
  string filename;
  cout << "Enter input filename (empty for 'trace.txt'): ";
  getline(cin, filename);

  if ( filename.empty() ) {
    filename = "trace.txt";
  }

  // Record start time
  struct timeval start, end;
  gettimeofday(&start, 0);

  // Read input file
  vector<int> targets;

  ifstream file;
  file.open(filename);

  int buf;
  while ( file >> buf ) {
    targets.push_back(buf);
  }

  file.close();
  int sizeTargets = targets.size();
  
  // ==========

  // Simulate using FIFO
  cout << "\nFIFO---" << endl;
  cout << "size\t\tmiss\t\thit\t\tpage fault ratio" << endl;
  for ( int numFrame = 128; numFrame <= 1024; numFrame *= 2 ) {
    // Loop through each simulation case
    
    int cntMiss = 0; // count of Misses

    unordered_set<int> lookup; // Loop up table
    queue<int> frames; // frames
    bool isFramesFull = false;

    for ( int i = 0; i < sizeTargets; ++i ) {
      if ( lookup.find(targets[i]) == lookup.end() ) { 
        // target not in page table
        
        if ( isFramesFull ) {
          // swap out victim
          int victim = frames.front();
          lookup.erase(victim);
          frames.pop();
        }

        // swap in target
        lookup.insert(targets[i]);
        frames.push(targets[i]);

        // increase miss count by 1
        if ( ++cntMiss == numFrame ) {
          isFramesFull = true;
        }
      }
    }

    // Print result for each test case
    double faultRatio = (double)cntMiss / (double)sizeTargets;
    cout << numFrame << "\t\t" << cntMiss << "\t\t" << sizeTargets - cntMiss << "\t\t" << setprecision(9) << fixed << faultRatio << endl;
  }
  


  // ==========

  // Simulate using LRU
  cout << "\nLRU---" << endl;
  cout << "size\t\tmiss\t\thit\t\tpage fault ratio" << endl;
  for ( int numFrame = 128; numFrame <= 1024; numFrame *= 2 ) {
    // Loop through each simulation cases
    
    int cntMiss = 0; // count of Misses

    unordered_map<int, Node*> lookup; // Loop up table

    LinkedList frames; // frames
    bool isFramesFull = false;

    for ( int i = 0; i < sizeTargets; ++i ) {
      if ( lookup.find(targets[i]) == lookup.end() ) { 
        // target not in page table
        
        if ( isFramesFull ) {
          // swap out victim
          int victim = frames.head->data;
          frames.pop_head();
          lookup.erase(victim);
        }

        // swap in target
        frames.push_tail(targets[i]);
        lookup.insert(pair<int, Node*>(targets[i], frames.tail));

        if ( ++cntMiss == numFrame ) {
          isFramesFull = true;
        }
      } else {
        // target is in page table
        
        // update target to the end of frames
        frames.move_to_tail(lookup[targets[i]]);
        lookup[targets[i]] = frames.tail;
      }
    }

    double faultRatio = (double)cntMiss / (double)sizeTargets;
    cout << numFrame << "\t\t" << cntMiss << "\t\t" << sizeTargets - cntMiss << "\t\t" << setprecision(9) << fixed << faultRatio << endl;
  }

  // Print elapsed time
  gettimeofday(&end, 0);
  int sec = end.tv_sec - start.tv_sec;
  int usec = end.tv_usec - start.tv_usec;

  printf("\nElapsed time: %lf sec\n", sec + (usec / 1000000.0));

  return 0;
}
