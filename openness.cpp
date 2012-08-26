#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "outstreams.hpp"
#include "coordinates.hpp"
#include "utility.hpp"
#include "SC2Map.hpp"



float SC2Map::OPENNESS_NOTCALCULATED = -1.0f;


  // openness is a numerical value for a cell that
  // determines how far away the nearest unpathable
  // cell is, or how "open" the cell is
void SC2Map::computeOpenness()
{
  // we need the previous pass's openness values
  // while we are updating the current values
  mapOpenness     = new float[cxDimPlayable*cyDimPlayable*NUM_PATH_TYPES];
  mapOpennessPrev = new float[cxDimPlayable*cyDimPlayable*NUM_PATH_TYPES];

  // initialize all cells for all pathing types to not-calculated
  for( int pci = 0; pci < cxDimPlayable; ++pci )
  {
    for( int pcj = 0; pcj < cyDimPlayable; ++pcj )
    {
      point c;
      c.pcSet( pci, pcj );

      for( int t = 0; t < NUM_PATH_TYPES; ++t )
      {
        setOpenness( &c, (PathType)t, OPENNESS_NOTCALCULATED );
      }
    }
  }

  computeOpenness( PATH_GROUND_WITHROCKS );
  computeOpenness( PATH_GROUND_NOROCKS   );
}


void SC2Map::computeOpenness( PathType t )
{
  bool firstPass = true;

  // terminate algorithm when we've calculated every pathable cell
  int numCellsCalculated = 0;

  // use to calculate average openness;
  float total = 0.0f;


  while( numCellsCalculated < numPathableCells[t] )
  {
    // to begin, copy current into the last pass
    memcpy( mapOpennessPrev,
            mapOpenness,
            cxDimPlayable*cyDimPlayable*NUM_PATH_TYPES*sizeof( float ) );

    // consult last pass to calculate current pass
    for( int pci = 0; pci < cxDimPlayable; ++pci )
    {
      for( int pcj = 0; pcj < cyDimPlayable; ++pcj )
      {
        point c;
        c.pcSet( pci, pcj );

        // only calculate openness for pathable cells
        if( !getPathing( &c, t ) )
        {
          continue;
        }

        // only set each cell once!
        if( checkHasOpennessLastPass( &c, t ) )
        {
          continue;
        }

        // Now we are considering a cell that has not been set, but IS pathable--
        // but, if no neighbors have been set, then it shouldn't be marked this pass
        //
        // If at least one neighbor has an openness value,
        // it should be set to the lowest of {openness value coming in
        // from a neighbor plus the distance to that neighbor}

        Node* u               = getPathNode( &c, t );
        float opennessCurrent = infinity;
        bool  markNow         = false;

        if( firstPass )
        {
          // at the base level (openness 0)
          // we test if any of the 4 cardinal
          // neighbors are unpathable
          if( u->neighbors[0] == NULL ||
              u->neighbors[1] == NULL ||
              u->neighbors[2] == NULL ||
              u->neighbors[3] == NULL )
          {
            markNow = true;

            // this cell is 1.0 cell away from an unpathable cell
            opennessCurrent = 1.0f;
          }

        } else {
          // otherwise we test if any neighbors have had
          // their openness set, and if so take the lowest
          // openness value for the current cell
          for( int i = 0; i < NUM_NODE_NEIGHBORS; ++i )
          {
            Node* v = u->neighbors[i];
            if( v == NULL ) { continue; }

            if( checkHasOpennessLastPass( &(v->loc), t ) )
            {
              markNow = true;

              float openness = getOpennessLastPass( &(v->loc), t ) + neighborWeights[i];
              if( openness < opennessCurrent )
              {
                opennessCurrent = openness;
              }
            }
          }
        }

        if( markNow )
        {
          setOpenness( &c, t, opennessCurrent );

          total += opennessCurrent;

          if( opennessCurrent > opennessMax[t] )
          {
            opennessMax[t] = opennessCurrent;
          }

          ++numCellsCalculated;
        }
      }
    }

    if( firstPass )
    {
      firstPass = false;
    }
  }

  opennessAvg[t] = total / (float)numCellsCalculated;
}



void SC2Map::setOpenness( point* c, PathType t, float o )
{
  if( !isPlayableCell( c ) )
  {
    printError( "Attempt to access data from unplayable cell (%d, %d).\n",
                c->pcx,
                c->pcy );
    exit( -1 );
  }
  mapOpenness[NUM_PATH_TYPES*(c->pcy*cxDimPlayable + c->pcx) + t] = o;
}


bool SC2Map::checkHasOpenness( point* c, PathType t )
{
  if( !isPlayableCell( c ) )
  {
    printError( "Attempt to access data from unplayable cell (%d, %d)\n.",
                c->pcx,
                c->pcy );
    exit( -1 );
  }
  return mapOpenness[NUM_PATH_TYPES*(c->pcy*cxDimPlayable + c->pcx) + t] > -0.5f;
}


float SC2Map::getOpenness( point* c, PathType t )
{
  if( !isPlayableCell( c ) )
  {
    printError( "Attempt to access data from unplayable cell (%d, %d)\n.",
                c->pcx,
                c->pcy );
    exit( -1 );
  }
  return mapOpenness[NUM_PATH_TYPES*(c->pcy*cxDimPlayable + c->pcx) + t];
}



bool SC2Map::checkHasOpennessLastPass( point* c, PathType t )
{
  if( !isPlayableCell( c ) )
  {
    return false;
  }

  return
    mapOpennessPrev[NUM_PATH_TYPES*(c->pcy*cxDimPlayable + c->pcx) + t] > -0.5f;
}

float SC2Map::getOpennessLastPass( point* c, PathType t )
{
  if( !isPlayableCell( c ) )
  {
    printError( "Attempt to access data from unplayable cell (%d, %d)\n.",
                c->pcx,
                c->pcy );
    exit( -1 );
  }
  return mapOpennessPrev[NUM_PATH_TYPES*(c->pcy*cxDimPlayable + c->pcx) + t];
}






float SC2Map::calculateAverageOpennessInNeighborhood( point*   p,
                                                      float    radius,
                                                      PathType t )
{
  float oTotal       = 0.0f;
  int   cellsSampled = 0;

  int r = (int)(radius + 1.0f);

  for( int pci = -r; pci < r; ++pci )
  {
    for( int pcj = -r; pcj < r; ++pcj )
    {
      // only sample openness in circle
      // around the base
      float xsq = (float)(pci*pci);
      float ysq = (float)(pcj*pcj);
      if( sqrt( xsq + ysq ) > radius )
      {
        continue;
      }

      ++cellsSampled;

      point c;
      c.pcSet( p->pcx + pci,
               p->pcy + pcj );

      if( isPlayableCell( &c ) &&
          checkHasOpenness( &c, t ) )
      {
        oTotal += getOpenness( &c, t );
      }
    }
  }

  return oTotal / (float)cellsSampled;
}






// Find a point on the shortest path from a main to every other main.  If a map does
// not have this, I think it does not have one main choke.  Once this is identified,
// could calculate space in main with the pathing-fill algorithm, so don't duplicate
// that code, reuse it.
static PathType pathTypeLocateChokes = PATH_GROUND_WITHROCKS_NORESOURCES;


void SC2Map::locateChokes()
{
  for( list<StartLoc*>::const_iterator itr1 = startLocs.begin();
       itr1 != startLocs.end();
       ++itr1 )
  {
    StartLoc* sl1 = *itr1;

    list<point> possibleChokes;


    for( list<StartLoc*>::const_iterator itr2 = startLocs.begin();
         itr2 != startLocs.end();
         ++itr2 )
    {
      StartLoc* sl2 = *itr2;

      if( sl1 == sl2 )
      {
        continue;
      }

      Node* src = getPathNode( &(sl2->loc), pathTypeLocateChokes );
      Node* u   = getPathNode( &(sl1->loc), pathTypeLocateChokes );

      if( src == NULL )
      {
        printError( "Start location %s is in an unpathable cell.\n",
                    sl2->name );
        exit( -1 );
      }

      if( u == NULL )
      {
        printError( "Start location %s is in an unpathable cell.\n",
                    sl1->name );
        exit( -1 );
      }

      Node* v = getShortestPathPredecessor( src, u, pathTypeLocateChokes );
      if( v == NULL )
      {
        // no path, just skip this pairing
        continue;
      }

      // The choke should be closer than 50% of the distance
      // from a start location to any other start location
      float dTotal = getShortestPathDistance( src, u, pathTypeLocateChokes );
      float dTest  = dTotal;

      point pBest;
      pBest.mSet( -1000.0f + sl2->loc.mx,
                  -1000.0f + sl2->loc.my );

      float dChokeMin = infinity;
      bool trippedThreshold = false;

      while( v != NULL && dTest > 0.5f*dTotal )
      {
        float dChoke = chokeDistance( &(v->loc), pathTypeLocateChokes );

        if( trippedThreshold && dChoke > dChokeMin )
        {
          // we just got a worse result, kick out
          break;
        }

        if( dChoke < getfConstant( "chokeDetectionThreshold" ) )
        {
          trippedThreshold = true;

          dChokeMin = dChoke;
          pBest.set( &(v->loc) );
        }

        u     = v;
        v     = getShortestPathPredecessor( src, u, pathTypeLocateChokes );
        dTest = getShortestPathDistance   ( src, u, pathTypeLocateChokes );
      }

      possibleChokes.push_back( pBest );
    }

    // if the best choke from this location to all others
    // is reasonably close, take the average
    bool  chokesAgree = true;
    float xTotal = 0.0f;
    float yTotal = 0.0f;
    for( list<point>::iterator itr1 = possibleChokes.begin();
         chokesAgree && itr1 != possibleChokes.end();
         ++itr1 )
    {
      point c1;
      c1.pcSet( (*itr1).pcx, (*itr1).pcy );

      list<point>::iterator itr2 = itr1;
      ++itr2;
      for( ;
           itr2 != possibleChokes.end();
           ++itr2 )
      {
        point c2;
        c2.pcSet( (*itr2).pcx, (*itr2).pcy );

        if( p2pDistance( &c1, &c2 ) > getfConstant( "chokeDetectionAgreement" ) )
        {
          chokesAgree = false;
          break;
        }
      }

      xTotal += c1.mx;
      yTotal += c1.my;
    }

    if( chokesAgree )
    {
      float xAvg = xTotal / (float)possibleChokes.size();
      float yAvg = yTotal / (float)possibleChokes.size();
      sl1->mainChoke.mSet( xAvg, yAvg );

    } else {
      printWarning( "Could not locate main choke for start location %s.\n",
                    sl1->name );
      sl1->mainChoke.mSet( -1.0f, -1.0f );
    }
  }
}



// for some point, scan in opposite directions for unpathable cells
// and add the two distances together: take the minimum span as the
// choke distance of the given point
float SC2Map::chokeDistance( point* c, PathType t )
{
  float minimumSpan = infinity;
  float span;

  span = spanDistance( c, t, -1,  0 ) + spanDistance( c, t,  1,  0 ); if( span < minimumSpan ) { minimumSpan = span; }
  span = spanDistance( c, t,  0, -1 ) + spanDistance( c, t,  0,  1 ); if( span < minimumSpan ) { minimumSpan = span; }
  span = spanDistance( c, t, -1, -1 ) + spanDistance( c, t,  1,  1 ); if( span < minimumSpan ) { minimumSpan = span; }
  span = spanDistance( c, t,  1, -1 ) + spanDistance( c, t, -1,  1 ); if( span < minimumSpan ) { minimumSpan = span; }

  return minimumSpan;
}


// from the given point, in the given unit direction, how far
// is the first unpathable cell?
float SC2Map::spanDistance( point* c, PathType t, int dx, int dy )
{
  point s;
  s.set( c );
  while( getPathingOutOfBoundsOK( &s, t ) )
  {
    s.pcSet( s.pcx + dx, s.pcy + dy );
  }
  return p2pDistance( c, &s );
}

