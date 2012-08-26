#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <string>
using namespace std;

#include "outstreams.hpp"
#include "coordinates.hpp"
#include "SC2Map.hpp"



void SC2Map::writeToCSV()
{
  string strOutCSV( this->outputPath );
  strOutCSV += "\\" + this->mapNameInOutputFiles + "-stats.csv";

  FILE* fileCSV = fopen( strOutCSV.data(), "w" );

  if( fileCSV == NULL )
  {
    printError( "Could not open %s for writing.\n", strOutCSV.data() );
    return;
  }


  // write out column headers
  fprintf( fileCSV, "Start Location," );

  for( list<StartLoc*>::const_iterator itr = startLocs.begin();
       itr != startLocs.end();
       ++itr )
  {
    StartLoc* sl = *itr;

    fprintf( fileCSV, "Ground distance to %s WITH ROCKS,",                   sl->name );
    //fprintf( fileCSV, "Cliff-walk distance to %s WITH ROCKS,",               sl->name );
    //fprintf( fileCSV, "Air distance to %s,",                                 sl->name );
    //fprintf( fileCSV, "Ground-CWalk Disparity %% to %s WITH ROCKS,",         sl->name );
    //fprintf( fileCSV, "Ground-Air Disparity %% to %s WITH ROCKS,",           sl->name );
    //fprintf( fileCSV, "Ground distance to %s NO ROCKS,",                     sl->name );
    //fprintf( fileCSV, "Cliff-walk distance to %s NO ROCKS,",                 sl->name );
    //fprintf( fileCSV, "%% Decrease Ground distance Remove Rocks to %s,",     sl->name );
    //fprintf( fileCSV, "%% Decrease Cliff-walk distance Remove Rocks to %s,", sl->name );
    fprintf( fileCSV, "Ground distance to %s Nat2Nat WITH ROCKS,",           sl->name );
    //fprintf( fileCSV, "Positional Balance %% by Resources vs %s,",           sl->name );
    //fprintf( fileCSV, "Positional Balance %% by Openness vs %s,",            sl->name );
    fprintf( fileCSV, "Ground distance to %s Third2Third WITH ROCKS,",           sl->name );
  }

  //fprintf( fileCSV, "Worst Positional Balance %%" );
  fprintf( fileCSV, "\n" );


  // write a line for each start location
  for( list<StartLoc*>::const_iterator itr1 = startLocs.begin();
       itr1 != startLocs.end();
       ++itr1 )
  {
    StartLoc* sl1 = *itr1;

    fprintf( fileCSV, "%s,", sl1->name );

    float worstPBalance = 200.0f;

    for( list<StartLoc*>::const_iterator itr2 = startLocs.begin();
         itr2 != startLocs.end();
         ++itr2 )
    {
      StartLoc* sl2 = *itr2;

      if( sl1 == sl2 )
      {
        // don't compare start loc to itself!
        //fprintf( fileCSV, "-,-,-,-,-,-,-,-,-,-,-,-," );
        fprintf( fileCSV, "-,-,-," );
        continue;
      }


      float dGroundWR = getShortestPathDistance( &(sl1->loc), &(sl2->loc), PATH_GROUND_WITHROCKS );
      //float dCWalkWR  = getShortestPathDistance( &(sl1->loc), &(sl2->loc), PATH_CWALK_WITHROCKS  );
      //float dAir      = getShortestAirDistance ( &(sl1->loc), &(sl2->loc) );

      //float Ground2CWalkDisparity = 100.0f * (1.0f - dCWalkWR/dGroundWR);
      //float Ground2AirDisparity   = 100.0f * (1.0f - dAir    /dGroundWR);


      //float dGroundNR = getShortestPathDistance( &(sl1->loc), &(sl2->loc), PATH_GROUND_NOROCKS );
      //float dCWalkNR  = getShortestPathDistance( &(sl1->loc), &(sl2->loc), PATH_CWALK_NOROCKS  );

      //float GroundDecreaseRemRocks = 100.0f * (1.0f - dGroundNR/dGroundWR);
      //float CWalkDecreaseRemRocks  = 100.0f * (1.0f - dCWalkNR /dCWalkWR);

      float dNat2Nat = 0.0f;
      if( sl1->natBase != NULL && sl2->natBase != NULL )
      {
        dNat2Nat = getShortestPathDistance( &(sl1->natBase->loc), &(sl2->natBase->loc), PATH_GROUND_WITHROCKS );
      }

      float dThird2Third = 0.0f;
      if( sl1->thirdBase != NULL && sl2->thirdBase != NULL )
      {
        dThird2Third = getShortestPathDistance( &(sl1->thirdBase->loc), &(sl2->thirdBase->loc), PATH_GROUND_WITHROCKS );
      }

      fprintf( fileCSV, "%.1f,", dGroundWR              );
      //fprintf( fileCSV, "%.1f,", dCWalkWR               );
      //fprintf( fileCSV, "%.1f,", dAir                   );
      //fprintf( fileCSV, "%.1f,", Ground2CWalkDisparity  );
      //fprintf( fileCSV, "%.1f,", Ground2AirDisparity    );
      //fprintf( fileCSV, "%.1f,", dGroundNR              );
      //fprintf( fileCSV, "%.1f,", dCWalkNR               );
      //fprintf( fileCSV, "%.1f,", GroundDecreaseRemRocks );
      //fprintf( fileCSV, "%.1f,", CWalkDecreaseRemRocks  );
      fprintf( fileCSV, "%.1f,", dNat2Nat               );
      //fprintf( fileCSV, "%.1f,", sl1->sl2percentBalancedByResources[sl2] );
      //fprintf( fileCSV, "%.1f,", sl1->sl2percentBalancedByOpenness[sl2] );
      fprintf( fileCSV, "%.1f,", dThird2Third               );

      /*
      if( sl1->sl2percentBalancedByResources[sl2] < worstPBalance )
      {
        worstPBalance = sl1->sl2percentBalancedByResources[sl2];
      }

      if( sl1->sl2percentBalancedByOpenness[sl2] < worstPBalance )
      {
        worstPBalance = sl1->sl2percentBalancedByOpenness[sl2];
      }
      */
    }

    //fprintf( fileCSV, "%f", worstPBalance );
    fprintf( fileCSV, "\n" );
  }

  fclose( fileCSV );
}
