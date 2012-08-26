#ifndef ___sc2mapTypes_hpp___
#define ___sc2mapTypes_hpp___


#include <map>
#include <list>
#include <string>
using namespace std;

#include "coordinates.hpp"
#include "utility.hpp"



typedef char  s8;
typedef short s16;
typedef int   s32;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// max length of any filenames
#define FILENAME_LENGTH 512


// effectively infinity, no map has a shortest path
// of millions of cells from one point to another
#define infinity 9000000.0f


// start location names are "12 o'clock" and such
#define STARTLOC_NAME_LENGTH 11



// this defines a mode to process placed objects
// based on the version of the Object internal format
enum ObjectMode
{
  OBJMODE_BETAPH1_v26 = 1,
  OBJMODE_BETAPH2_v26,
};



// typically this enum is used as an offset
// into arrays--like the pathing array--so
// there is an entry for each path type at
// every location
enum PathType
{
  PATH_GROUND_NOROCKS = 0,
  PATH_GROUND_WITHROCKS,
  PATH_CWALK_NOROCKS,
  PATH_CWALK_WITHROCKS,
  PATH_GROUND_WITHROCKS_NORESOURCES,
  PATH_BUILDABLE,
  PATH_BUILDABLE_MAIN,
  NUM_PATH_TYPES
};



// only use the directions N, S, E, W,
// NE, NW, SE, SW, and the NNE, ENE, etc.
#define NUM_NODE_NEIGHBORS 16



// forward declaration for a recursive type
struct Node;

struct Node
{
  // each node in one pathing graph has
  // a unique ID from 0 to N, depending on
  // how many pathable cells a map has
  int id;

  // alternatively, a node is named by its
  // position in map coordinates as well
  point loc;

  // neighbors
  Node* neighbors[NUM_NODE_NEIGHBORS];

  // used temporarily during each
  // shortest path calculation
  int   queueIndex;
  float key;

  // only perform shortest path calcs if
  // the result is requested
  bool pathsFromThisSrcCalculated;
};



// forward declaration
struct Base;

struct StartLoc
{
  point loc;
  float r, g, b;
  char  name[STARTLOC_NAME_LENGTH];
  int   idNum;

  // when this start location spawns against
  // another start location, the resource
  // influence is a total of the resources of
  // bases this location has influence over
  // compared to that start loc
  map<StartLoc*, float> sl2resourceInfluence;
  map<StartLoc*, float> sl2opennessInfluence;
  map<StartLoc*, float> sl2percentBalancedByResources;
  map<StartLoc*, float> sl2percentBalancedByOpenness;

  Base* mainBase;
  Base* natBase;
  Base* thirdBase;
  Base* backdoorBase;

  point mainChoke;
  point natChoke;

  // count space in main in cells
  int spaceInMain;
};



enum ResourceType {
  MINERALS,
  VESPENEGAS,
  MINERALS_HY,
  VESPENEGAS_HY,
  NUM_RESOURCE_TYPES
};

struct Resource
{
  point        loc;
  u8           cliffLevel;
  ResourceType type;
  float        amount;
  string       placedObjName;
};



enum BaseNumExpo {
  MAIN_EXPO,
  NATURAL_EXPO,
  THIRD_EXPO,
  OTHER_EXPO,
};

enum BaseIslandType {
  NOT_AN_ISLAND,
  ISLAND,
  SEMI_ISLAND,
};

struct Base {

  point    loc;
  u8       cliffLevel;

  // if this base is nearer a start location,
  // which one?
  StartLoc* sl;
  
  bool isInMain;
  
  // which "number" expo is this base, a main? natural?
  BaseNumExpo numExpo;
  
  BaseIslandType islandType;
  
  list<Resource*> resources;

  float totalRegMinerals;
  float totalRegVespeneGas;
  float totalHYMinerals;
  float totalHYVespeneGas;
  float resourceTotal;

  float avgOpennessForNeighborhood;

  // this mapping is from path nodes (per pathing type) to
  // the distance they are from the base's true location,
  // which in some cases is over unpathable area (such as
  // a base blocked by destructible rocks)
  map<Node*, float> node2patchDistance[NUM_PATH_TYPES];

  // start loc, start loc to an influence value, it means
  // The influence SL1 exerts on the base when spawned
  // against SL2 in 1v1.
  map< StartLoc*, map<StartLoc*, float>* > sl1vsl2_influence;

  // for main/nat/third classification
  map<StartLoc*, float> sl2averageInfluence;
};



struct Watchtower
{
  point loc;
  float range;
};


struct Destruct
{
  point loc;
};


struct LoSB
{
  point loc;
};


struct Color
{
  Color()
  {
    r = 0.0f; g = 0.0f; b = 0.0f;
  }

  Color( Color* c ) {
    r = c->r; g = c->g; b = c->b;
  }
  
  Color( float rIn, float gIn, float bIn )
  {
    r = rIn; g = gIn; b = bIn;
  }

  Color( string hexValue )
  {
    r = xtof( hexValue.at( 0 ), hexValue.at( 1 ) );
    g = xtof( hexValue.at( 2 ), hexValue.at( 3 ) );
    b = xtof( hexValue.at( 4 ), hexValue.at( 5 ) );
  }

  float r;
  float g;
  float b;
};



struct Footprint
{
  list<int> relativeCoordinates;
};


#endif // ___sc2mapTypes_hpp___
