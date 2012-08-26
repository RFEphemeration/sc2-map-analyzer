#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "outstreams.hpp"
#include "coordinates.hpp"
#include "SC2Map.hpp"
#include "SC2MapAggregator.hpp"


void SC2MapAggregator::buildOutputColumns()
{
  addColumn( "Archive",                 "%s",     offsetof( SC2MapSummary, archiveName         ), COLTYPE_STR );
  addColumn( "Name In Output Files",    "%s",     offsetof( SC2MapSummary, fileName            ), COLTYPE_STR );
  addColumn( "Map",                     "%s",     offsetof( SC2MapSummary, mapName             ), COLTYPE_STR );
  addColumn( "Playable Size",           "%s",     offsetof( SC2MapSummary, playableSize        ), COLTYPE_STR );
  addColumn( "% Pathable",              "%.0f%%", offsetof( SC2MapSummary, percentPlayableCellsPathable ), COLTYPE_FLOAT );
  addColumn( "Num Start Locs",          "%d",     offsetof( SC2MapSummary, numStartLocs        ), COLTYPE_INT );
  addColumn( "Num Bases (inc. StrtLc)", "%d",     offsetof( SC2MapSummary, numBasesIncludingSL ), COLTYPE_INT );
  addColumn( "Avg Resources/Base",      "%.1fK",  offsetof( SC2MapSummary, avgResourcesPerBase ), COLTYPE_FLOAT );
  addColumn( "Total Minerals",          "%.1fK",  offsetof( SC2MapSummary, totalMinerals       ), COLTYPE_FLOAT );
  addColumn( "% Min HY",                "%.0f%%", offsetof( SC2MapSummary, percentMineralsHY   ), COLTYPE_FLOAT );
  addColumn( "Total Vespene Gas",       "%.1fK",  offsetof( SC2MapSummary, totalVespeneGas     ), COLTYPE_FLOAT );
  addColumn( "% Gas HY",                "%.0f%%", offsetof( SC2MapSummary, percentVespeneGasHY ), COLTYPE_FLOAT );

  addColumn( "Min Ground Distance Main-to-Main",     "%.1f", offsetof( SC2MapSummary, minGroundDistanceMain2Main ), COLTYPE_FLOAT );
  addColumn( "Avg Ground Distance Main-to-Main",     "%.1f", offsetof( SC2MapSummary, avgGroundDistanceMain2Main ), COLTYPE_FLOAT );
  addColumn( "Min Cliff-Walk Distance Main-to-Main", "%.1f", offsetof( SC2MapSummary, minCWalkDistanceMain2Main  ), COLTYPE_FLOAT );
  addColumn( "Avg Cliff-Walk Distance Main-to-Main", "%.1f", offsetof( SC2MapSummary, avgCWalkDistanceMain2Main  ), COLTYPE_FLOAT );
  addColumn( "Min Air Distance Main-to-Main",        "%.1f", offsetof( SC2MapSummary, minAirDistanceMain2Main    ), COLTYPE_FLOAT );
  addColumn( "Avg Air Distance Main-to-Main",        "%.1f", offsetof( SC2MapSummary, avgAirDistanceMain2Main    ), COLTYPE_FLOAT );

  addColumn( "Min Ground Distance Nat-to-Nat",     "%.1f", offsetof( SC2MapSummary, minGroundDistanceNat2Nat ), COLTYPE_FLOAT );
  addColumn( "Avg Ground Distance Nat-to-Nat",     "%.1f", offsetof( SC2MapSummary, avgGroundDistanceNat2Nat ), COLTYPE_FLOAT );
  addColumn( "Min Cliff-Walk Distance Nat-to-Nat", "%.1f", offsetof( SC2MapSummary, minCWalkDistanceNat2Nat  ), COLTYPE_FLOAT );
  addColumn( "Avg Cliff-Walk Distance Nat-to-Nat", "%.1f", offsetof( SC2MapSummary, avgCWalkDistanceNat2Nat  ), COLTYPE_FLOAT );
  addColumn( "Min Air Distance Nat-to-Nat",        "%.1f", offsetof( SC2MapSummary, minAirDistanceNat2Nat    ), COLTYPE_FLOAT );
  addColumn( "Avg Air Distance Nat-to-Nat",        "%.1f", offsetof( SC2MapSummary, avgAirDistanceNat2Nat    ), COLTYPE_FLOAT );

  addColumn( "Max Openness", "%.2f", offsetof( SC2MapSummary, maxOpenness ), COLTYPE_FLOAT );
  addColumn( "Avg Openness", "%.2f", offsetof( SC2MapSummary, averageOpennessPathableCells ), COLTYPE_FLOAT );

  addColumn( "Watchtower Coverage of Pathable Cells", "%.1f%%", offsetof( SC2MapSummary, watchtowerCoverage ), COLTYPE_FLOAT );

  addColumn( "% Positional Balance", "%.1f%%", offsetof( SC2MapSummary, positionalBalancePercentage ), COLTYPE_FLOAT );

  //addColumn( "", "", offsetof( SC2MapSummary,  ), COLTYPE_ );
}


void SC2MapAggregator::addColumn( const char* title,
                                  const char* format,
                                  int         offsetToValue,
                                  ColumnType  type )
{
  Column* c = new Column;
  strncpy( c->title,  title,  MAXSTRLEN );
  strncpy( c->format, format, MAXSTRLEN );
  c->offsetToValue = offsetToValue;
  c->type = type;
  outputColumns.push_back( c );
}


void SC2MapAggregator::aggregate( SC2Map* sc2map )
{
  SC2MapSummary* ms = new SC2MapSummary();

  strcpy( ms->archiveName, sc2map->archiveWithExt.data()       );
  strcpy( ms->mapName,     sc2map->mapName.data()              );
  strcpy( ms->fileName,    sc2map->mapNameInOutputFiles.data() );

  ms->cellWidth  = sc2map->cxDimPlayable;
  ms->cellHeight = sc2map->cyDimPlayable;

  sprintf( ms->playableSize,
           "%dx%d",
           sc2map->cxDimPlayable,
           sc2map->cyDimPlayable );

  // stuff to calculate over the cells of the map
  float numPlayableCells = ((float)sc2map->cxDimPlayable) *
                           ((float)sc2map->cyDimPlayable);
  float numCellsPathable = 0.0f;


  ms->percentPlayableCellsPathable = 100.0f * numCellsPathable / numPlayableCells;


  ms->maxOpenness                  = sc2map->opennessMax[PATH_GROUND_WITHROCKS];
  ms->averageOpennessPathableCells = sc2map->opennessAvg[PATH_GROUND_WITHROCKS];

  ms->watchtowerCoverage           = sc2map->watchtowerCoverage * 100.0f;


  // do the resources in kilos
  ms->totalMinerals       = sc2map->totalMinerals   / 1000.0f;
  ms->totalVespeneGas     = sc2map->totalVespeneGas / 1000.0f;
  ms->percentMineralsHY   = 100.0f * sc2map->totalHYMinerals   / sc2map->totalMinerals;
  ms->percentVespeneGasHY = 100.0f * sc2map->totalHYVespeneGas / sc2map->totalVespeneGas;


  ms->numStartLocs        = sc2map->startLocs.size();
  ms->numBasesIncludingSL = sc2map->bases.size();

  ms->avgResourcesPerBase = (sc2map->totalMinerals + sc2map->totalVespeneGas) /
                            ((float)sc2map->bases.size()) /
                            1000.0f;


  // the following stuff gets set as we iterate over
  // all the starting locations for info
  ms->minGroundDistanceMain2Main = infinity;
  ms->minCWalkDistanceMain2Main  = infinity;
  ms->minAirDistanceMain2Main    = infinity;
  float dGroundTotalMain2Main    = 0.0f;
  float dCWalkTotalMain2Main     = 0.0f;
  float dAirTotalMain2Main       = 0.0f;

  ms->minGroundDistanceNat2Nat   = infinity;
  ms->minCWalkDistanceNat2Nat    = infinity;
  ms->minAirDistanceNat2Nat      = infinity;
  float dGroundTotalNat2Nat      = 0.0f;
  float dCWalkTotalNat2Nat       = 0.0f;
  float dAirTotalNat2Nat         = 0.0f;


  // starts at 100% and drops as start locs are checked
  ms->positionalBalancePercentage = 100.0f;

  int startLocPairs = 0;

  for( list<StartLoc*>::const_iterator sl1Itr = sc2map->startLocs.begin();
       sl1Itr != sc2map->startLocs.end();
       ++sl1Itr )
  {
    StartLoc* sl1 = *sl1Itr;

    for( list<StartLoc*>::const_iterator sl2Itr = sc2map->startLocs.begin();
         sl2Itr != sc2map->startLocs.end();
         ++sl2Itr )
    {
      StartLoc* sl2 = *sl2Itr;

      if( sl1 == sl2 ) { continue; }

      ++startLocPairs;

      float dGroundMain2Main = sc2map->getShortestPathDistance( &(sl1->loc), &(sl2->loc), PATH_GROUND_WITHROCKS );
      float dCWalkMain2Main  = sc2map->getShortestPathDistance( &(sl1->loc), &(sl2->loc), PATH_CWALK_WITHROCKS  );
      float dAirMain2Main    = sc2map->getShortestAirDistance ( &(sl1->loc), &(sl2->loc) );

      if( dGroundMain2Main < ms->minGroundDistanceMain2Main )
      {
        ms->minGroundDistanceMain2Main = dGroundMain2Main;
      }
      if( dCWalkMain2Main < ms->minCWalkDistanceMain2Main )
      {
        ms->minCWalkDistanceMain2Main = dCWalkMain2Main;
      }
      if( dAirMain2Main < ms->minAirDistanceMain2Main )
      {
        ms->minAirDistanceMain2Main = dAirMain2Main;
      }

      dGroundTotalMain2Main += dGroundMain2Main;
      dCWalkTotalMain2Main  += dCWalkMain2Main;
      dAirTotalMain2Main    += dAirMain2Main;

      if( sl1->natBase != NULL && sl2->natBase != NULL )
      {
        float dGroundNat2Nat = sc2map->getShortestPathDistance( &(sl1->natBase->loc), &(sl2->natBase->loc), PATH_GROUND_WITHROCKS );
        float dCWalkNat2Nat  = sc2map->getShortestPathDistance( &(sl1->natBase->loc), &(sl2->natBase->loc), PATH_CWALK_WITHROCKS  );
        float dAirNat2Nat    = sc2map->getShortestAirDistance ( &(sl1->natBase->loc), &(sl2->natBase->loc) );

        if( dGroundNat2Nat < ms->minGroundDistanceNat2Nat )
        {
          ms->minGroundDistanceNat2Nat = dGroundNat2Nat;
        }
        if( dCWalkNat2Nat < ms->minCWalkDistanceNat2Nat )
        {
          ms->minCWalkDistanceNat2Nat = dCWalkNat2Nat;
        }
        if( dAirNat2Nat < ms->minAirDistanceNat2Nat )
        {
          ms->minAirDistanceNat2Nat = dAirNat2Nat;
        }

        dGroundTotalNat2Nat += dGroundNat2Nat;
        dCWalkTotalNat2Nat  += dCWalkNat2Nat;
        dAirTotalNat2Nat    += dAirNat2Nat;
      }

      if( sl1->sl2percentBalancedByResources[sl2] < ms->positionalBalancePercentage )
      {
        ms->positionalBalancePercentage = sl1->sl2percentBalancedByResources[sl2];
      }

      if( sl1->sl2percentBalancedByOpenness[sl2] < ms->positionalBalancePercentage )
      {
        ms->positionalBalancePercentage = sl1->sl2percentBalancedByOpenness[sl2];
      }
    }
  }

  ms->avgGroundDistanceMain2Main = dGroundTotalMain2Main / ((float)startLocPairs);
  ms->avgCWalkDistanceMain2Main  = dCWalkTotalMain2Main  / ((float)startLocPairs);
  ms->avgAirDistanceMain2Main    = dAirTotalMain2Main    / ((float)startLocPairs);

  ms->avgGroundDistanceNat2Nat   = dGroundTotalNat2Nat / ((float)startLocPairs);
  ms->avgCWalkDistanceNat2Nat    = dCWalkTotalNat2Nat  / ((float)startLocPairs);
  ms->avgAirDistanceNat2Nat      = dAirTotalNat2Nat    / ((float)startLocPairs);


  summaries.push_back( ms );
}


void SC2MapAggregator::writeToCSV( string* outputPath )
{
  string strOut( *outputPath );
  strOut += "\\aggregate-stats.csv";

  if( summaries.size() == 0 )
  {
    printWarning( "No map files were analyzed.\n" );
    return;
  }

  FILE* fileCSV = fopen( strOut.data(), "w" );

  if( fileCSV == NULL )
  {
    printError( "Could not open %s for writing.\n", strOut.data() );
    return;
  }

  printMessage( "Aggregate CSV is %s.\n\n", strOut.data() );


  // write out column headers
  int countColumns = 0;
  for( list<Column*>::const_iterator itr = outputColumns.begin();
       itr != outputColumns.end();
       ++itr )
  {
    Column* c = *itr;

    ++countColumns;
    if( countColumns < outputColumns.size() )
    {
      fprintf( fileCSV, "%s,",  c->title );
    } else {
      fprintf( fileCSV, "%s\n", c->title );
    }
  }

  for( list<SC2MapSummary*>::const_iterator itr = summaries.begin();
       itr != summaries.end();
       ++itr )
  {
    SC2MapSummary* ms = *itr;

    countColumns = 0;
    for( list<Column*>::const_iterator itr = outputColumns.begin();
         itr != outputColumns.end();
         ++itr )
    {
      Column* c = *itr;

      char format[MAXSTRLEN];

      ++countColumns;
      if( countColumns < outputColumns.size() )
      {
        sprintf( format, "%s,",  c->format );
      } else {
        sprintf( format, "%s\n", c->format );
      }

      switch( c->type )
      {
        case COLTYPE_STR:
        {
          char* ptr = (char*)((char*)ms + c->offsetToValue);
          fprintf( fileCSV, format, ptr );
        } break;

        case COLTYPE_INT:
        {
          int* ptr = (int*)((char*)ms + c->offsetToValue);
          fprintf( fileCSV, format, *ptr );
        } break;

        case COLTYPE_FLOAT:
        {
          float* ptr = (float*)((char*)ms + c->offsetToValue);
          fprintf( fileCSV, format, *ptr );
        } break;

        default:
          printError( "Unknown column type for summary CSV.\n" );
          return;
      }
    }
  }

  fclose( fileCSV );
}
