#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "outstreams.hpp"
#include "SC2Map.hpp"
#include "PrioQueue.hpp"



////////////////////////////////////////////
//
//  For some Node u the neighbor links we
//  will bother to make are 16, provides a
//  good approximation of fully-connecting
//  every Node that has a pathable straight
//  line to another Node
//
//        15       8
//         .       .
//    14 . 7 . 0 . 4 . 9
//         .   .   .
//         3 . u . 1
//         .   .   .
//    13 . 6 . 2 . 5 . 10
//         .       .
//        12       11
//
//  Note there is no need to link u directly
//  to any more cells in a straight line,
//  that path will naturally be built.
//
////////////////////////////////////////////
float SC2Map::k0 = sqrt( 2.0f ); // distance u-4, etc.
float SC2Map::k1 = sqrt( 5.0f ); // distance u-8, etc.
float SC2Map::neighborWeights[] =
{
  1.0f, 1.0f, 1.0f, 1.0f,
  k0,   k0,   k0,   k0,
  k1,   k1,   k1,   k1,
  k1,   k1,   k1,   k1,
};



// shortest paths can be calculated between any two
// nodes in the pathing map for any path type, just
// prep the structures and only calculate shortest
// paths when other code requests an answer
void SC2Map::prepShortestPaths()
{
  // there are a graph of path nodes for every path type
  mapPathNodes = new Node*[txDimPlayable*
                           tyDimPlayable*
                           NUM_PATH_TYPES];

  for( int t = 0; t < NUM_PATH_TYPES; ++t )
  {
    buildPathGraph( (PathType)t );
  }
}


void SC2Map::setPathNode( point* c, PathType t, Node* u )
{
  if( !isPlayableCell( c ) )
  {
    printError( "Attempt to access data from unplayable cell (%d, %d).\n",
                c->pcx,
                c->pcy );
    exit( -1 );
  }
  mapPathNodes[NUM_PATH_TYPES*(c->pcy*cxDimPlayable + c->pcx) + t] = u;
}


Node* SC2Map::getPathNode( point* c, PathType t )
{
  if( !isPlayableCell( c ) )
  {
    // allow the "getter" of nodes to simply return NULL when
    // access is out of bounds to simplify logic of algorithms
    // that start at a given cell and try to inspect the neighbors
    return NULL;
  }

  return mapPathNodes[NUM_PATH_TYPES*(c->pcy*cxDimPlayable + c->pcx) + t];
}



void SC2Map::buildPathGraph( PathType t )
{
  // first create a node for every pathable cell
  // for the given pathing type
  for( int pci = 0; pci < cxDimPlayable; ++pci )
  {
    for( int pcj = 0; pcj < cyDimPlayable; ++pcj )
    {
      point c;
      c.pcSet( pci, pcj );

      Node* u = NULL;

      if( getPathing( &c, t ) )
      {
        u = new Node();

        u->pathsFromThisSrcCalculated = false;

        u->loc.pcSet( pci, pcj );

        for( int i = 0; i < NUM_NODE_NEIGHBORS; ++i )
        {
          u->neighbors[i] = NULL;
        }

        u->id = this->nodes[t].size();
        this->nodes[t].push_back( u );
      }

      setPathNode( &c, t, u );
    }
  }

  // all the nodes are created, now run a second pass
  // where we decide which of the possible 16 neighbor
  // edges to create based on what neighbors exist
  for( int pci = 0; pci < cxDimPlayable; ++pci )
  {
    for( int pcj = 0; pcj < cyDimPlayable; ++pcj )
    {
      // useful points for the following neighbor
      // calculations and tests--all in cell coordinates
      point c_p0_p0; c_p0_p0.pcSet( pci + 0, pcj + 0 );
      point c_p0_p1; c_p0_p1.pcSet( pci + 0, pcj + 1 );
      point c_p1_p2; c_p1_p2.pcSet( pci + 1, pcj + 2 );
      point c_p1_p1; c_p1_p1.pcSet( pci + 1, pcj + 1 );
      point c_p2_p1; c_p2_p1.pcSet( pci + 2, pcj + 1 );
      point c_p1_p0; c_p1_p0.pcSet( pci + 1, pcj + 0 );
      point c_p2_m1; c_p2_m1.pcSet( pci + 2, pcj - 1 );
      point c_p1_m1; c_p1_m1.pcSet( pci + 1, pcj - 1 );
      point c_p1_m2; c_p1_m2.pcSet( pci + 1, pcj - 2 );
      point c_p0_m1; c_p0_m1.pcSet( pci + 0, pcj - 1 );


      Node* u = getPathNode( &c_p0_p0, t );
      if( u == NULL ) { continue; }

      // we only have to link to half of u's possible
      // neighbors because the the other half of the links
      // will be made by a node v who has u as its neighbor
      Node* v;

      // for adjacent cells, if it is there its connected
      v = getPathNode( &c_p0_p1, t );
      if( v != NULL )
      {
        u->neighbors[0] = v;
        v->neighbors[2] = u;
      }

      v = getPathNode( &c_p1_p0, t );
      if( v != NULL )
      {
        u->neighbors[1] = v;
        v->neighbors[3] = u;
      }

      // for diagonal cells, we say there is a path if one
      // of the two adjacent cells is pathable
      // THIS WOULD BE GREAT IF t3SyncInfoPathing WAS AVAILABLE
      // IN EVERY MAP.  SINCE IT IS NOT, WE HAVE TO APPROXIMATE
      // PATHING AND IN SOME CASES A TRUE PATH IS ONLY CONNECTED
      // TO ANOTHER CELL BY A DIAGONAL NEIGHBOR, SO FOR NOW IT'S
      // IF DIAG NEIGHBOR EXISTS --> IT'S PATHABLE
      v = getPathNode( &c_p1_p1, t );
      if( v != NULL )
      {
        //if( getPathNode( &c_p0_p1, t ) != NULL ||
        //    getPathNode( &c_p1_p0, t ) != NULL
        //  ) {
          u->neighbors[4] = v;
          v->neighbors[6] = u;
        //}
      }

      v = getPathNode( &c_p1_m1, t );
      if( v != NULL )
      {
        //if( getPathNode( &c_p0_m1, t ) != NULL ||
        //    getPathNode( &c_p1_p0, t ) != NULL
        //  ) {
          u->neighbors[5] = v;
          v->neighbors[7] = u;
        //}
      }

      // for the knight's jump cells if BOTH the adjacent
      // cells in between are pathable, it suffices:
      //    P v   <-- if P cells are pathable, make
      //  u P         the link u<-->v
      v = getPathNode( &c_p1_p2, t );
      if( v != NULL )
      {
        if( getPathNode( &c_p0_p1, t ) != NULL &&
            getPathNode( &c_p1_p1, t ) != NULL
          ) {
          u->neighbors[8]  = v;
          v->neighbors[12] = u;
        }
      }

      v = getPathNode( &c_p2_p1, t );
      if( v != NULL )
      {
        if( getPathNode( &c_p1_p0, t ) != NULL &&
            getPathNode( &c_p1_p1, t ) != NULL
          ) {
          u->neighbors[9]  = v;
          v->neighbors[13] = u;
        }
      }

      v = getPathNode( &c_p2_m1, t );
      if( v != NULL )
      {
        if( getPathNode( &c_p1_p0, t ) != NULL &&
            getPathNode( &c_p1_m1, t ) != NULL
          ) {
          u->neighbors[10] = v;
          v->neighbors[14] = u;
        }
      }

      v = getPathNode( &c_p1_m2, t );
      if( v != NULL )
      {
        if( getPathNode( &c_p0_m1, t ) != NULL &&
            getPathNode( &c_p1_m1, t ) != NULL
          ) {
          u->neighbors[11] = v;
          v->neighbors[15] = u;
        }
      }
    }
  }
}


float SC2Map::getShortestPathDistance( point* src, point* dst, PathType t )
{
  if( !isPlayableCell( src ) || !isPlayableCell( dst ) )
  {
    return infinity;
  }

  Node* u = getPathNode( src, t );
  Node* v = getPathNode( dst, t );

  if( u == NULL || v == NULL )
  {
    return infinity;
  }

  return getShortestPathDistance( u, v, t );
}


float SC2Map::getShortestPathDistance( Node* u, Node* v, PathType t )
{
  if( u == v )
  {
    return 0.0f;
  }

  if( !(u->pathsFromThisSrcCalculated) )
  {
    computeShortestPaths( u, t );
    u->pathsFromThisSrcCalculated = true;
  }
  return (*(d[t][u->id]))[v->id];
}


void SC2Map::setShortestPathDistance( Node* u, Node* v, PathType t, float dIn )
{
  (*(d[t][u->id]))[v->id] = dIn;
}


Node* SC2Map::getShortestPathPredecessor( Node* u, Node* v, PathType t )
{
  if( u == v )
  {
    return NULL;
  }

  if( !(u->pathsFromThisSrcCalculated) )
  {
    computeShortestPaths( u, t );
    u->pathsFromThisSrcCalculated = true;
  }
  return (*(pi[t][u->id]))[v->id];
}


void SC2Map::setShortestPathPredecessor( Node* u, Node* v, PathType t, Node* piIn )
{
  (*(pi[t][u->id]))[v->id] = piIn;
}


void SC2Map::computeShortestPaths( Node* src, PathType t )
{
  // add entries to the shortest path hashmaps for this source
  vector<float>*  dEntry = new vector<float>();
  vector<Node*>* piEntry = new vector<Node*>();

   dEntry->resize( nodes[t].size() );
  piEntry->resize( nodes[t].size() );

  d [t].insert( make_pair( src->id,  dEntry ) );
  pi[t].insert( make_pair( src->id, piEntry ) );


  if( !pqueue.isEmpty() )
  {
    printError( "Priority Queue not empty when starting a new shortest paths calculation.\n" );
    exit( -1 );
  }

  // initialize all nodes in the shortest path calculation
  for( int i = 0; i < nodes[t].size(); ++i )
  {
    Node* u = nodes[t][i];

    // effectively infinity, no map has a shortest path
    // of millions of cells from one point to another
    pqueue.insert( u, infinity );

    setShortestPathPredecessor( src, u, t, NULL );
  }

  pqueue.decreaseKey( src, 0.0f );

  while( !pqueue.isEmpty() )
  {
    Node* u = pqueue.extractMin();

    setShortestPathDistance( src, u, t, u->key );

    for( int i = 0; i < NUM_NODE_NEIGHBORS; ++i )
    {
      Node* v = u->neighbors[i];

      if( v == NULL ) { continue; }

      if( v->key > u->key + neighborWeights[i] )
      {
        pqueue.decreaseKey( v, u->key + neighborWeights[i] );

        setShortestPathPredecessor( src, v, t, u );
      }
    }
  }
}


float SC2Map::getShortestAirDistance( point* src, point* dst )
{
  float dx = src->mx - dst->mx;
  float dy = src->my - dst->my;

  return sqrt( dx*dx + dy*dy );
}


bool SC2Map::effectivelyInfinity( float x )
{
  return x > infinity - 1.0f;
}



float SC2Map::getShortestPathDistance( point* p, Base* b, PathType t )
{
  if( !isPlayableCell( p ) )
  {
    return infinity;
  }

  Node* u = getPathNode( p, t );
  if( u == NULL )
  {
    return infinity;
  }

  return getShortestPathDistance( u, b, t );
}


float SC2Map::getShortestPathDistance( Node* u, Base* b, PathType t )
{
  if( !(u->pathsFromThisSrcCalculated) )
  {
    computeShortestPaths( u, t );
    u->pathsFromThisSrcCalculated = true;
  }

  float dShortest = infinity;

  for( map<Node*, float>::iterator itr = (b->node2patchDistance[t]).begin();
       itr != (b->node2patchDistance[t]).end();
       ++itr )
  {
    Node* v      = itr->first;
    float dPatch = itr->second;
    float dRoute = getShortestPathDistance( u, v, t );

    if( dPatch + dRoute < dShortest )
    {
      dShortest = dPatch + dRoute;
    }
  }

  return dShortest;
}


float SC2Map::getShortestPathDistance( Base* b1, Base* b2, PathType t )
{
  float dShortest = infinity;

  for( map<Node*, float>::iterator itr1 = (b1->node2patchDistance[t]).begin();
       itr1 != (b1->node2patchDistance[t]).end();
       ++itr1 )
  {
    Node* u       = itr1->first;
    float dPatch1 = itr1->second;


    for( map<Node*, float>::iterator itr2 = (b2->node2patchDistance[t]).begin();
         itr2 != (b2->node2patchDistance[t]).end();
         ++itr2 )
    {
      Node* v       = itr2->first;
      float dPatch2 = itr2->second;


      float dRoute = getShortestPathDistance( u, v, t );

      if( dPatch1 + dPatch2 + dRoute < dShortest )
      {
        dShortest = dPatch1 + dPatch2 + dRoute;
      }
    }
  }

  return dShortest;
}


Node* SC2Map::getShortestPathPredecessor( Node* u, Base* b, PathType t )
{
  if( !(u->pathsFromThisSrcCalculated) )
  {
    computeShortestPaths( u, t );
    u->pathsFromThisSrcCalculated = true;
  }

  float dShortest = infinity;
  Node* pred      = NULL;

  for( map<Node*, float>::iterator itr = (b->node2patchDistance[t]).begin();
       itr != (b->node2patchDistance[t]).end();
       ++itr )
  {
    Node* v      = itr->first;
    float dPatch = itr->second;
    float dRoute = getShortestPathDistance( u, v, t );

    if( dPatch + dRoute < dShortest )
    {
      dShortest = dPatch + dRoute;
      pred      = v;
    }
  }

  return pred;
}


void SC2Map::getShortestPathPredecessors( Base* b0, Base* b1, PathType t,
                                          Node** uOut, Node** vOut )
{
  float dShortest = infinity;

  for( map<Node*, float>::iterator itr0 = (b0->node2patchDistance[t]).begin();
       itr0 != (b0->node2patchDistance[t]).end();
       ++itr0 )
  {
    Node* u       = itr0->first;
    float dPatch0 = itr0->second;


    for( map<Node*, float>::iterator itr1 = (b1->node2patchDistance[t]).begin();
         itr1 != (b1->node2patchDistance[t]).end();
         ++itr1 )
    {
      Node* v       = itr1->first;
      float dPatch1 = itr1->second;


      float dRoute = getShortestPathDistance( u, v, t );

      if( dPatch0 + dPatch1 + dRoute < dShortest )
      {
        dShortest = dPatch0 + dPatch1 + dRoute;

        *uOut = u;
        *vOut = v;
      }
    }
  }
}
