#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "StormLib.h"
#include "tinyxml.h"

#include "utility.hpp"
#include "debug.hpp"
#include "outstreams.hpp"
#include "coordinates.hpp"
#include "SC2Map.hpp"



void SC2Map::setMapCliffChange( point* c, bool mcc )
{
  mapCliffChanges[(c->mcx/2) + (c->mcy/2)*(cxDimMap/2)] = mcc;
}


bool SC2Map::getMapCliffChange( point* c )
{
  if( c->mcx <  0        ||
      c->mcx >= cxDimMap ||
      c->mcy <  0        ||
      c->mcy >= cyDimMap )
  {
    return false;
  }

  return mapCliffChanges[(c->mcx/2) + (c->mcy/2)*(cxDimMap/2)];
}



void SC2Map::setPathing( point* c, PathType t, bool p )
{
  if( !isPlayableCell( c ) )
  {
    // just silently fail, no problem
    return;
  }
  mapPathing[NUM_PATH_TYPES*(c->pcy*cxDimPlayable + c->pcx) + t] = p;
}



void SC2Map::setPathingAllTypes( point* c, bool p )
{
  for( int t = 0; t < NUM_PATH_TYPES; ++t )
  {
    setPathing( c, (PathType)t, p );
  }
}



void SC2Map::setPathingNonCliffTypes( point* c, bool p )
{
  setPathing( c, PATH_GROUND_NOROCKS,               p );
  setPathing( c, PATH_GROUND_WITHROCKS,             p );
  setPathing( c, PATH_GROUND_WITHROCKS_NORESOURCES, p );
  setPathing( c, PATH_BUILDABLE,                    p );
  setPathing( c, PATH_BUILDABLE_MAIN,               p );
}


void SC2Map::setPathingCliffTypes( point* c, bool p )
{
  setPathing( c, PATH_CWALK_NOROCKS,   p );
  setPathing( c, PATH_CWALK_WITHROCKS, p );
}


void SC2Map::setPathingBuildableTypes( point* c, bool p )
{
  setPathing( c, PATH_BUILDABLE,      p );
  setPathing( c, PATH_BUILDABLE_MAIN, p );
}



bool SC2Map::getPathing( point* c, PathType t )
{
  if( !isPlayableCell( c ) )
  {
    printError( "Attempt to access data from unplayable cell (%d, %d)\n.",
                c->pcx,
                c->pcy );
    exit( -1 );
  }
  return mapPathing[NUM_PATH_TYPES*(c->pcy*cxDimPlayable + c->pcx) + t];
}


bool SC2Map::getPathingOutOfBoundsOK( point* c, PathType t )
{
  if( !isPlayableCell( c ) )
  {
    return false;
  }
  return getPathing( c, t );
}




void SC2Map::computePathingFromCliffChanges()
{
  for( int t = 0; t < NUM_PATH_TYPES; ++t )
  {
    numPathableCells[t] = 0;
  }

  // the data for analysis only includes the playable area
  mapPathing = new bool[cxDimPlayable*cyDimPlayable*NUM_PATH_TYPES];

  for( int jMapCell = 0; jMapCell < cyDimMap/2; ++jMapCell )
  {
    for( int iMapCell = 0; iMapCell < cxDimMap/2; ++iMapCell )
    {
      point c_p0_p0;
      point c_p1_p0;
      point c_p0_p1;
      point c_p1_p1;
      c_p0_p0.mcSet(     2*iMapCell,     2*jMapCell );
      c_p1_p0.mcSet( 1 + 2*iMapCell,     2*jMapCell );
      c_p0_p1.mcSet(     2*iMapCell, 1 + 2*jMapCell );
      c_p1_p1.mcSet( 1 + 2*iMapCell, 1 + 2*jMapCell );


      // start by allowing all pathing
      setPathingAllTypes( &c_p0_p0, true );
      setPathingAllTypes( &c_p1_p0, true );
      setPathingAllTypes( &c_p0_p1, true );
      setPathingAllTypes( &c_p1_p1, true );

      point t_mid; t_mid.ptSet( c_p1_p1.pcx,     c_p1_p1.pcy     );
      point t_ur;  t_ur .ptSet( c_p1_p1.pcx + 1, c_p1_p1.pcy + 1 );
      point t_lr;  t_lr .ptSet( c_p1_p1.pcx + 1, c_p1_p1.pcy - 1 );
      point t_ll;  t_ll .ptSet( c_p1_p1.pcx - 1, c_p1_p1.pcy - 1 );
      point t_ul;  t_ul .ptSet( c_p1_p1.pcx - 1, c_p1_p1.pcy + 1 );

      if( !isPlayableTerrain( &t_mid ) )
      {
        continue;
      }

      u8 mid, ur, lr, ll, ul;
      mid = getHeight( &t_mid );
      if( isPlayableTerrain( &t_ur ) ) { ur = getHeight( &t_ur  ); } else { ur = mid; }
      if( isPlayableTerrain( &t_lr ) ) { lr = getHeight( &t_lr  ); } else { lr = mid; }
      if( isPlayableTerrain( &t_ll ) ) { ll = getHeight( &t_ll  ); } else { ll = mid; }
      if( isPlayableTerrain( &t_ul ) ) { ul = getHeight( &t_ul  ); } else { ul = mid; }



      // if a corner is lowest elevation, its unpathable
      if( ur == 0 ) { setPathingAllTypes( &c_p1_p1, false ); }
      if( lr == 0 ) { setPathingAllTypes( &c_p1_p0, false ); }
      if( ll == 0 ) { setPathingAllTypes( &c_p0_p0, false ); }
      if( ul == 0 ) { setPathingAllTypes( &c_p0_p1, false ); }

      // if middle os lowest, all four cells of the terrain cell are unpathable
      if( mid == 0 )
      {
        setPathingAllTypes( &c_p1_p1, false );
        setPathingAllTypes( &c_p1_p0, false );
        setPathingAllTypes( &c_p0_p0, false );
        setPathingAllTypes( &c_p0_p1, false );
      }



      // if a corner is lower than the mid, its unpathable
      if( ur < mid ) { setPathingNonCliffTypes( &c_p1_p1, false ); }
      if( lr < mid ) { setPathingNonCliffTypes( &c_p1_p0, false ); }
      if( ll < mid ) { setPathingNonCliffTypes( &c_p0_p0, false ); }
      if( ul < mid ) { setPathingNonCliffTypes( &c_p0_p1, false ); }

      // if a corner is higher than the mid, it and its adjacent
      // corners are unpathable
      if( ur > mid )
      {
        setPathingNonCliffTypes( &c_p1_p1, false );
        setPathingNonCliffTypes( &c_p1_p0, false );
        setPathingNonCliffTypes( &c_p0_p1, false );
      }
      if( lr > mid )
      {
        setPathingNonCliffTypes( &c_p1_p0, false );
        setPathingNonCliffTypes( &c_p1_p1, false );
        setPathingNonCliffTypes( &c_p0_p0, false );
      }
      if( ll > mid )
      {
        setPathingNonCliffTypes( &c_p0_p0, false );
        setPathingNonCliffTypes( &c_p1_p0, false );
        setPathingNonCliffTypes( &c_p0_p1, false );
      }
      if( ul > mid )
      {
        setPathingNonCliffTypes( &c_p0_p1, false );
        setPathingNonCliffTypes( &c_p1_p1, false );
        setPathingNonCliffTypes( &c_p0_p0, false );
      }

      // repeat for a difference of 2 to determine
      // cliff-walker unpathable cells

      // if a corner is lower than the mid, its unpathable
      if( ur < mid - 1 ) { setPathingCliffTypes( &c_p1_p1, false ); }
      if( lr < mid - 1 ) { setPathingCliffTypes( &c_p1_p0, false ); }
      if( ll < mid - 1 ) { setPathingCliffTypes( &c_p0_p0, false ); }
      if( ul < mid - 1 ) { setPathingCliffTypes( &c_p0_p1, false ); }

      // if a corner is higher than the mid, it and its adjacent
      // corners are unpathable
      if( ur > mid + 1 )
      {
        setPathingCliffTypes( &c_p1_p1, false );
        setPathingCliffTypes( &c_p1_p0, false );
        setPathingCliffTypes( &c_p0_p1, false );
      }
      if( lr > mid + 1 )
      {
        setPathingCliffTypes( &c_p1_p0, false );
        setPathingCliffTypes( &c_p1_p1, false );
        setPathingCliffTypes( &c_p0_p0, false );
      }
      if( ll > mid + 1 )
      {
        setPathingCliffTypes( &c_p0_p0, false );
        setPathingCliffTypes( &c_p1_p0, false );
        setPathingCliffTypes( &c_p0_p1, false );
      }
      if( ul > mid + 1 )
      {
        setPathingCliffTypes( &c_p0_p1, false );
        setPathingCliffTypes( &c_p1_p1, false );
        setPathingCliffTypes( &c_p0_p0, false );
      }
    }
  }
}




void SC2Map::processRamp( TiXmlElement* ramp )
{
  const char* dirStr     = ramp->Attribute( "dir"     );
  const char* leftLoStr  = ramp->Attribute( "leftLo"  );
  const char* rightLoStr = ramp->Attribute( "rightLo" );

  if( dirStr     == NULL ||
      leftLoStr  == NULL ||
      rightLoStr == NULL )
  {
    printWarning( "Unexpected ramp definition, ignoring.\n" );
    return;
  }


  int dir = atoi( dirStr );

  if( dir < 0 || dir > 7 )
  {
    printWarning( "Unexpected ramp data, ignoring.\n" );
    return;
  }

  point leftLo  = parseRampPoint( leftLoStr );
  point rightLo = parseRampPoint( rightLoStr );


  // s transitions from parsed ramp point to the
  // algorithm's start point (ramp point is at the
  // corner of 4 cells)
  int sx, sy;

  // u is unit vector pointing up the ramp
  int ux, uy;

  // v is a unit vector pointing along the ramp
  // from the leftLo point towards the rightLo point
  int vx, vy;

  // w is diagonally, between u and v (w switches bishop
  // colors, basically) needed for diagonal ramps
  int wx, wy;



  switch( dir )
  {
    // which direction are you walking when
    // going up the ramp?

    // south
    case 0:
      ux =  0; uy = -1;
      vx = -1; vy =  0;
      sx = -1; sy = -1;
    break;

    // north
    case 1:
      ux =  0; uy =  1;
      vx =  1; vy =  0;
      sx =  0; sy =  0;
    break;

    // west
    case 2:
      ux = -1; uy =  0;
      vx =  0; vy =  1;
      sx = -1; sy =  0;
    break;

    // east
    case 3:
      ux =  1; uy =  0;
      vx =  0; vy = -1;
      sx =  0; sy = -1;
    break;



    // south west
    case 4:
      ux = -1; uy = -1;
      vx = -1; vy =  1;
      wx = -1; wy =  0;
      sx = -1; sy = -1;
    break;

    // south east
    case 5:
      ux =  1; uy = -1;
      vx = -1; vy = -1;
      wx =  0; wy = -1;
      sx =  0; sy = -1;
    break;

    // north west
    case 6:
      ux = -1; uy =  1;
      vx =  1; vy =  1;
      wx =  0; wy =  1;
      sx = -1; sy =  0;
    break;

    // north east
    case 7:
      ux =  1; uy =  1;
      vx =  1; vy = -1;
      wx =  1; wy =  0;
      sx =  0; sy =  0;
    break;
  }


  // if the ramp algorithm takes this many steps,
  // something has gone horribly wrong.
  int sanityCounter = 1000;

  // p0 is the "pivot point" that slides along
  // the width of the ramp until we're done
  point p0;
  p0.mSet( leftLo.mx , leftLo.my );
  p0.mcSet( p0.mcx + sx, p0.mcy + sy );

  // p1 jumps from p0 to fill out the ramp
  point p1;

  // when p0 equals p2, do other ramp edge and end
  point p2;
  p2.mSet( rightLo.mx , rightLo.my );
  p2.mcSet( p2.mcx + sx, p2.mcy + sy );


  // the straight ramp and diagonal ramp have similar algorithms, but
  // the "shape" of the pathing applied at each step is different, so
  // separate them by direction here
  if( dir < 4 )
  {
    // straight ramp algorithm

    // do the ramp edge
    p1.pcSet( p0.pcx - vx, p0.pcy - vy );
    setPathingNonCliffTypes( &p1, false );

    p1.pcSet( p1.pcx - ux, p1.pcy - uy );
    setPathingNonCliffTypes( &p1, false );

    while( (p0.pcx != p2.pcx || p0.pcy != p2.pcy) &&
           sanityCounter > 0 )
    {
      setPathingAllTypes      ( &p0, true  );
      setPathingBuildableTypes( &p0, false );

      p1.pcSet( p0.pcx + ux, p0.pcy + uy );
      setPathingAllTypes      ( &p1, true  );
      //setPathingBuildableTypes( &p1, false ); <-- DON'T DO THIS

      p1.pcSet( p0.pcx - ux, p0.pcy - uy );
      setPathingAllTypes      ( &p1, true  );
      setPathingBuildableTypes( &p1, false );

      p1.pcSet( p0.pcx - 2*ux, p0.pcy - 2*uy );
      setPathingAllTypes      ( &p1, true  );
      setPathingBuildableTypes( &p1, false );

      p0.pcSet( p0.pcx + vx, p0.pcy + vy );

      --sanityCounter;
    }

    // do the ramp edge
    setPathingNonCliffTypes( &p0, false );

    p1.pcSet( p0.pcx - ux, p0.pcy - uy );
    setPathingNonCliffTypes( &p1, false );

    return;
  }

  // otherwise, diagonal ramp algorithm


  // do the ramp edge
  p1.pcSet( p0.pcx - wx, p0.pcy - wy );
  setPathingNonCliffTypes( &p1, false );

  p1.pcSet( p1.pcx - ux, p1.pcy - uy );
  setPathingNonCliffTypes( &p1, false );

  p1.pcSet( p0.pcx - 2*wx, p0.pcy - 2*wy );
  setPathingNonCliffTypes( &p1, false );

  p1.pcSet( p1.pcx + ux, p1.pcy + uy );
  setPathingNonCliffTypes( &p1, false );


  setPathingAllTypes      ( &p0, true  );
  setPathingBuildableTypes( &p0, false );

  p1.pcSet( p0.pcx + ux, p0.pcy + uy );
  setPathingAllTypes      ( &p1, true  );
  setPathingBuildableTypes( &p1, false );

  p1.pcSet( p0.pcx - ux, p0.pcy - uy );
  setPathingAllTypes      ( &p1, true  );
  setPathingBuildableTypes( &p1, false );

  p1.pcSet( p0.pcx - 2*ux, p0.pcy - 2*uy );
  setPathingAllTypes      ( &p1, true  );
  setPathingBuildableTypes( &p1, false );

  p0.pcSet( p0.pcx + wx - ux, p0.pcy + wy - uy );

  setPathingAllTypes      ( &p0, true  );
  setPathingBuildableTypes( &p0, false );

  p1.pcSet( p0.pcx + ux, p0.pcy + uy );
  setPathingAllTypes      ( &p1, true  );
  setPathingBuildableTypes( &p1, false );

  p1.pcSet( p0.pcx + 2*ux, p0.pcy + 2*uy );
  setPathingAllTypes      ( &p1, true  );
  setPathingBuildableTypes( &p1, false );

  p1.pcSet( p0.pcx - ux, p0.pcy - uy );
  setPathingAllTypes      ( &p1, true  );
  setPathingBuildableTypes( &p1, false );

  p1.pcSet( p0.pcx - 2*ux, p0.pcy - 2*uy );
  setPathingAllTypes      ( &p1, true  );
  setPathingBuildableTypes( &p1, false );

  p0.pcSet( p0.pcx + wx, p0.pcy + wy );


  while( (p0.pcx != p2.pcx || p0.pcy != p2.pcy) &&
         sanityCounter > 0 )
  {
    setPathingAllTypes      ( &p0, true  );
    setPathingBuildableTypes( &p0, false );

    p1.pcSet( p0.pcx + ux, p0.pcy + uy );
    setPathingAllTypes      ( &p1, true  );
    setPathingBuildableTypes( &p1, false );

    p1.pcSet( p0.pcx - ux, p0.pcy - uy );
    setPathingAllTypes      ( &p1, true  );
    setPathingBuildableTypes( &p1, false );

    p1.pcSet( p0.pcx - 2*ux, p0.pcy - 2*uy );
    setPathingAllTypes      ( &p1, true  );
    setPathingBuildableTypes( &p1, false );

    p0.pcSet( p0.pcx + wx - ux, p0.pcy + wy - uy );

    setPathingAllTypes      ( &p0, true  );
    setPathingBuildableTypes( &p0, false );

    p1.pcSet( p0.pcx + ux, p0.pcy + uy );
    setPathingAllTypes      ( &p1, true  );
    setPathingBuildableTypes( &p1, false );

    p1.pcSet( p0.pcx + 2*ux, p0.pcy + 2*uy );
    setPathingAllTypes      ( &p1, true  );
    setPathingBuildableTypes( &p1, false );

    p1.pcSet( p0.pcx - ux, p0.pcy - uy );
    setPathingAllTypes      ( &p1, true  );
    setPathingBuildableTypes( &p1, false );

    p1.pcSet( p0.pcx - 2*ux, p0.pcy - 2*uy );
    setPathingAllTypes      ( &p1, true  );
    setPathingBuildableTypes( &p1, false );

    p0.pcSet( p0.pcx + wx, p0.pcy + wy );

    --sanityCounter;
  }


  if( sanityCounter == 0 )
  {
    printError( "Ramp algorithm failed.\n" );
    exit( -1 );
  }


  setPathingAllTypes      ( &p0, true  );
  setPathingBuildableTypes( &p0, false );

  p1.pcSet( p0.pcx + ux, p0.pcy + uy );
  setPathingAllTypes      ( &p1, true  );
  setPathingBuildableTypes( &p1, false );

  p1.pcSet( p0.pcx - ux, p0.pcy - uy );
  setPathingAllTypes      ( &p1, true  );
  setPathingBuildableTypes( &p1, false );

  p1.pcSet( p0.pcx - 2*ux, p0.pcy - 2*uy );
  setPathingAllTypes      ( &p1, true  );
  setPathingBuildableTypes( &p1, false );

  p0.pcSet( p0.pcx + wx, p0.pcy + wy );

  p1.pcSet( p0.pcx - ux, p0.pcy - uy );
  setPathingNonCliffTypes( &p1, false );

  p1.pcSet( p1.pcx - ux, p1.pcy - uy );
  setPathingNonCliffTypes( &p1, false );

  p1.pcSet( p1.pcx + wx, p1.pcy + wy );
  setPathingNonCliffTypes( &p1, false );

  p1.pcSet( p1.pcx + ux, p1.pcy + uy );
  setPathingNonCliffTypes( &p1, false );
}


point SC2Map::parseRampPoint( const char* str )
{
  string s( str );
  string::size_type loc1 = s.find( "c=(", 0 );
  string::size_type loc2 = s.find( ") w", 0 );
  string rp = s.substr( loc1 + 3, loc2 - loc1 );

  float x;
  float y;
  sscanf( rp.data(), "%e, %e", &x, &y );

  // make sure that ramp points snap to the
  // terrain point EXACTLY!  (49.9999 should be 50.0, etc)
  x = snapIfNearInt( x );
  y = snapIfNearInt( y );

  point p;
  p.mSet( x, y );
  return p;
}
