#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "outstreams.hpp"
#include "SC2Map.hpp"
#include "PrioQueue.hpp"


int PrioQueue::parent( int i )
{
  // i/2
  return i >> 1;
}

int PrioQueue::left( int i )
{
  // 2*i
  return i << 1;
}

int PrioQueue::right( int i )
{
  // 2*i + 1
  return (i << 1) + 1;
}

inline Node* PrioQueue::get( int i )
{
  // already boundary safe from STL
  return heap.at( i );
}

inline void PrioQueue::set( int i, Node* u )
{
  // do our own online capacity increases
  if( i >= heap.size() )
  {
    heap.resize( 2*i + 10, NULL );
  }
  heap[i] = u;
}



PrioQueue::PrioQueue()
{
  size = 0;
}


bool PrioQueue::isEmpty()
{
  return size == 0;
}


void PrioQueue::insert( Node* u, float key )
{
  set( size, u );
  u->queueIndex = size;
  ++size;

  u->key = key;
  decreaseKey( u, key );
}


Node* PrioQueue::extractMin()
{
  if( isEmpty() )
  {
    printError( "Heap underflow.\n" );
    exit( -1 );
  }

  Node* min = get( 0 );
  min->queueIndex = -1;

  set( 0, get( size - 1 ) );
  --size;

  heapify( 0 );

  return min;
}


void PrioQueue::decreaseKey( Node* u, float newKey )
{
  if( newKey > u->key )
  {
    printError( "New key is larger than current key.\n" );
    exit( -1 );
  }

  int i = u->queueIndex;
  u->key = newKey;

  int p = parent( i );
  Node* v = get( p );

  while( i > 0 && v->key > u->key )
  {
    // swap i and parent
    set( p, u );
    set( i, v );
    u->queueIndex = p;
    v->queueIndex = i;

    i = p;
    p = parent( i );

    u = get( i );
    v = get( p );
  }
}


// enforce the min-heap property
void PrioQueue::heapify( int i )
{
  int l = left ( i );
  int r = right( i );

  int smallest;

  if( l < size && get( l )->key < get( i )->key )
  {
    smallest = l;
  } else {
    smallest = i;
  }

  if( r < size && get( r )->key < get( smallest )->key )
  {
    smallest = r;
  }

  if( smallest != i )
  {
    // swap i and smallest
    Node* ti = get( i );
    Node* ts = get( smallest );
    set( i,        ts );
    set( smallest, ti );
    ti->queueIndex = smallest;
    ts->queueIndex = i;

    heapify( smallest );
  }
}
