#include <string>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include "debug.hpp"
#include "utility.hpp"
#include "config.hpp"
#include "outstreams.hpp"
#include "SC2Map.hpp"
#include "SC2MapAggregator.hpp"



// controls set by options: defaults
static bool pauseTerminal          = true;
static bool dirRecurse             = false;
static bool renderTerrain          = false;
static bool renderPathing          = false;
static bool renderBases            = false;
static bool renderOpenness         = false;
static bool renderShortest         = false;
static bool renderInfluence        = false;
static bool renderInfluenceHeatMap = false;
static bool renderSummary          = false;
static bool writeCSVpermap         = false;
static bool writeCSVaggr           = false;


void statFail( string* fileFullPath );

void tryFileOrDir( string* fileFullPath, bool first, bool recurse );
void tryFile     ( string* fileFullPath );


void AtExit()
{
  // flush output
  fflush( stdout );
  fflush( stderr );

  printOutstreamStatus();

  // make sure to hold terminal open so
  // user can read output
  if( pauseTerminal )
  {
    system( "pause" );
  }
}


static string           toolPath;
static SC2MapAggregator mapAggregator;


int main( int argc, char** argv )
{
  atexit( &AtExit );


  if( SC2Map::pngwriterTest() != 0 )
  {
    printError( "Cannot load resources for PNGwriter: you may need to run this program as Administrator.\n" );
    exit( -1 );
  }

  string toolFullName( argv[0] );

  string::size_type locBSlash = toolFullName.rfind( "\\" );
  toolPath.assign( toolFullName, 0, locBSlash + 1 );
  formatPath( &toolPath );

  printMessage( "SC2 Map Analyzer v%s, Algorithms v%s\n  Tool path: %s\n  Message %s for support.\n\n",
                QUOTEMACRO( VEXE ),
                QUOTEMACRO( VALG ),
                toolPath.data(),
                adminEmail
              );

  initConfigReading();
  readConfigFiles( false, NULL, &configInternal );

  readConfigFiles( true, &toolPath, &configUserGlobal );


  writeCSVaggr = getGlobalOutputOption( "writeCSVaggr" );


  if( configUserGlobal.toAnalyze.empty() &&
      configUserGlobal.toAnalyzeRecurse.empty() )
  {
    printMessage( "No file arguments, searching current directory for maps...\n\n" );
    string currentDir( "." );
    tryFileOrDir( &currentDir, true, true );
  }

  for( list<string>::iterator argItr = configUserGlobal.toAnalyze.begin();
       argItr != configUserGlobal.toAnalyze.end();
       ++argItr )
  {
    tryFileOrDir( &(*argItr), true, false );
  }

  for( list<string>::iterator argItr = configUserGlobal.toAnalyzeRecurse.begin();
       argItr != configUserGlobal.toAnalyzeRecurse.end();
       ++argItr )
  {
    tryFileOrDir( &(*argItr), true, true );
  }


  if( writeCSVaggr )
  {
    printMessage( "Calculating aggregate statistics...\n" );
    mapAggregator.buildOutputColumns();

    if( configUserGlobal.outputPath.empty() )
    {
      mapAggregator.writeToCSV( &toolPath );
    } else {
      mapAggregator.writeToCSV( &(configUserGlobal.outputPath) );
    }
  }

  return 0;
}



void statFail( string* fileFullPath )
{
  const char* filename = fileFullPath->data();

  const char msgToAnalyze[] = "\nHave you edited to-analyze.txt with paths to maps you want to analyze?\n\n";

  switch( errno )
  {
    case EACCES:
      printWarning( "Permission denied for %s, skipping...\n",
                    filename );
      break;

    case EBADF:
    case EFAULT:
      printWarning( "%s is an invalid file, skipping...\n",
                    filename );
      break;

    case ENAMETOOLONG:
      printWarning( "Path and filename %s is too long, skipping...\n",
                    filename );
      break;

    case ENOENT:
    case ENOTDIR:
      printWarning( "A component of the path %s does not exist or is not a directory, skipping...\n%s",
                    filename,
                    msgToAnalyze );
      break;

    case ENOMEM:
      printError( "Not enough system memory to continue.\n" );
      exit( -1 );
      break;

    default:
      printWarning( "Unknown problem trying to access %s, skipping...\n",
                    filename );
  }
}


// find out if the non-option argument is a file or
// a directory, and for directories decide if we should
// recursively explore them if the flag is set
void tryFileOrDir( string* fileFullPathIn, bool first, bool recurse )
{
  // make a copy of the path and filename for mutation
  string fileFullPath( *fileFullPathIn );

  formatPath( &fileFullPath );

  // find out what kind of file we have
  struct stat fileStatus;
  if( stat( fileFullPath.data(), &fileStatus ) < 0 )
  {
    statFail( &fileFullPath );
    return;
  }

  // only deal with regular files or directories,
  // ignore other file types (or "modes")
  if( S_ISREG( fileStatus.st_mode ) )
  {
    tryFile( &fileFullPath );


  } else if( S_ISDIR( fileStatus.st_mode ) ) {

    if( !first && !recurse )
    {
      return;
    }

    DIR* dir = opendir( fileFullPath.data() );
    if( dir != NULL )
    {
      struct dirent* de;
      while( de = readdir( dir ) )
      {
        // ignore the current and up-one directories, . and ..
        if( strcmp( de->d_name, "."  ) == 0 ||
            strcmp( de->d_name, ".." ) == 0 )
        {
          continue;
        }

        // make a path out of this directory name
        string deeperFile( fileFullPath );
        deeperFile.append( "\\" );
        deeperFile.append( de->d_name );

        // only continue to recurse beyond the first level
        // if the user passed recursive option
        tryFileOrDir( &deeperFile, false, recurse );
      }
      closedir( dir );

    } else {
      printWarning( "Could not open directory %s.\n", fileFullPath.data() );
    }

  } else {
    printWarning( "%s is not a regular file or directory, skipping...", fileFullPath.data() );
  }
}



// found a regular file, see if it has a map
// extension and then try to open the archive
void tryFile( string* fileFullPath )
{
  string ext1( ".s2ma" );
  string ext2( ".SC2Map" );

  string::size_type loc1 = fileFullPath->rfind( ext1 );
  string::size_type loc2 = fileFullPath->rfind( ext2 );

  // if either search for an extension doesn't come
  // up with the location where the extension should be...
  if( loc1 != fileFullPath->size() - ext1.size() &&
      loc2 != fileFullPath->size() - ext2.size() )
  {
    // not a Starcraft 2 Map Archive...
    return;
  }

  // find last dot, last back slash
  string::size_type locDot    = fileFullPath->rfind( "." );
  string::size_type locBSlash = fileFullPath->rfind( "\\" );

  string path( *fileFullPath, 0,             locBSlash + 1                 );
  string file( *fileFullPath, locBSlash + 1, locDot - locBSlash - 1        );
  string ext ( *fileFullPath, locDot,        fileFullPath->size() - locDot );

  string localOutputPath( path );

  // consider switching to the globally specified
  // output path, if it exists AND is valid
  if( !configUserGlobal.outputPath.empty() )
  {
    struct stat dirStatus;
    if( stat( configUserGlobal.outputPath.data(), &dirStatus ) >= 0 )
    {
      if( S_ISDIR( dirStatus.st_mode ) )
      {
        localOutputPath.assign( configUserGlobal.outputPath );
      }
    }
  }


  string fileWithExt( file );
  fileWithExt.append( ext );

  SC2Map* sc2map = new SC2Map( toolPath,
                               localOutputPath,
                               path,
                               file,
                               fileWithExt );

  printMessage( "\nOpening %s...\n", fileFullPath->data() );

  readConfigFiles( false, &path, &(sc2map->configUserLocal) );

  // consider switching to the locally specified
  // output path, if it exists AND is valid
  if( !sc2map->configUserLocal.outputPath.empty() )
  {
    struct stat dirStatus;
    if( stat( sc2map->configUserLocal.outputPath.data(), &dirStatus ) >= 0 )
    {
      if( S_ISDIR( dirStatus.st_mode ) )
      {
        sc2map->outputPath = sc2map->configUserLocal.outputPath;
      }
    }
  }

  renderTerrain          = sc2map->getOutputOption( "renderTerrain"          );
  renderPathing          = sc2map->getOutputOption( "renderPathing"          );
  renderBases            = sc2map->getOutputOption( "renderBases"            );
  renderOpenness         = sc2map->getOutputOption( "renderOpenness"         );
  renderShortest         = sc2map->getOutputOption( "renderShortest"         );
  renderInfluence        = sc2map->getOutputOption( "renderInfluence"        );
  renderInfluenceHeatMap = sc2map->getOutputOption( "renderInfluenceHeatMap" );
  renderSummary          = sc2map->getOutputOption( "renderSummary"          );
  writeCSVpermap         = sc2map->getOutputOption( "writeCSVpermap"         );

  if( sc2map->readMap() < 0 )
  {
    printWarning( "Could not read required map files for %s, skipping\n\n", file.data() );
    return;
  }

  printMessage( "Prepping analysis,\n" );

  sc2map->countPathableCells();

  printMessage( "." );

  sc2map->prepShortestPaths();

  printMessage( "." );

  sc2map->identifyBases();

  printMessage( "." );

  sc2map->computeOpenness();

  printMessage( "." );

  sc2map->locateChokes();

  printMessage( "." );

  sc2map->analyzeBases();

  printMessage( "\nAnalyzing and generating output,\n" );
  printMessage( "  Map-specific output in [%s]\n", sc2map->outputPath.data() );

  if( renderTerrain )
  {
    sc2map->renderImageTerrainOnly();
  }

  printMessage( "." );

  if( renderPathing )
  {
    sc2map->renderImagesPathing();
  }

  printMessage( "." );

  if( renderBases )
  {
    sc2map->renderImageBases();
  }

  printMessage( "." );

  if( renderOpenness )
  {
    sc2map->renderImageOpenness();
  }

  printMessage( "." );

  if( renderShortest )
  {
    sc2map->renderImagesShortestPaths();
  }

  printMessage( "." );

  if( renderInfluence )
  {
    sc2map->renderImagesInfluence();
  }

  printMessage( "." );

  if( renderInfluenceHeatMap )
  {
    sc2map->renderImagesInfluenceHeatMap();
  }

  printMessage( "." );

  if( renderSummary )
  {
    sc2map->renderImageSummary();
  }

  printMessage( "." );

  if( writeCSVpermap )
  {
    sc2map->writeToCSV();
  }

  printMessage( "." );

  if( writeCSVaggr )
  {
    mapAggregator.aggregate( sc2map );
  }

  printMessage( "\n\n" );


  // normally the guts of this function
  // are commented out, uncomment for testing
  sc2map->renderTestImages();


  delete sc2map;
}
