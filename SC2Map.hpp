#ifndef ___SC2Map_hpp___
#define ___SC2Map_hpp___

#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>
using namespace std;

#include "StormLib.h"
#include "tinyxml.h"
#include "pngwriter.h"

#include "sc2mapTypes.hpp"
#include "config.hpp"
#include "coordinates.hpp"
#include "PrioQueue.hpp"


class SC2Map
{

public:


  //////////////////////////////////////////////////
  // implemented in SC2Map.cpp
  //////////////////////////////////////////////////
  SC2Map( string toolPathIn,
          string outputPathIn,
          string argPathIn,
          string archiveNameIn,
          string archiveWithExtIn );
  ~SC2Map();


  //////////////////////////////////////////////////
  // implemented in config.cpp
  //////////////////////////////////////////////////
  int    getiConstant   ( string name );
  float  getfConstant   ( string name );
  Color* getColor       ( string name );
  bool   getOutputOption( string name );
  string getOutputPath();

  //////////////////////////////////////////////////
  // implemented in read.cpp
  //////////////////////////////////////////////////
  int readMap();


  //////////////////////////////////////////////////
  // implemented in bookkeeping.cpp
  //////////////////////////////////////////////////
  void countPathableCells();


  //////////////////////////////////////////////////
  // implemented in placedobjects.cpp
  //////////////////////////////////////////////////
  void buildResourceList();


  //////////////////////////////////////////////////
  // implemented in bases.cpp
  //////////////////////////////////////////////////
  void identifyBases();
  void analyzeBases();
  void computeSpaceInMain();


  //////////////////////////////////////////////////
  // implemented in openness.cpp
  //////////////////////////////////////////////////
  void computeOpenness();
  void locateChokes();
  

  //////////////////////////////////////////////////
  // implemented in dijkstra.cpp
  //////////////////////////////////////////////////
  void prepShortestPaths();


  //////////////////////////////////////////////////
  // implemented in spreadsheet.cpp
  //////////////////////////////////////////////////
  void writeToCSV();


  //////////////////////////////////////////////////
  // implemented in render.cpp
  //////////////////////////////////////////////////

  // do at program entry to verify beforehand the
  // PNG writing will work
  static int pngwriterTest();

  // one image
  void renderImageSummary();

  // one image, uses path type="ground with rocks"
  void renderImageOpenness();

  // two images for each start location pair,
  // one for paths between mains, one between nats
  void renderImagesShortestPaths();

  // one image per pair of start locations
  void renderImagesInfluence();

  // one image per pair of start locations
  void renderImagesInfluenceHeatMap();

  // one image, just terrain layer
  void renderImageTerrainOnly();

  // one image for each pathing type
  void renderImagesPathing();

  // one image, all bases with labels
  void renderImageBases();

  // for development
  void renderTestImages();



// TODO: UNDO THIS LATER
// really don't want to write all the accessors
// for the SC2MapAggregator right now, so make
// all the members of this class public
//protected:



  //////////////////////////////////////////////////
  // in SC2Map.cpp
  //////////////////////////////////////////////////
  string toolPath;
  string outputPath;
  string argPath;
  string archiveName;
  string archiveWithExt;

  Config configUserLocal;



  //////////////////////////////////////////////////
  // in read.cpp
  //////////////////////////////////////////////////
  int readArchiveFile( const HANDLE archive,
                       const char*  strFilename,
                       int*         bufferOutSize,
                       u8**         bufferOut );

  int verifyMagicWord( const u8* magic, const u8* buffer );

  int readGameStrings_txt    ( const HANDLE archive );
  int readMapInfo            ( const HANDLE archive );
  int readt3HeightMap        ( const HANDLE archive );
  int readt3Terrain_xml      ( const HANDLE archive );
  int readt3CellFlags        ( const HANDLE archive );
  int readPaintedPathingLayer( const HANDLE archive );
  int readObjects            ( const HANDLE archive );

  // put the raw string of the map in strMapName and put
  // a cleaned up version of that string in strMapFilename
  // for opening output files with valid filenames
  string mapName;
  string mapNameInOutputFiles;

  void makeMapNameValidForFilenames();

  // when opening several maps, warn when a map filename is reused
  static set<string> mapFilenamesUsed;


  // note that the dimensions of the total map
  // and playable area are not given in points
  // because it is counter-intuitive to access
  // something like tSizeMap.mtx.  Other
  // coordinates make sense as points.

  // total map dimensions in terrain units
  // and cell units
  int txDimMap;
  int tyDimMap;
  int cxDimMap;
  int cyDimMap;

  // playable area dimensions
  int txDimPlayable;
  int tyDimPlayable;
  int cxDimPlayable;
  int cyDimPlayable;


  // boundaries of playable area, note that
  // left, bottom is inclusive but right, top
  // is not
  point cLeftBottom;
  point cRightTop;

  point tLeftBottom;
  point tRightTop;


  inline bool isPlayableTerrain( point* t )
  {
    return
      t->ptx >= 0 && t->ptx < txDimPlayable &&
      t->pty >= 0 && t->pty < tyDimPlayable;
  }


  inline bool isPlayableCell   ( point* c )
  {
    return
      c->pcx >= 0 && c->pcx < cxDimPlayable &&
      c->pcy >= 0 && c->pcy < cyDimPlayable;
  }


  // the available cliff heights: 0 is unplayable
  // low ground and levels 1, 2, 3 may be in play
  u8*  mapHeight;
  u8   getHeight( point* t );
  void setHeight( point* t, u8 h );

  u8 getHeightCell( point* c );


  //////////////////////////////////////////////////
  // in pathing.cpp
  //////////////////////////////////////////////////
  void computePathingFromCliffChanges();

  // an interdemiate layer to calculate accurate
  // terrain pathing
  bool* mapCliffChanges;
  bool  getMapCliffChange( point* c );
  void  setMapCliffChange( point* c, bool mcc );

  // the pathing map is 0 (false) if no pathing is
  // allowed in the cell and 1 (true) if allowed--
  // build pathing maps for cliff walkers also, and
  // do a version of paths with and without
  // destructible rocks
  bool* mapPathing;

  void  setPathing( point* c, PathType t, bool p );

  void  setPathingAllTypes      ( point* c, bool p );
  void  setPathingNonCliffTypes ( point* c, bool p );
  void  setPathingCliffTypes    ( point* c, bool p );
  void  setPathingBuildableTypes( point* c, bool p );

  bool  getPathing             ( point* c, PathType t );
  bool  getPathingOutOfBoundsOK( point* c, PathType t );


  // calculate this when reading in the map data
  // and make it available for analysis
  int numPathableCells[NUM_PATH_TYPES];


  void processRamp( TiXmlElement* ramp );
  point parseRampPoint( const char* str );




  //////////////////////////////////////////////////
  // in placedobjects.cpp
  //////////////////////////////////////////////////
  list<StartLoc*>    startLocs;
  list<Watchtower*>  watchtowers;
  list<Resource*>    resources;
  list<Destruct*>    destructs;
  list<LoSB*>        losbs;
  list<point>        pathingFillsToRender;
  list<point>        opennessNeighborhoodsToRender;

  float totalMinerals;
  float totalVespeneGas;
  float totalHYMinerals;
  float totalHYVespeneGas;

  void processPlacedObject( ObjectMode objMode, TiXmlElement* object );

  void applyFillsAndFootprints();

  void applyFill( point* c );
  int  makePointKey( point* c );
  void propagateFill( int dx, int dy, point* c,
                      map<int, point>* fillSet,
                      map<int, point>* workSet );

  void applyFootprint( point* c, float rot, string* type, string* name );




  //////////////////////////////////////////////////
  // in bases.cpp
  //////////////////////////////////////////////////
  list<Base*> bases;

  void locateBases();
  void assignNearestPathNodes();
  void nameStartLocs();
  void calculateAverageOpennessPerBase();
  void calculateInfluence();
  void assignMainNatThirdIslands();
  void calculatePositionalBalance();

  bool getNearestBase( Resource* r, Base** bOut );
  void addToBaseTotals( Base* b, Resource* r );
  void addToMapTotals( Resource* r );

  float weightedInfluenceDistance( StartLoc* sl, Base* b, point* p );
  float averageInfluence         ( StartLoc* sl, Base* b );

  float watchtowerCoverage;
  void calculateWatchtowerCoverage();



  //////////////////////////////////////////////////
  // in openness.cpp
  //////////////////////////////////////////////////
  static float OPENNESS_NOTCALCULATED;

  // openness is a numerical value for a cell that
  // determines how far away the nearest unpathable
  // cell is, or how "open" the cell is
  float* mapOpenness;
  float* mapOpennessPrev;

  void computeOpenness( PathType t );

  bool  checkHasOpenness( point* c, PathType t );
  float getOpenness     ( point* c, PathType t );
  void  setOpenness     ( point* c, PathType t, float o );

  bool  checkHasOpennessLastPass( point* c, PathType t );
  float getOpennessLastPass     ( point* c, PathType t );


  float calculateAverageOpennessInNeighborhood( point*   p,
                                                float    radius,
                                                PathType t );

  float chokeDistance( point* c, PathType t );
  float spanDistance ( point* c, PathType t, int dx, int dy );

  void findSpace( int dx, int dy, point* c,
                  map<int, point>* fillSet,
                  map<int, point>* workSet,
                  point* choke );

  // max openness values found for the map, one for
  // each pathing type, and also average openness
  float opennessMax[NUM_PATH_TYPES];
  float opennessAvg[NUM_PATH_TYPES];



  //////////////////////////////////////////////////
  // in dijkstra.cpp
  //////////////////////////////////////////////////

  // for connecting the pathing graphs
  static float k0; // distance u-4, etc.
  static float k1; // distance u-8, etc.
  static float neighborWeights[NUM_NODE_NEIGHBORS];

  void buildPathGraph( PathType t );
  void computeShortestPaths( Node* src, PathType t );

  // there is a disjoint graph of nodes for each path
  // type all contained in this array
  Node** mapPathNodes;
  Node*  getPathNode( point* c, PathType t );
  void   setPathNode( point* c, PathType t, Node* u );

  // for each path type there is a sequence of nodes
  // where each node is also in the map of path nodes
  // by location, and the node's position in this vector
  // is the same as its ID
  vector<Node*> nodes[NUM_PATH_TYPES];

  // the shortest path distance (d) and
  // predecessor (pi) results of many-pairs shortest
  // path calculations.  When you need to access say:
  //   *shortest path distance, d
  //   *for path type t
  //   *from node ID=i to node ID=j you would use:
  //
  //     (*(d[t][i]))[j]
  //
  map< int, vector<float>* > d [NUM_PATH_TYPES];
  map< int, vector<Node*>* > pi[NUM_PATH_TYPES];

  // best used by other modules--if you ask for shortest distance
  // from points that are out of bounds or over unpathable cells
  // you get a nice meaningful infinity returned
  float getShortestPathDistance( point* src, point* dst, PathType t );

  float getShortestPathDistance   ( Node* u, Node* v, PathType t );
  void  setShortestPathDistance   ( Node* u, Node* v, PathType t, float dIn );

  Node* getShortestPathPredecessor( Node* u, Node* v, PathType t );
  void  setShortestPathPredecessor( Node* u, Node* v, PathType t, Node* piIn );

  float getShortestPathDistance( point* p, Base* b, PathType t );

  float getShortestPathDistance( Node* u,  Base* b,  PathType t );
  float getShortestPathDistance( Base* b1, Base* b2, PathType t );

  Node* getShortestPathPredecessor( Node* u, Base* b, PathType t );

  void getShortestPathPredecessors( Base* b0, Base* b1, PathType t,
                                    Node** uOut, Node** vOut );


  // a reusable priority queue for the shortest path
  // algorithm (Dijkstra's)
  PrioQueue pqueue;

  // doesn't require Dijkstra's, but fits nicely in this file anyway
  float getShortestAirDistance( point* src, point* dst );


  // just a helper func
  static bool effectivelyInfinity( float x );



  //////////////////////////////////////////////////
  // implemented in spreadsheet.cpp
  //////////////////////////////////////////////////




  //////////////////////////////////////////////////
  // in render.cpp
  //////////////////////////////////////////////////
  static int iLeft;
  static int iBottom;
  static int iRight;
  static int iTop;
  static int iDimT;
  static int iDimC;

  static int iFooterHeight;


  int ix2png( int ix );
  int iy2png( int iy );

  void resetPNGWriter( const char* strFileSuffix,
                       int         iBorderLeft,
                       int         iBorderRight,
                       int         iBorderTop,
                       int         iBorderBottom );

  void renderFooter();

  void renderTerrain();
  void plotTerrain( point* t );
  void plotTerrainPoint( point* t, int* iPoints, int numPoints, Color* color );

  void renderImagePathing( const char* strOutPNG, PathType t );
  void renderPathing( PathType t );
  void plotPathing( point* c, PathType t );

  void renderOpenness( PathType t );
  void plotOpenness( point* c, PathType t );

  void openness2color( float oIn, Color* colorOut );

  void renderResources();
  void plotMineralField( point* p, bool highYield );
  void plotGeyser( point* p, bool highYield );

  void renderWatchtowers();
  void plotWatchtower( Watchtower* wt );

  void renderStartLocs();
  void plotStartLoc( StartLoc* sl, Color* color );

  void renderDestructs();
  void plotDestruct( point* p );

  void renderLoSBs();
  void plotLoSB( point* p );

  void renderChokes();
  void plotChoke( point* p, char* text, Color* color );

  void renderBases();
  void renderBasesDebug();
  //void plotBaseRelationToStartLocs( Base* b, int* baseline );
  
  void plotBaseInformation( Base* base );

  //void plotBaseLocalProperties    ( Base* b, int* baseline );
  //void plotBaseGlobalProperties   ( Base* b, int* baseline );
  //void plotBaseResources          ( Base* b, int* baseline );
  void plotBaseDebugInformation   ( Base* b );

  // remove later
  //void plotBaseStats( Base* base );
  //void plotBaseClass( Base* base, bool classOnly );



  void renderSpaceInMain();

  void renderImageInfluenceHeatMap( StartLoc* sl1, StartLoc* sl2 );
  void renderInfluenceHeatMap     ( StartLoc* sl1, StartLoc* sl2 );
  void plotInfluenceHeatMap( point*    c,
                             StartLoc* sl1,
                             StartLoc* sl2 );


  void renderImageInfluence( StartLoc* sl1, StartLoc* sl2 );
  void renderInfluence     ( StartLoc* sl1, StartLoc* sl2 );

  void renderImageShortestPaths( char*  filename,
                                 char*  title,
                                 point* p0,
                                 Base*  b0,
                                 point* p1,
                                 Base*  b1 );

  void plotAirPath( point* p0, point* p1, Color* color,
                    float labelPlacement );

  void plotShortestPath( point* p, Base* b, PathType t, Color* color,
                         float labelPlacement );

  void plotShortestPath( Base* b0, Base* b1, PathType t, Color* color,
                         float labelPlacement );

  void plotShortestPath( point* p0, point* p1, PathType t, Color* color,
                         float labelPlacement,
                         float dOverride );

  void plotAllShortestPaths( point* src, PathType t );

  void plotCellPoint( point* c, Color* color );

  void plotArrow( int x0, int y0, int x1, int y1,
                  Color* color );
  void plotLine ( int x0, int y0, int x1, int y1,
                  float thick, Color* color );

  void plotTri( int x1, int y1,
                int x2, int y2,
                int x3, int y3,
                Color* color );

  void plotText( char* text,
                 int fontsize,
                 bool centered, int ix, int iy,
                 float angle,
                 Color* colorf, // foreground
                 Color* colorb  // background
               );
  int getTextWidth( char* text, int fontsize );
  
  void plotTextMultiColor( int numSegments,
                           char** texts,
                           int fontsize,
                           bool centered, int ix, int iy,
                           float angle,
                           Color** fores,
                           Color** backs
                         );
                         
  void gradient( float   zeroToOne,
                 int     numColors,
                 string* colorBaseName,
                 Color*  cOut );

  void renderTestFrameTranslation();
  void renderTestClockFaceDetection();
  void renderTestAllPaths();
  void renderTestPathingFills();
  void renderTestDetectChokes();

  pngwriter* img;
};


#endif // ___SC2Map_hpp___
