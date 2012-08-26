#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "outstreams.hpp"
#include "coordinates.hpp"
#include "sc2mapTypes.hpp"
#include "SC2Map.hpp"




void SC2Map::identifyBases() {
  nameStartLocs();
  locateBases();
  assignNearestPathNodes();
}


void SC2Map::analyzeBases() {
  calculateWatchtowerCoverage();
  calculateAverageOpennessPerBase();
  calculateInfluence();
  assignMainNatThirdIslands();
  computeSpaceInMain();
  calculatePositionalBalance();
}



static PathType pathTypeLocateBases  = PATH_GROUND_WITHROCKS_NORESOURCES;
static PathType pathTypeLocateChokes = PATH_GROUND_WITHROCKS;



void SC2Map::locateBases() {

  for( list<Resource*>::const_iterator itr = resources.begin();
       itr != resources.end();
       ++itr )
  {
    Resource* r = *itr;

    Base* b;
    if( !getNearestBase( r, &b ) )
    {
      // skip resources placed on unpathable areas
      continue;
    }

    if( b == NULL )
    {
      // start a new base for this resource
      b = new Base();
      b->loc.set( &(r->loc) );

      b->isInMain = false;
      
      b->resources.push_back( r );
      addToBaseTotals( b, r );
      addToMapTotals( r );

      this->bases.push_back( b );

    } else {
      // there is already a base for this
      // resource, add it and calculate new
      // position for the base
      b->resources.push_back( r );
      addToBaseTotals( b, r );
      addToMapTotals( r );

      float mxTotal = 0;
      float myTotal = 0;
      for( list<Resource*>::const_iterator itr = b->resources.begin();
           itr != b->resources.end();
           ++itr )
      {
        mxTotal += (*itr)->loc.mx;
        myTotal += (*itr)->loc.my;
      }

      mxTotal /= b->resources.size();
      myTotal /= b->resources.size();

      b->loc.mSet( mxTotal, myTotal );
    }
  }
}



void SC2Map::addToBaseTotals( Base* b, Resource* r ) {
  b->resourceTotal = r->amount;

  switch( r->type )
  {
    case MINERALS:
      b->totalRegMinerals += r->amount;
    break;

    case MINERALS_HY:
      b->totalHYMinerals += r->amount;
    break;

    case VESPENEGAS:
      b->totalRegVespeneGas += r->amount;
    break;

    case VESPENEGAS_HY:
      b->totalHYVespeneGas += r->amount;
    break;

    default:
      printError( "A placed resource has an unknown type!" );
      exit( -1 );
  }
}


void SC2Map::addToMapTotals( Resource* r ) {
  switch( r->type )
  {
    case MINERALS:
      totalMinerals += r->amount;
    break;

    case MINERALS_HY:
      totalMinerals   += r->amount;
      totalHYMinerals += r->amount;
    break;

    case VESPENEGAS:
      totalVespeneGas += r->amount;
    break;

    case VESPENEGAS_HY:
      totalVespeneGas   += r->amount;
      totalHYVespeneGas += r->amount;
    break;

    default:
      printError( "A placed resource has an unknown type!" );
      exit( -1 );
  }
}


// if the resource is over unpathable cells, say as a decoration
// instead of a real resource, and there is no path node for it,
// return FALSE and ignore it in the identification of bases
bool SC2Map::getNearestBase( Resource* r, Base** bOut ) {

  Node* nResource = getPathNode( &(r->loc), pathTypeLocateBases );

  if( nResource == NULL )
  {
    printWarning( "Resource at %f, %f is in an unpathable cell.\n",
                  r->loc.mx, r->loc.my );
    return false;
  }

  Base* b = NULL;
  float d = infinity;

  for( list<Base*>::iterator itr = bases.begin();
       itr != bases.end();
       ++itr )
  {
    // we have to do Node->Base shortest path the Node->Node way here because we
    // cannot use the nicer Node->Base until after this algorithm finds the
    // final base position!!  if the base does not have a direct pathing node,
    // then we need a suitable substitute-->a resource within the base.
    Base* bConsider = *itr;
    Node* nBase     = getPathNode( &(bConsider->loc), pathTypeLocateBases );

    if( nBase == NULL )
    {
      if( bConsider->resources.size() == 0 )
      {
        printError( "A potential base with zero resources?\n" );
        exit( -1 );
      }

      // get node for first resource in list belonging to this base
      nBase = getPathNode( &((*(bConsider->resources.begin()))->loc),
                           pathTypeLocateBases );

      if( nBase == NULL )
      {
        printError( "A resource without a valid location?\n" );
        exit( -1 );
      }
    }

    float dConsider = getShortestPathDistance( nResource, nBase, pathTypeLocateBases );

    // 25 units is as far as a resource in a base should be from
    // any other resource that belongs to the same base
    if( dConsider < d && dConsider < 25.0f )
    {
      b = bConsider;
      d = dConsider;
    }
  }

  *bOut = b;
  return true;
}


// base's location might be over unpathable area (like
// if there are rocks blocking the base) so let's "patch"
// distance calculations by building a mapping of nearby
// path nodes to the distance away they are to the base's
// true position
void SC2Map::assignNearestPathNodes() {

  for( list<Base*>::const_iterator bItr = bases.begin();
       bItr != bases.end();
       ++bItr )
  {
    Base* b = *bItr;

    for( int t = 0; t < NUM_PATH_TYPES; ++t )
    {
      // check to see if the base loc is already in
      // a pathable node
      Node* n = getPathNode( &(b->loc), (PathType)t );
      if( n != NULL )
      {
        // if so, put one entry in the patch and exit
        // early
        (b->node2patchDistance[t])[n] =
          getShortestAirDistance( &(n->loc), &(b->loc) );
        continue;
      }

      // if over something non-pathable, then we should
      // "cast" a ray in 8 directions, find nearest
      // pathable cell within a limit, mark distance away
      int dxs[] = { 1,  1,  0, -1, -1, -1, 0, 1 };
      int dys[] = { 0, -1, -1, -1,  0,  1, 1, 1 };

      int numDirs        = 8;
      int numCellsToCast = 10;

      // make sure we find at least one entry...
      bool foundOne = false;

      for( int i = 0; i < numDirs; ++i )
      {
        // the nearest point to the base, which we already
        // know is not pathable, so start looking along dir
        point c; c.set( &(b->loc) );

        for( int j = 0; j < numCellsToCast; ++j )
        {
          c.pcSet( c.pcx + dxs[i], c.pcy + dys[i] );

          n = getPathNode( &c, (PathType)t );
          if( n != NULL )
          {
            // if so, put an entry in the patch and we're done
            // looking in this direction
            (b->node2patchDistance[t])[n] =
              getShortestAirDistance( &(n->loc), &(b->loc) );

            foundOne = true;
            break;
          }
        }
      }

      if( !foundOne )
      {
        printError( "Cannot find any pathable cells near base for path type %d.\n", t );
        exit( -1 );
      }
    }
  }
}


void SC2Map::nameStartLocs() {

  // atan2 returns an angle in radians in range [-pi, pi]
  // so let's cut the range into 13 parts, where we will find
  // half of 9 o'clock at the bottom, and half of it at the top.
  float upperBounds[13];

  // each wedge of clock is 1/12 of 2*pi, or pi/6
  float wedge = M_PI / 6.0f;

  // the upper bound of 9 o'clock's BOTTOM half is half a wedge
  // over the bottom of the range
  float bound = -M_PI + 0.5f*wedge;

  for( int i = 0; i < 13; ++i )
  {
    upperBounds[i] = bound;
    bound += wedge;
  }

  // start at bottom of range, go positive radians around clock
  char wedgeNames[][16] =
  {
    "9 o'clock",
    "8 o'clock",
    "7 o'clock",
    "6 o'clock",
    "5 o'clock",
    "4 o'clock",
    "3 o'clock",
    "2 o'clock",
    "1 o'clock",
    "12 o'clock",
    "11 o'clock",
    "10 o'clock",
    "9 o'clock",
  };

  char wedgeNumbers[13] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 12, 11, 10, 9 };

  // Try to use this scheme unless two start locations have
  // the same number--then give up and just number them
  bool clockNumUsed[13] = { false };
  bool reusedNum        = false;

  // set a point at the playable terrain's center
  // to calculate the angle to a start location
  point center;
  center.ptSet( txDimPlayable/2,
                tyDimPlayable/2 );

  for( list<StartLoc*>::const_iterator itr = startLocs.begin();
       itr != startLocs.end();
       ++itr )
  {
    StartLoc* sl = *itr;

    // TODO alter angle for aspect ratio?
    float ang = atan2( sl->loc.my - center.my,
                       sl->loc.mx - center.mx );

    for( int i = 0; i < 13; ++i )
    {
      if( ang < upperBounds[i] )
      {
        // find out if the clock wedge is already used,
        if( clockNumUsed[i] ) { reusedNum = true; }

        // if we are at 9, mark BOTH halves as used
        if( i == 0 || i == 12 )
        {
          clockNumUsed[0]  = true;
          clockNumUsed[12] = true;
        } else {
          clockNumUsed[i]  = true;
        }

        strcpy( sl->name, wedgeNames[i] );
        sl->idNum = wedgeNumbers[i];

        // done looking for a wedge for this start loc
        break;
      }
    }
  }

  if( reusedNum )
  {
    // that scheme didn't work out, just number them
    int n = 1;

    for( list<StartLoc*>::const_iterator itr = startLocs.begin();
         itr != startLocs.end();
         ++itr )
    {
      StartLoc* sl = *itr;
      sprintf( sl->name, "%d", n );
      sl->idNum = n;
      ++n;
    }
  }
}


void SC2Map::calculateAverageOpennessPerBase() {

  for( list<Base*>::const_iterator bItr = bases.begin();
       bItr != bases.end();
       ++bItr )
  {
    Base* b = *bItr;

    b->avgOpennessForNeighborhood =
      calculateAverageOpennessInNeighborhood( &(b->loc),
                                              getfConstant( "baseAvgOpennessNeighborhoodRadius" ),
                                              PATH_GROUND_WITHROCKS );
  }
}


void SC2Map::calculateInfluence() {

  // THIS CALCULATION HAS CHANGED--influence is calculated
  // separately for EACH PAIR of start locations, so we
  // are considering all the spawn configurations

  // initialize this calc
  for( list<StartLoc*>::const_iterator slItr1 = startLocs.begin();
       slItr1 != startLocs.end();
       ++slItr1 )
  {
    StartLoc* sl1 = *slItr1;

    for( list<StartLoc*>::const_iterator slItr2 = startLocs.begin();
         slItr2 != startLocs.end();
         ++slItr2 )
    {
      StartLoc* sl2 = *slItr2;

      if( sl1 == sl2 ) { continue; }

      sl1->sl2resourceInfluence[sl2] = 0.0f;
      sl1->sl2opennessInfluence[sl2] = 0.0f;
    }
  }

  // how much influence, as a percentage, does each
  // start location IN A PAIR exert on a given base?
  for( list<Base*>::const_iterator bItr = bases.begin();
       bItr != bases.end();
       ++bItr )
  {
    Base* b = *bItr;

    for( list<StartLoc*>::const_iterator slItr1 = startLocs.begin();
         slItr1 != startLocs.end();
         ++slItr1 )
    {
      StartLoc* sl1 = *slItr1;

      map<StartLoc*, float>* innerMap = new map<StartLoc*, float>;

      float d1 = weightedInfluenceDistance( sl1, b, NULL );

      for( list<StartLoc*>::const_iterator slItr2 = startLocs.begin();
           slItr2 != startLocs.end();
           ++slItr2 )
      {
        StartLoc* sl2 = *slItr2;

        if( sl1 == sl2 ) { continue; }

        float d2 = weightedInfluenceDistance( sl2, b, NULL );

        float dTotal   = d1 + d2;
        float dAverage = 0.5f * dTotal;
        float pNeutral = 0.5f;

        float pInfl = pNeutral + ((dAverage - d1) / dTotal);

        innerMap->insert( make_pair( sl2, 100.0f * pInfl ) );
      }

      b->sl1vsl2_influence.insert( make_pair( sl1, innerMap ) );
    }
  }

  // calculate average influence for main/nat/third
  for( list<StartLoc*>::const_iterator slItr = startLocs.begin();
       slItr != startLocs.end();
       ++slItr )
  {
    StartLoc* sl = *slItr;

    for( list<Base*>::const_iterator bItr = bases.begin();
         bItr != bases.end();
         ++bItr )
    {
      Base* b = *bItr;

      b->sl2averageInfluence[sl] = averageInfluence( sl, b );
    }
  }
}


float SC2Map::weightedInfluenceDistance( StartLoc* sl, Base* b, point* p ) {

  if( (b == NULL && p == NULL) ||
      (b != NULL && p != NULL)    )
  {
    printError( "Only a base or point should be non-null." );
    exit( -1 );
  }

  // return a distance that is a weighted combination
  // of distances from this StartLoc to this Base
  float dwi = 0.0f;

  // when the distance to a base is infinite
  // like an island base by ground path, don't use
  // the infinity constant because it in the calculation
  // it completely obscures the non-infinite factors.
  // Instead, use an otherwise big number based on the
  // map's playable dimensions
  float dNoInfluence = (float) (txDimPlayable + tyDimPlayable);


  float dGround;
  if( b != NULL )
  {
    dGround = getShortestPathDistance( &(sl->loc), b, PATH_GROUND_WITHROCKS );
  } else {
    dGround = getShortestPathDistance( &(sl->loc), p, PATH_GROUND_WITHROCKS );
  }
  if( effectivelyInfinity( dGround ) )
  {
    dGround = dNoInfluence;
  }
  dwi += getfConstant( "influenceWeightGround" ) * dGround;

  float dCWalk;
  if( b != NULL )
  {
    dCWalk = getShortestPathDistance( &(sl->loc), b, PATH_CWALK_WITHROCKS );
  } else {
    dCWalk = getShortestPathDistance( &(sl->loc), p, PATH_CWALK_WITHROCKS );
  }
  if( SC2Map::effectivelyInfinity( dCWalk ) )
  {
    dCWalk = dNoInfluence;
  }
  dwi += getfConstant( "influenceWeightCWalk" ) * dCWalk;

  float dAir;
  if( b != NULL )
  {
    dAir = getShortestAirDistance( &(sl->loc), &(b->loc) );
  } else {
    dAir = getShortestAirDistance( &(sl->loc), p );
  }
  dwi += getfConstant( "influenceWeightAir" ) * dAir;

  return dwi;
}


float SC2Map::averageInfluence( StartLoc* sl, Base* b ) {

  // this calculation is dangerous when there are
  // less than two start locations! Watch for divide/0!
  if( startLocs.size() < 2 )
  {
    return 0.0f;
  }

  float total = 0.0f;
  map<StartLoc*, float>* innerMap = b->sl1vsl2_influence[sl];

  for( list<StartLoc*>::const_iterator slItr = startLocs.begin();
       slItr != startLocs.end();
       ++slItr )
  {
    StartLoc* sl2 = *slItr;

    if( sl == sl2 ) { continue; }

    total += (*innerMap)[sl2];
  }

  return total / (float)(startLocs.size() - 1);
}


void SC2Map::assignMainNatThirdIslands() {

  // find a start loc's main, nat and third
  // also calculate the list of expansions that are
  // an island relative to every start location

  // don't even consider a base as a main, nat or
  // third unless the start loc exerts an influence
  // greater than a threshold
  float minInfluence = getfConstant( "minInfluenceToConsiderNatOrThird" );

  
  // to know if a base is an island or semi island we have to
  // consider its relationship to each start location one at
  // a time.  The strategy is everything is an island and semi-island
  // until we have a path proving it ISN'T, then after looking
  // at all start locations we can mark a base as island or semi.
  set<Base*> islands;
  set<Base*> semiislands;
  for( list<Base*>::const_iterator bItr = bases.begin();
       bItr != bases.end();
       ++bItr ) {
       
    Base* b = *bItr;
    islands.insert( b );
    semiislands.insert( b );
    
    // also initialize other attributes we're about
    // to compute below
    b->sl      = NULL;
    b->numExpo = OTHER_EXPO;   
  }
  

  for( list<StartLoc*>::const_iterator slItr = startLocs.begin();
       slItr != startLocs.end();
       ++slItr ) {
       
    StartLoc* sl = *slItr;

    sl->mainBase  = NULL;
    sl->natBase   = NULL;
    sl->thirdBase = NULL;

    for( list<Base*>::const_iterator bItr = bases.begin();
         bItr != bases.end();
         ++bItr ) {
         
      Base* b = *bItr;
      
      
      // do island classifications
      float dGroundWR = getShortestPathDistance( &(sl->loc), b, PATH_GROUND_WITHROCKS );
      float dGroundNR = getShortestPathDistance( &(sl->loc), b, PATH_GROUND_NOROCKS   );

      if( !SC2Map::effectivelyInfinity( dGroundNR ) ) {
        // if there is a path with no rocks this base is NOT an island
        islands.erase( b );
      }

      if( !SC2Map::effectivelyInfinity( dGroundWR ) ) {
        // if there is a path with rocks this base is NOT a semi island
        semiislands.erase( b );
      }

      
      // now do main/nat/third because we might jump out of this iteration
      float bInfluence = b->sl2averageInfluence[sl];

      if( bInfluence < minInfluence ) {
        continue;
      }

      // should this base beat out the current third?
      if( sl->thirdBase == NULL ||
          sl->thirdBase->sl2averageInfluence[sl] < bInfluence )
      {
        sl->thirdBase = b;
        b->sl         = sl;
        b->numExpo    = THIRD_EXPO;
      } else {
        continue;
      }

      // should this base beat out the current nat?
      if( sl->natBase == NULL ||
          sl->natBase->sl2averageInfluence[sl] < bInfluence )
      {
        sl->thirdBase = sl->natBase;
        if( sl->thirdBase != NULL ) {
          sl->thirdBase->numExpo = THIRD_EXPO;
        }
        
        sl->natBase   = b;
        b->sl         = sl;
        b->numExpo    = NATURAL_EXPO;
      } else {
        continue;
      }

      // should this base beat out the current main?
      if( sl->mainBase == NULL ||
          sl->mainBase->sl2averageInfluence[sl] < bInfluence )
      {
        sl->natBase = sl->mainBase;
        if( sl->natBase != NULL ) {
          sl->natBase->numExpo = NATURAL_EXPO;
        }
        
        sl->mainBase = b;
        b->sl        = sl;
        b->numExpo   = MAIN_EXPO;
      }

    }
  }
  

  // now mark islands and semi islands
  for( list<Base*>::const_iterator bItr = bases.begin();
       bItr != bases.end();
       ++bItr ) {
       
    Base* b = *bItr;
    
    if( islands.find( b ) != islands.end() ) {
      b->islandType = ISLAND;
      
    } else if( semiislands.find( b ) != semiislands.end() ) {
      b->islandType = SEMI_ISLAND;
      
    } else {
      b->islandType = NOT_AN_ISLAND;
    }
  }
}


void SC2Map::calculatePositionalBalance() {

  for( list<StartLoc*>::const_iterator slItr1 = startLocs.begin();
       slItr1 != startLocs.end();
       ++slItr1 )
  {
    StartLoc* sl1 = *slItr1;

    for( list<StartLoc*>::const_iterator slItr2 = startLocs.begin();
         slItr2 != startLocs.end();
         ++slItr2 )
    {
      StartLoc* sl2 = *slItr2;

      if( sl1 == sl2 ) { continue; }

      for( list<Base*>::const_iterator bItr = bases.begin();
           bItr != bases.end();
           ++bItr )
      {
        Base* b = *bItr;

        map<StartLoc*, float>* innerMap1 = b->sl1vsl2_influence[sl1];
        map<StartLoc*, float>* innerMap2 = b->sl1vsl2_influence[sl2];

        float di = (*innerMap1)[sl2] - (*innerMap2)[sl1];

        if( di < 0.0f ) {
          di = 0.0f;
        }

        sl1->sl2resourceInfluence[sl2] += b->resourceTotal              * di;
        sl1->sl2opennessInfluence[sl2] += b->avgOpennessForNeighborhood * di;
      }
    }
  }

  for( list<StartLoc*>::const_iterator slItr1 = startLocs.begin();
       slItr1 != startLocs.end();
       ++slItr1 )
  {
    StartLoc* sl1 = *slItr1;

    for( list<StartLoc*>::const_iterator slItr2 = startLocs.begin();
         slItr2 != startLocs.end();
         ++slItr2 )
    {
      StartLoc* sl2 = *slItr2;

      if( sl1 == sl2 ) { continue; }

      //float r = sl1->sl2resourceInfluence[sl2] /
      //          (totalMinerals + totalVespeneGas);

      // multiply by 200 because we want locations to appear
      // balanced like this:  SL1 is 98% vs. SL2 at 102%
      //sl1->sl2percentBalancedByResources[sl2] = 200.0f * r;

      sl1->sl2percentBalancedByResources[sl2] = 100.0f *
                                                sl1->sl2resourceInfluence[sl2] /
                                                sl2->sl2resourceInfluence[sl1];

      // for openness, just get percent difference
      sl1->sl2percentBalancedByOpenness[sl2] = 100.0f *
                                               sl1->sl2opennessInfluence[sl2] /
                                               sl2->sl2opennessInfluence[sl1];
    }
  }
}




void SC2Map::computeSpaceInMain() {
  for( list<StartLoc*>::const_iterator itr = startLocs.begin();
       itr != startLocs.end();
       ++itr ) {
       
    StartLoc* sl = *itr;

    if( sl->mainChoke.mx < 0.0f ) {
      sl->spaceInMain = -1;
      continue;
    }
    
    
    // while we're computing the space in main, also find any bases
    // also in here, other than the starting resources
    list<Base*> possibleInMainBases;
    for( list<Base*>::const_iterator bItr = bases.begin();
       bItr != bases.end();
       ++bItr ) {
       
      Base* b = *bItr;
      
      if( b == sl->mainBase ) {
        continue;
      }
      
      possibleInMainBases.push_back( b );
    }
    

    map<int, point> fillSet;
    map<int, point> workSet;

    findSpace( 0, 0, &(sl->loc), &fillSet, &workSet, &(sl->mainChoke) );

    while( !workSet.empty() ) {
    
      map<int, point>::iterator itr = workSet.begin();
      point c = itr->second;
      workSet.erase( makePointKey( &c ) );

      fillSet[makePointKey( &c )] = c;

      // did we find a base?
      for( list<Base*>::const_iterator bItr = possibleInMainBases.begin();
       bItr != possibleInMainBases.end();
       ++bItr ) {
       
        Base* b = *bItr;
      
        if( p2pDistance( &c, &(b->loc) ) < getfConstant( "inMainBaseRadius" ) ) {
          b->isInMain = true;
        }
      }
      
      
      
      findSpace(  1,  0, &c, &fillSet, &workSet, &(sl->mainChoke) );
      findSpace(  0, -1, &c, &fillSet, &workSet, &(sl->mainChoke) );
      findSpace( -1,  0, &c, &fillSet, &workSet, &(sl->mainChoke) );
      findSpace(  0,  1, &c, &fillSet, &workSet, &(sl->mainChoke) );
    }

    sl->spaceInMain = fillSet.size();
  }
}


void SC2Map::findSpace( int dx, int dy, point* c,
                        map<int, point>* fillSet,
                        map<int, point>* workSet,
                        point* choke ) {
  point cn;
  cn.pcSet( c->pcx + dx, c->pcy + dy );

  if( !isPlayableCell( &cn ) )
  {
    return;
  }

  if( !getPathing( &cn, pathTypeLocateChokes ) )
  {
    return;
  }

  if( p2pDistance( &cn, choke ) < getfConstant( "spaceInMainChokeRadius" ) )
  {
    return;
  }

  int k = makePointKey( &cn );

  map<int, point>::iterator itr = fillSet->find( k );

  if( itr == fillSet->end() )
  {
    (*workSet)[k] = cn;
  }
}




void SC2Map::calculateWatchtowerCoverage() {

  set<int> cellsCovered;

  for( list<Watchtower*>::const_iterator wtItr = watchtowers.begin();
       wtItr != watchtowers.end();
       ++wtItr )
  {
    Watchtower* wt = *wtItr;

    int r = (int)(wt->range + 1.0f);

    for( int pci = -r; pci < r; ++pci )
    {
      for( int pcj = -r; pcj < r; ++pcj )
      {
        float xsq = (float)(pci*pci);
        float ysq = (float)(pcj*pcj);
        if( sqrt( xsq + ysq ) > wt->range )
        {
          continue;
        }

        point c;
        c.pcSet( wt->loc.pcx + pci,
                 wt->loc.pcy + pcj );

        if( isPlayableCell( &c ) &&
            getPathing( &c, PATH_GROUND_NOROCKS ) )
        {
          cellsCovered.insert( c.pcx + c.pcy*cxDimPlayable );
        }
      }
    }
  }

  watchtowerCoverage =
    (float)cellsCovered.size() /
    (float)numPathableCells[PATH_GROUND_NOROCKS];
}
