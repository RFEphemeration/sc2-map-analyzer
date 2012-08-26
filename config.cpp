#include <stdlib.h>
#include <stdio.h>

#include "config.hpp"
#include "utility.hpp"
#include "outstreams.hpp"

#include "SC2Map.hpp"


static const int INPUT_LINE_LEN = 1024;


string footprintRot90cw  = "__rot90cw";
string footprintRot180cw = "__rot180cw";
string footprintRot270cw = "__rot270cw";


void readLocalesConfig  ( string* path, Config* c );
void readFootprintConfig( string* path, Config* c );
void readToAnalyzeConfig( string* path, Config* c );
void readConstantsConfig( string* path, Config* c );
void readColorsConfig   ( string* path, Config* c );
void readOutputConfig   ( string* path, Config* c );


// there are three places to look for a given
// config file, in this order:

// 1. In the arg directory (user)
// 2. In the tool directory (user)
// 3. In the tool/resources dir (system default)

Config configInternal;
Config configUserGlobal;



Config::~Config()
{
  for( list<Footprint*>::iterator itr = allFootprints.begin();
       itr != allFootprints.end();
       ++itr )
  {
    delete *itr;
  }

  for( map< string, map<string, Footprint*>* >::iterator itr = type2name2foot.begin();
       itr != type2name2foot.end();
       ++itr )
  {
    delete itr->second;
  }
}








// initialze some data that supports
// the reading of config files
static list<string> validLocales;
static set <string> validOutputs;


void initConfigReading()
{
  validLocales.push_back( "enUS" );
  validLocales.push_back( "enGB" );
  validLocales.push_back( "deDE" );
  validLocales.push_back( "frFR" );
  validLocales.push_back( "ruRU" );
  validLocales.push_back( "itIT" );
  validLocales.push_back( "esMX" );
  validLocales.push_back( "esES" );
  validLocales.push_back( "plPL" );
  validLocales.push_back( "ptBR" );
  validLocales.push_back( "koKR" );
  validLocales.push_back( "zhCN" );
  validLocales.push_back( "zhTW" );

  validOutputs.insert( "renderTerrain"          );
  validOutputs.insert( "renderPathing"          );
  validOutputs.insert( "renderBases"            );
  validOutputs.insert( "renderOpenness"         );
  validOutputs.insert( "renderShortest"         );
  validOutputs.insert( "renderInfluence"        );
  validOutputs.insert( "renderInfluenceHeatMap" );
  validOutputs.insert( "renderSummary"          );
  validOutputs.insert( "writeCSVpermap"         );
  validOutputs.insert( "writeCSVaggr"           );
}


void createInternalConfig( Config* c )
{
  for( list<string>::iterator itr = validLocales.begin();
       itr != validLocales.end();
       ++itr )
  {
    c->localePreferences.push_back( *itr );
  }


  c->outputConfigPresent = true;
  c->outputOptions.insert( "renderSummary"  );
  c->outputOptions.insert( "renderShortest" );
  c->outputOptions.insert( "writeCSVpermap" );
  c->outputOptions.insert( "writeCSVaggr"   );


  c->fConstants["defaultMineralAmount"] = 1500.0f;
  c->fConstants["defaultGeyserAmount" ] = 2500.0f;

  c->fConstants["opennessRenderMax" ] = 14.0f;

  c->fConstants["baseAvgOpennessNeighborhoodRadius"] = 12.0f;

  c->fConstants["minInfluenceToConsiderNatOrThird"] = 47.0f;

  c->fConstants["chokeDetectionThreshhold"] = 12.0f;
  c->fConstants["chokeDetectionAgreement" ] = 7.0f;

  c->fConstants["spaceInMainChokeRadius"] = 8.0f;

  c->fConstants["influenceWeightGround" ] = 0.70f;
  c->fConstants["influenceWeightCWalk"  ] = 0.10f;
  c->fConstants["influenceWeightAir"    ] = 0.20f;


  c->colors["defaultTxtFg"] = Color( 1.0f, 1.0f, 1.0f );
  c->colors["defaultTxtBg"] = Color( 0.0f, 0.0f, 0.0f );

  c->colors["footerTxtFg"] = c->colors["defaultTxtFg"];
  c->colors["footerTxtBg"] = c->colors["defaultTxtBg"];

  c->colors["shortestPathGround"] = Color( 1.0f, 1.0f, 0.0f );
  c->colors["shortestPathCWalk" ] = Color( 1.0f, 0.0f, 1.0f );
  c->colors["shortestPathAir"   ] = Color( 0.0f, 1.0f, 1.0f );

  c->colors["influence1"] = Color( 1.0f, 1.0f, 0.0f );
  c->colors["influence2"] = Color( 0.5f, 0.5f, 1.0f );

  c->colors["terrainElev0"] = Color( 0.00f, 0.00f, 0.00f );
  c->colors["terrainElev1"] = Color( 0.25f, 0.25f, 0.25f );
  c->colors["terrainElev2"] = Color( 0.45f, 0.45f, 0.45f );
  c->colors["terrainElev3"] = Color( 0.70f, 0.70f, 0.70f );

  c->colors["pathingBlocked"] = Color( 0.7f, 0.0f, 0.0f );
  c->colors["pathingClear"  ] = Color( 0.0f, 0.7f, 0.0f );

  c->colors["destruct" ] = Color( 0.1f, 0.1f, 0.8f );
  c->colors["LoSB"     ] = Color( 0.3f, 0.7f, 0.0f );
  c->colors["mainChoke"] = Color( 1.0f, 1.0f, 0.0f );

  c->colors["regMinerals"] = Color( 0.0f, 1.0f, 1.0f );
  c->colors["HYMinerals" ] = Color( 0.8f, 0.8f, 0.0f );
  c->colors["regGeyser"  ] = Color( 1.0f, 1.0f, 1.0f );

  c->colors["watchTower"      ] = Color( 0.8f, 1.0f, 0.0f );
  c->colors["watchTowerRadius"] = Color( 1.0f, 1.0f, 1.0f );

  c->colors["startLocation"] = Color( 1.0f, 1.0f, 1.0f );

  c->colors["baseClass-semiIsland"] = Color( 0.2f, 0.6f, 1.0f );
  c->colors["baseClass-island"    ] = Color( 0.2f, 0.6f, 1.0f );
  c->colors["baseClass-natural"   ] = Color( 0.7f, 1.0f, 0.7f );
  c->colors["baseClass-third"     ] = Color( 1.0f, 0.7f, 0.7f );

  c->colors["spaceInMain"] = Color( 1.0f, 1.0f, 0.8f );

  c->colors["opennessLow"]  = Color( "e86419" );
  c->colors["opennessMid1"] = Color( "9be170" );
  c->colors["opennessMid2"] = Color( "8395f9" );
  c->colors["opennessHigh"] = Color( "ffb0c0" );
}


void readConfigFiles( bool global, string* path, Config* c )
{
  // the reason for this bool is to decide whether an
  // output.txt was present at the level for this Config
  // object, if so mark it as present (output is overridden)
  c->outputConfigPresent = false;

  if( path == NULL )
  {
    createInternalConfig( c );
    return;
  }

  if( global )
  {
    // these config files are not recognized at local level
    readToAnalyzeConfig( path, c );
  }

  readLocalesConfig  ( path, c );
  readConstantsConfig( path, c );
  readFootprintConfig( path, c );
  readColorsConfig   ( path, c );
  readOutputConfig   ( path, c );
}


// all config files should call this on each input
// line before parsing anything because it will:
// 1) remove comments
// 2) convert white space to plain spaces
void preprocess( string* line )
{
  string::size_type i;

  // 1)
  i = line->find_first_of( "#" );
  if( i != string::npos )
  {
    line->erase( i, string::npos );
  }

  // 2)
  replaceChars( "\t\r\n", " ", line );
}



void readLocalesConfig( string* path, Config* c )
{
  string configFilename( *path );
  configFilename.append( "\\locales.txt" );

  FILE* file = fopen( configFilename.data(), "r" );
  if( file == NULL )
  {
    // if a config file isn't here, no problem just move on
    return;
  }

  int lineNum = 0;
  while( !feof( file ) )
  {
    ++lineNum;

    char lineRaw[INPUT_LINE_LEN];

    char* ret = fgets( lineRaw, INPUT_LINE_LEN, file );
    if( ret == NULL )
    {
      continue;
    }

    string line( lineRaw );
    preprocess( &line );

    strncpy( lineRaw, line.data(), INPUT_LINE_LEN );

    char delims[] = " ";

    char* token = NULL;
    token = strtok( lineRaw, delims );

    if( token == NULL )
    {
      // empty line, no problem
      continue;
    }

    // is token in the list of valid locales?
    bool tokenIsValidLocale = false;

    for( list<string>::iterator validItr = validLocales.begin();
         validItr != validLocales.end();
         ++validItr )
    {
      if( strncmp( token, (*validItr).data(), INPUT_LINE_LEN ) == 0 )
      {
        tokenIsValidLocale = true;

        // found one! If this locale isn't already in the preferences
        // order for the config object, then add it to the back
        bool alreadyInPrefs = false;
        for( list<string>::iterator prefItr = c->localePreferences.begin();
             prefItr != c->localePreferences.end();
             ++prefItr )
        {
          if( *prefItr == *validItr )
          {
            alreadyInPrefs = true;
            break;
          }
        }

        if( !alreadyInPrefs )
        {
          c->localePreferences.push_back( *validItr );
        }

        break;
      }
    }

    if( !tokenIsValidLocale )
    {
      printWarning( "Invalid locale '%s' at %s:%d, ignoring.\n",
                    token,
                    configFilename.data(),
                    lineNum );
      continue;
    }

    token = strtok( NULL, delims );
    if( token != NULL )
    {
      printWarning( "Invalid extra token '%s' at %s:%d, ignoring.\n",
                    token,
                    configFilename.data(),
                    lineNum );
      continue;
    }
  }
}



enum readFootprintMode
{
  RFM_OBJNAMES,
  RFM_CELLS,
};


void readFootprintConfig( string* path, Config* c )
{
  string configFilename( *path );
  configFilename.append( "\\footprints.txt" );

  FILE* file = fopen( configFilename.data(), "r" );
  if( file == NULL )
  {
    // if a config file isn't here, no problem just move on
    return;
  }


  readFootprintMode mode = RFM_OBJNAMES;

  list<string> units;
  list<string> doodads;
  list<string> destructs;
  list<string> resources;
  list<string> nobuilds;
  list<string> nobuildmains;
  list<string> losbs;

  Footprint* footprint = NULL;

  int rcx;
  int rcy;
  

  int lineNum = 0;
  while( !feof( file ) )
  {
    ++lineNum;

    char lineRaw[INPUT_LINE_LEN];

    char* ret = fgets( lineRaw, INPUT_LINE_LEN, file );
    if( ret == NULL )
    {
      continue;
    }

    string line( lineRaw );
    preprocess( &line );

    strncpy( lineRaw, line.data(), INPUT_LINE_LEN );

    // declare vars for two parsing modes below
    char* delims;
    char* token;
    string type;
    string name;


    switch( mode )
    {
      case RFM_OBJNAMES:

        delims = (char*)" ";
        token = strtok( lineRaw, delims );

        if( token == NULL )
        {
          // empty line, no problem
          continue;
        }

        type.assign( token );


        // check if objname mode is over
        if( type == "{" )
        {
          mode = RFM_CELLS;

          footprint = new Footprint();


          token = strtok( NULL, delims );
          if( token != NULL )
          {
            printWarning( "Unexpected extra token %s at %s:%d, brace should have its own line.\n",
                          token,
                          configFilename.data(),
                          lineNum );
          }

          continue;
        }


        // otherwise parse objname

        if( type != "unit"        &&
            type != "doodad"      &&
            type != "destruct"    &&
            type != "resource"    &&
            type != "nobuildmain" &&
            type != "nobuild"     &&
            type != "losb"        )
        {
          printWarning( "Unknown placed object type '%s' at %s:%d, ignoring line.\n",
                        token,
                        configFilename.data(),
                        lineNum );
          continue;
        }

        token = strtok( NULL, delims );
        if( token == NULL )
        {
          printWarning( "Expected object name after %s at %s:%d, ignoring line.\n",
                        type.data(),
                        configFilename.data(),
                        lineNum );
          continue;
        }

        name.assign( token );

        if( type == "unit"        ) { units       .push_back( name ); } else
        if( type == "doodad"      ) { doodads     .push_back( name ); } else
        if( type == "destruct"    ) { destructs   .push_back( name ); } else
        if( type == "resource"    ) { resources   .push_back( name ); } else
        if( type == "nobuildmain" ) { nobuildmains.push_back( name ); } else
        if( type == "nobuild"     ) { nobuilds    .push_back( name ); } else
        if( type == "losb"        ) { losbs       .push_back( name ); }

        token = strtok( NULL, delims );
        if( token != NULL )
        {
          printWarning( "Unexpected extra token %s at %s:%d, ignoring.\n",
                        token,
                        configFilename.data(),
                        lineNum );
          continue;
        }
      break;


      case RFM_CELLS:

        delims = (char*)" ,";
        token = strtok( lineRaw, delims );

        if( token == NULL )
        {
          // empty line, no problem
          continue;
        }

        type.assign( token );

        // check if cells mode is over
        if( type == "}" )
        {
          mode = RFM_OBJNAMES;

          token = strtok( NULL, delims );
          if( token != NULL )
          {
            printWarning( "Unexpected extra token %s at %s:%d, brace should have its own line.\n",
                          token,
                          configFilename.data(),
                          lineNum );
          }


          map<string, Footprint*>* units_name2foot = c->type2name2foot["unit"];
          if( units_name2foot == NULL )
          {
            units_name2foot = new map<string, Footprint*>();
            c->type2name2foot["unit"] = units_name2foot;
          }

          for( list<string>::iterator uItr = units.begin();
               uItr != units.end();
               ++uItr )
          {
            units_name2foot->insert( make_pair( *uItr, footprint ) );
          }


          map<string, Footprint*>* doodads_name2foot = c->type2name2foot["doodad"];
          if( doodads_name2foot == NULL )
          {
            doodads_name2foot = new map<string, Footprint*>();
            c->type2name2foot["doodad"] = doodads_name2foot;
          }

          for( list<string>::iterator dItr = doodads.begin();
               dItr != doodads.end();
               ++dItr )
          {
            doodads_name2foot->insert( make_pair( *dItr, footprint ) );
          }


          map<string, Footprint*>* destructs_name2foot = c->type2name2foot["destruct"];
          if( destructs_name2foot == NULL )
          {
            destructs_name2foot = new map<string, Footprint*>();
            c->type2name2foot["destruct"] = destructs_name2foot;
          }

          for( list<string>::iterator dItr = destructs.begin();
               dItr != destructs.end();
               ++dItr )
          {
            destructs_name2foot->insert( make_pair( *dItr, footprint ) );
          }


          map<string, Footprint*>* resources_name2foot = c->type2name2foot["resource"];
          if( resources_name2foot == NULL )
          {
            resources_name2foot = new map<string, Footprint*>();
            c->type2name2foot["resource"] = resources_name2foot;
          }

          for( list<string>::iterator dItr = resources.begin();
               dItr != resources.end();
               ++dItr )
          {
            resources_name2foot->insert( make_pair( *dItr, footprint ) );
          }


          map<string, Footprint*>* nobuildmains_name2foot = c->type2name2foot["nobuildmain"];
          if( nobuildmains_name2foot == NULL )
          {
            nobuildmains_name2foot = new map<string, Footprint*>();
            c->type2name2foot["nobuildmain"] = nobuildmains_name2foot;
          }

          for( list<string>::iterator dItr = nobuildmains.begin();
               dItr != nobuildmains.end();
               ++dItr )
          {
            nobuildmains_name2foot->insert( make_pair( *dItr, footprint ) );
          }


          map<string, Footprint*>* nobuilds_name2foot = c->type2name2foot["nobuild"];
          if( nobuilds_name2foot == NULL )
          {
            nobuilds_name2foot = new map<string, Footprint*>();
            c->type2name2foot["nobuild"] = nobuilds_name2foot;
          }

          for( list<string>::iterator dItr = nobuilds.begin();
               dItr != nobuilds.end();
               ++dItr )
          {
            nobuilds_name2foot->insert( make_pair( *dItr, footprint ) );
          }


          map<string, Footprint*>* losbs_name2foot = c->type2name2foot["losb"];
          if( losbs_name2foot == NULL )
          {
            losbs_name2foot = new map<string, Footprint*>();
            c->type2name2foot["losb"] = losbs_name2foot;
          }

          for( list<string>::iterator dItr = losbs.begin();
               dItr != losbs.end();
               ++dItr )
          {
            losbs_name2foot->insert( make_pair( *dItr, footprint ) );
          }


          units       .clear();
          doodads     .clear();
          destructs   .clear();
          resources   .clear();
          nobuildmains.clear();
          nobuilds    .clear();
          losbs       .clear();

          footprint = NULL;

          continue;
        }



        // otherwise parse relative cell coordinates
        rcx = atoi( token );

        token = strtok( NULL, delims );
        if( token == NULL )
        {
          printWarning( "Expected a second coordinate at %s:%d, ignoring line.\n",
                        configFilename.data(),
                        lineNum );
          continue;
        }

        rcy = atoi( token );

        footprint->relativeCoordinates.push_back( rcx );
        footprint->relativeCoordinates.push_back( rcy );


        token = strtok( NULL, delims );
        if( token != NULL )
        {
          printWarning( "Unexpected extra token %s at %s:%d, ignoring.\n",
                        token,
                        configFilename.data(),
                        lineNum );
        }
      break;


      default:
        printError( "Unknown mode while parsing footprint.txt!" );
        exit( -1 );
    }
  }


  // if we end mid-footprint parse, gotta clean it up, otherwise
  // its in the master list for the Config and it will be cleaned
  if( footprint != NULL )
  {
    delete footprint;
  }
}



void readToAnalyzeConfig( string* path, Config* c )
{
  string configFilename( *path );
  configFilename.append( "\\to-analyze.txt" );

  FILE* file = fopen( configFilename.data(), "r" );
  if( file == NULL )
  {
    // if a config file isn't here, no problem just move on
    return;
  }

  int lineNum = 0;
  while( !feof( file ) )
  {
    ++lineNum;

    char lineRaw[INPUT_LINE_LEN];

    char* ret = fgets( lineRaw, INPUT_LINE_LEN, file );
    if( ret == NULL )
    {
      continue;
    }

    string line( lineRaw );
    preprocess( &line );

    strncpy( lineRaw, line.data(), INPUT_LINE_LEN );

    bool  recurse       = false;
    char* fileToAnalyze = lineRaw;
    if( lineRaw[0] == 'r' && lineRaw[1] == ' ' )
    {
      recurse       = true;
      fileToAnalyze = &(lineRaw[2]);
    }

    string fileTrimmed( fileToAnalyze );
    trimTrailingSpaces( &fileTrimmed );
    trimLeadingSpaces ( &fileTrimmed );

    if( !fileTrimmed.empty() )
    {
      if( recurse ) {
        c->toAnalyzeRecurse.push_back( fileTrimmed );
      } else {
        c->toAnalyze.push_back( fileTrimmed );
      }
    }
  }
}



void readConstantsConfig( string* path, Config* c )
{
  string configFilename( *path );
  configFilename.append( "\\constants.txt" );

  FILE* file = fopen( configFilename.data(), "r" );
  if( file == NULL )
  {
    // if a config file isn't here, no problem just move on
    return;
  }

  int lineNum = 0;
  while( !feof( file ) )
  {
    ++lineNum;

    char lineRaw[INPUT_LINE_LEN];

    char* ret = fgets( lineRaw, INPUT_LINE_LEN, file );
    if( ret == NULL )
    {
      continue;
    }

    string line( lineRaw );
    preprocess( &line );

    strncpy( lineRaw, line.data(), INPUT_LINE_LEN );


    char delims1[] = " ";
    char* token = NULL;

    token = strtok( lineRaw, delims1 );
    if( token == NULL )
    {
      // empty line, no problem
      continue;
    }

    string type( token );
    trimTrailingSpaces( &type );
    trimLeadingSpaces ( &type );

    if( type != "int"   &&
        type != "float" &&
        type != "string" )
    {
      printWarning( "Invalid constant type '%s' at %s:%d, ignoring.\n",
                    token,
                    configFilename.data(),
                    lineNum );
      continue;
    }


    char delims2[] = "=";

    token = strtok( NULL, delims2 );
    if( token == NULL )
    {
      printWarning( "Expected a constant name at %s:%d, ignoring line.\n",
                    configFilename.data(),
                    lineNum );
      continue;
    }

    string name( token );
    trimTrailingSpaces( &name );
    trimLeadingSpaces ( &name );


    token = strtok( NULL, delims1 );
    if( token == NULL )
    {
      printWarning( "Expected a value for constant at %s:%d, ignoring line.\n",
                    configFilename.data(),
                    lineNum );
      continue;
    }

    string value( token );


    token = strtok( NULL, delims1 );
    if( token != NULL )
    {
      printWarning( "Invalid extra token '%s' at %s:%d, ignoring.\n",
                    token,
                    configFilename.data(),
                    lineNum );
      continue;
    }



    if( type == "int" ) {
      if( c->iConstants.count( name ) > 0 )
      {
        printWarning( "Constant '%s' being redefined at %s:%d.\n",
                      name.data(),
                      configFilename.data(),
                      lineNum );
      }
      c->iConstants[name] = atoi( value.data() );

    } else if( type == "float" ) {
      if( c->fConstants.count( name ) > 0 )
      {
        printWarning( "Constant '%s' being redefined at %s:%d.\n",
                      name.data(),
                      configFilename.data(),
                      lineNum );
      }
      c->fConstants[name] = atof( value.data() );

    } else if( type == "string" ) {
      if( c->sConstants.count( name ) > 0 )
      {
        printWarning( "Constant '%s' being redefined at %s:%d.\n",
                      name.data(),
                      configFilename.data(),
                      lineNum );
      }
      c->sConstants[name] = value;
    }
  }
}


void readColorsConfig( string* path, Config* c )
{
  string configFilename( *path );
  configFilename.append( "\\colors.txt" );

  FILE* file = fopen( configFilename.data(), "r" );
  if( file == NULL )
  {
    // if a config file isn't here, no problem just move on
    return;
  }

  int lineNum = 0;
  while( !feof( file ) )
  {
    ++lineNum;

    char lineRaw[INPUT_LINE_LEN];

    char* ret = fgets( lineRaw, INPUT_LINE_LEN, file );
    if( ret == NULL )
    {
      continue;
    }

    string line( lineRaw );
    preprocess( &line );

    strncpy( lineRaw, line.data(), INPUT_LINE_LEN );


    char delims[] = " =,";
    char* token = NULL;

    token = strtok( lineRaw, delims );
    if( token == NULL )
    {
      // empty line, no problem
      continue;
    }

    string name( token );
    trimTrailingSpaces( &name );
    trimLeadingSpaces ( &name );


    token = strtok( NULL, delims );
    if( token == NULL )
    {
      printWarning( "Expected a color at %s:%d, ignoring line.\n",
                    configFilename.data(),
                    lineNum );
      continue;
    }

    string value( token );
    trimTrailingSpaces( &value );
    trimLeadingSpaces ( &value );

    Color color;

    if( value.length() > 1   &&
        value.at( 0 ) == '0' &&
        value.at( 1 ) == 'x' )
    {
      // hex color

      value.erase( 0, 2 );
      if( value.length() != 6        ||
          !isxdigit( value.at( 0 ) ) ||
          !isxdigit( value.at( 1 ) ) ||
          !isxdigit( value.at( 2 ) ) ||
          !isxdigit( value.at( 3 ) ) ||
          !isxdigit( value.at( 4 ) ) ||
          !isxdigit( value.at( 5 ) )
        )
      {
        printWarning( "Expected 6 hex digit color instead of '%s' at %s:%d, ignoring.\n",
                      value.data(),
                      configFilename.data(),
                      lineNum );
        continue;
      }

      color = Color( value );


    } else if( c->colors.count( value ) > 0 ) {
      // copy a color
      color = c->colors[value];


    } else {
      // 3 component 0->1 color
      color.r = atof( value.data() );

      token = strtok( NULL, delims );
      if( token == NULL )
      {
        printWarning( "Expected two more components for color at %s:%d, ignoring line.\n",
                      configFilename.data(),
                      lineNum );
        continue;
      }

      color.g = atof( token );

      token = strtok( NULL, delims );
      if( token == NULL )
      {
        printWarning( "Expected one more component for color at %s:%d, ignoring line.\n",
                      configFilename.data(),
                      lineNum );
        continue;
      }

      color.b = atof( token );
    }


    if( c->colors.count( name ) > 0 )
    {
      printWarning( "Color '%s' being redefined at %s:%d.\n",
                    name.data(),
                    configFilename.data(),
                    lineNum );
    }

    c->colors[name] = color;


    token = strtok( NULL, delims );
    if( token != NULL )
    {
      printWarning( "Invalid extra token '%s' at %s:%d, ignoring.\n",
                    token,
                    configFilename.data(),
                    lineNum );
      continue;
    }
  }
}


void readOutputConfig( string* path, Config* c )
{
  string configFilename( *path );
  configFilename.append( "\\output.txt" );

  FILE* file = fopen( configFilename.data(), "r" );
  if( file == NULL )
  {
    // if a config file isn't here, no problem just move on
    return;
  }


  c->outputConfigPresent = true;


  int lineNum = 0;
  while( !feof( file ) )
  {
    ++lineNum;

    char lineRaw[INPUT_LINE_LEN];

    char* ret = fgets( lineRaw, INPUT_LINE_LEN, file );
    if( ret == NULL )
    {
      continue;
    }

    string line( lineRaw );
    preprocess( &line );

    strncpy( lineRaw, line.data(), INPUT_LINE_LEN );


    char delims[] = "=";
    char* token = NULL;

    token = strtok( lineRaw, delims );
    if( token == NULL )
    {
      // empty line, no problem
      continue;
    }

    string option( token );
    trimTrailingSpaces( &option );
    trimLeadingSpaces ( &option );

    if( option.length() == 0 )
    {
      // there was only white space, ignore
      continue;
    }

    if( option == "path" )
    {
      // the path has more parsing to do...
      token = strtok( NULL, delims );
      if( token == NULL )
      {
        printWarning( "Expected a path at %s:%d, ignoring line.\n",
                      configFilename.data(),
                      lineNum );
        continue;
      }

      string path( token );
      trimTrailingSpaces( &path );
      trimLeadingSpaces ( &path );

      formatPath( &path );

      if( !c->outputPath.empty() )
      {
        printWarning( "Output path being redefined at %s:%d.\n",
                      configFilename.data(),
                      lineNum );
      }
      c->outputPath.assign( path );

    } else {
      // all other options are just present or not

      if( validOutputs.count( option ) == 0 )
      {
        printWarning( "Unrecognized output option '%s' at %s:%d, ignorning line.\n",
                      option.data(),
                      configFilename.data(),
                      lineNum );
        continue;
      }

      if( c->outputOptions.count( option ) > 0 )
      {
        printWarning( "Output option '%s' redefined at %s:%d.\n",
                      option.data(),
                      configFilename.data(),
                      lineNum );
      }
      c->outputOptions.insert( option );
    }

    token = strtok( NULL, delims );
    if( token != NULL )
    {
      printWarning( "Invalid extra token '%s' at %s:%d, ignoring.\n",
                    token,
                    configFilename.data(),
                    lineNum );
      continue;
    }
  }
}



int SC2Map::getiConstant( string name )
{
  // first check local user config
  if( configUserLocal.iConstants.count( name ) > 0 )
  {
    return configUserLocal.iConstants[name];
  }

  // then global user config
  if( configUserGlobal.iConstants.count( name ) > 0 )
  {
    return configUserGlobal.iConstants[name];
  }

  // last resort is global internal config
  if( configInternal.iConstants.count( name ) > 0 )
  {
    return configInternal.iConstants[name];
  }

  printError( "No internal entry for int constant %s.\n",
              name.data() );
  exit( -1 );
}


// zero for success, -1 for error
float SC2Map::getfConstant( string name )
{
  // first check local user config
  if( configUserLocal.fConstants.count( name ) > 0 )
  {
    return configUserLocal.fConstants[name];
  }

  // then global user config
  if( configUserGlobal.fConstants.count( name ) > 0 )
  {
    return configUserGlobal.fConstants[name];
  }

  // last resort is global internal config
  if( configInternal.fConstants.count( name ) > 0 )
  {
    return configInternal.fConstants[name];
  }

  printError( "No internal entry for float constant %s.\n",
              name.data() );
  exit( -1 );
}


Color* SC2Map::getColor( string name )
{
  // first check local user config
  if( configUserLocal.colors.count( name ) > 0 )
  {
    return &(configUserLocal.colors[name]);
  }

  // then global user config
  if( configUserGlobal.colors.count( name ) > 0 )
  {
    return &(configUserGlobal.colors[name]);
  }

  // last resort is global internal config
  if( configInternal.colors.count( name ) > 0 )
  {
    return &(configInternal.colors[name]);
  }

  printError( "No internal entry for color %s.\n",
              name.data() );
  exit( -1 );
}


bool SC2Map::getOutputOption( string option )
{
  if( validOutputs.count( option ) == 0 )
  {
    printError( "Internal query for unknown option %s.\n",
                option.data() );
    exit( -1 );
  }

  // first check local user config
  if( configUserLocal.outputConfigPresent )
  {
    return configUserLocal.outputOptions.count( option ) > 0;
  }

  // then global user config
  if( configUserGlobal.outputConfigPresent )
  {
    return configUserGlobal.outputOptions.count( option ) > 0;
  }

  // last resort is global internal config
  if( configInternal.outputConfigPresent )
  {
    return configInternal.outputOptions.count( option ) > 0;
  }

  printError( "No internal entry for option %s.\n",
              option.data() );
  exit( -1 );
}


bool getGlobalOutputOption( string option )
{
  if( validOutputs.count( option ) == 0 )
  {
    printError( "Internal query for unknown option %s.\n",
                option.data() );
    exit( -1 );
  }

  // check global user config
  if( configUserGlobal.outputConfigPresent )
  {
    return configUserGlobal.outputOptions.count( option ) > 0;
  }

  // last resort is global internal config
  if( configInternal.outputConfigPresent )
  {
    return configInternal.outputOptions.count( option ) > 0;
  }

  printError( "No internal entry for option %s.\n",
              option.data() );
  exit( -1 );
}
