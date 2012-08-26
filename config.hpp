#ifndef ___config_hpp___
#define ___config_hpp___

#include <string>
#include <list>
#include <set>
#include <map>
using namespace std;

#include "sc2mapTypes.hpp"


struct Config
{
  ~Config();

  list<string> toAnalyze;
  list<string> toAnalyzeRecurse;

  list<string> localePreferences;


  bool        outputConfigPresent;
  string      outputPath;
  set<string> outputOptions;


  map<string, int>    iConstants;
  map<string, float>  fConstants;
  map<string, string> sConstants;

  map<string, Color> colors;


  // unit/doodad/etc. -> name -> footprint
  list<Footprint*> allFootprints;
  map< string, map<string, Footprint*>* > type2name2foot;
};


void initConfigReading();

void readConfigFiles( bool global, string* path, Config* c );

void createInternalConfig( Config* c );

bool getGlobalOutputOption( string option );


extern Config configInternal;
extern Config configUserGlobal;


extern string footprintRot90cw;
extern string footprintRot180cw;
extern string footprintRot270cw;


#endif // ___config_hpp___
