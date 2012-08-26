#ifndef ___PrioQueue_hpp___
#define ___PrioQueue_hpp___

#include <vector>
using namespace std;

struct Node;

// some methods of the priority queue require the
// valid index of a StartLoc in the startLocs
// vector--this is because Nodes store some info
// that is unique to each start location (like
// shortest path distance) so the priority queue
// needs the index to access the correct slot

class PrioQueue
{
public:

  PrioQueue();

  // normal priority queue interface
  bool  isEmpty    ();
  void  insert     ( Node* u, float key );
  void  decreaseKey( Node* u, float newKey );
  Node* extractMin ();

protected:

  vector<Node*> heap;
  int           size;

  inline int parent( int i );
  inline int left  ( int i );
  inline int right ( int i );

  // boundary-safe vector element access
  inline Node* get( int i );
  inline void  set( int i, Node* u );

  void heapify( int i );
};


#endif // ___PrioQueue_hpp___
