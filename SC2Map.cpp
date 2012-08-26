#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "pngwriter.h"

#include "outstreams.hpp"
#include "coordinates.hpp"
#include "SC2Map.hpp"


// Note that this set is static, there
// is only one for the execution
set<string> SC2Map::mapFilenamesUsed;



SC2Map::SC2Map( string toolPathIn,
                string outputPathIn,
                string argPathIn,
                string archiveNameIn,
                string archiveWithExtIn )
{
  toolPath       = toolPathIn;
  outputPath     = outputPathIn;
  argPath        = argPathIn;
  archiveName    = archiveNameIn;
  archiveWithExt = archiveWithExtIn;

  mapHeight       = NULL;
  mapCliffChanges = NULL;
  mapPathing      = NULL;
  mapOpenness     = NULL;
  mapOpennessPrev = NULL;
  mapPathNodes    = NULL;

  totalMinerals     = 0.0f;
  totalVespeneGas   = 0.0f;
  totalHYMinerals   = 0.0f;
  totalHYVespeneGas = 0.0f;

  // the image parameters will be changed for each
  // requested image to render
  img = new pngwriter( 0, 0, 0.0, "dummy.png" );
}


SC2Map::~SC2Map()
{
  delete img;

  for( list<Resource*>::const_iterator itr = resources.begin();
       itr != resources.end();
       ++itr )
  {
    delete *itr;
  }

  for( list<Base*>::const_iterator itr = bases.begin();
       itr != bases.end();
       ++itr )
  {
    Base* b = *itr;

    for( list<StartLoc*>::const_iterator slItr = startLocs.begin();
         slItr != startLocs.end();
         ++slItr )
    {
      StartLoc* sl = *slItr;
      delete b->sl1vsl2_influence[sl];
    }

    delete b;
  }

  for( list<StartLoc*>::const_iterator itr = startLocs.begin();
       itr != startLocs.end();
       ++itr )
  {
    delete *itr;
  }

  for( list<Watchtower*>::const_iterator itr = watchtowers.begin();
       itr != watchtowers.end();
       ++itr )
  {
    delete *itr;
  }

  for( list<Destruct*>::const_iterator itr = destructs.begin();
       itr != destructs.end();
       ++itr )
  {
    delete *itr;
  }

  for( list<LoSB*>::const_iterator itr = losbs.begin();
       itr != losbs.end();
       ++itr )
  {
    delete *itr;
  }

  if( mapHeight )
  {
    delete mapHeight;
  }

  if( mapCliffChanges )
  {
    delete mapCliffChanges;
  }

  if( mapPathing )
  {
    delete mapPathing;
  }

  if( mapOpenness )
  {
    delete mapOpenness;
  }

  if( mapOpennessPrev )
  {
    delete mapOpennessPrev;
  }

  if( mapPathNodes )
  {
    delete mapPathNodes;
  }

  for( int t = 0; t < NUM_PATH_TYPES; ++t )
  {
    for( int i = 0; i < nodes[t].size(); ++i )
    {
      delete nodes[t][i];
    }

    for( map< int, vector<float>* >::iterator itr = d[t].begin();
         itr != d[t].end();
         ++itr )
    {
      delete (*itr).second;
    }

    for( map< int, vector<Node*>* >::iterator itr = pi[t].begin();
         itr != pi[t].end();
         ++itr )
    {
      delete (*itr).second;
    }
  }
}
