#ifndef ___SC2MapAggregator_hpp___
#define ___SC2MapAggregator_hpp___

#include <string>
#include <list>
using namespace std;

#include "sc2mapTypes.hpp"
#include "SC2Map.hpp"


#define MAXSTRLEN 128


struct SC2MapSummary
{
  char  archiveName[FILENAME_LENGTH];
  char  mapName    [FILENAME_LENGTH];
  char  fileName   [FILENAME_LENGTH];

  int   cellWidth;
  int   cellHeight;
  char  playableSize[MAXSTRLEN];

  float percentPlayableCellsPathable;
  //int numCellsGroundConnectedToMains; // --> classify as island?

  float totalMinerals;
  float totalVespeneGas;
  float percentMineralsHY;
  float percentVespeneGasHY;

  int   numStartLocs;
  int   numBasesIncludingSL;
  float avgResourcesPerBase;


  float minGroundDistanceMain2Main;
  float minCWalkDistanceMain2Main;
  float minAirDistanceMain2Main;

  float avgGroundDistanceMain2Main;
  float avgCWalkDistanceMain2Main;
  float avgAirDistanceMain2Main;

  float minGroundDistanceNat2Nat;
  float minCWalkDistanceNat2Nat;
  float minAirDistanceNat2Nat;

  float avgGroundDistanceNat2Nat;
  float avgCWalkDistanceNat2Nat;
  float avgAirDistanceNat2Nat;

  // something about min and max disparities here--when the
  // ground/CWalk/fly distances are very different, how different
  // are they as a percentage?  what is the minimum disparity?

  // the minimum of this value over starting locations
  float positionalBalancePercentage;

  //float averageMainSize;


  float maxOpenness;
  float averageOpennessPathableCells;
  //float averageOpennessMainReachableCells;
  //float averageOpennessAllMain2MainPaths;
  // with rocks, and without?

  float watchtowerCoverage;

  float impactOfDestructibleRocksPercentage; // how much do rocks change distances? 0 -> 100%


};


enum ColumnType
{
  COLTYPE_STR,
  COLTYPE_INT,
  COLTYPE_FLOAT,
};

struct Column
{
  char title[MAXSTRLEN];
  char format[MAXSTRLEN];
  int offsetToValue;
  ColumnType type;
};


class SC2MapAggregator
{
public:

  void buildOutputColumns();

  void addColumn( const char* title,
                  const char* format,
                  int         offsetToValue,
                  ColumnType  type );

  void aggregate( SC2Map* sc2map );

  void writeToCSV( string* outputPath );

protected:

  list<SC2MapSummary*> summaries;
  list<Column*>        outputColumns;
};


#endif // ___SC2MapAggregator_hpp___
