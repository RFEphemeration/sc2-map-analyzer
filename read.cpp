#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "StormLib.h"
#include "tinyxml.h"

#include "utility.hpp"
#include "debug.hpp"
#include "outstreams.hpp"
#include "coordinates.hpp"
#include "SC2Map.hpp"



int SC2Map::readMap()
{
  HANDLE archive;

  string fullArchiveName( argPath );
  fullArchiveName.append( archiveWithExt );

  if( !SFileOpenArchive( fullArchiveName.data(), 0, 0, &archive ) )
  {
    printWarning( "Could not open map MPQ archive file %s\n", archiveWithExt.data() );
    return -1;
  }

  // the order of reading archive files is important--some
  // analysis data structures are depedenent on one another
  if( readGameStrings_txt( archive ) < 0 )
  {
    printWarning( "Could not find a locale to extract map name, using archive filename instead.\n" );
    mapName.assign( archiveName );
  }
  // however we got a name for the map, make a version
  // that is valid for use in filenames
  makeMapNameValidForFilenames();

  printMessage( "  map name found for preferred locale: [%s]\n", mapName.data() );
  printMessage( "  map name as part of filenames:       [%s]\n", mapNameInOutputFiles.data() );

  if( readMapInfo( archive ) < 0 )
  {
    printMessage( "  No MapInfo file.\n" );
    return -1;
  }

  if( debugMapInfo > 0 )
  {
    printMessage( "End debugging MapInfo\n" );
    exit( 0 );
  }

  if( readt3HeightMap( archive ) < 0 )
  {
    printMessage( "  No t3HeightMap file.\n" );
    return -1;
  }

  computePathingFromCliffChanges();

  if( readt3Terrain_xml( archive ) < 0 )
  {
    printMessage( "  No Terrain.xml file, continuing...\n" );
  }

  if( readPaintedPathingLayer( archive ) < 0 )
  {
    printMessage( "  No PaintedPathingLayer file, continuing...\n" );
  }

  if( readObjects( archive ) < 0 )
  {
    printMessage( "  No Objects file, (StartLocs, resources, etc.), continuing...\n" );
  } else {
    // only do this after all Objects are read
    applyFillsAndFootprints();

    if( startLocs.empty() )
    {
      printWarning( "No start locations have been placed, some outputs may have odd values.\n" );
    }
  }

  if( debugObjects > 0 )
  {
    printMessage( "End debugging Objects\n" );
    exit( 0 );
  }


  SFileCloseArchive( archive );
  return 0;
}



int SC2Map::readArchiveFile( const HANDLE archive,
                             const char*  strFilename,
                             int*         bufferOutSize,
                             u8**         bufferOut )
{
  HANDLE hFile;

  if( !SFileOpenFileEx( archive, strFilename, 0, &hFile ) )
  {
    //printWarning( "Could not open %s for reading.\n", strFilename );
    return -1;
  }

  DWORD fileSizeBytes = SFileGetFileSize( hFile, NULL );
  if( fileSizeBytes == SFILE_INVALID_SIZE ||
      fileSizeBytes <= 0 )
  {
    printWarning( "%s is empty or invalid.\n", strFilename );
    return -1;
  }

  u8*   buffer = new u8[fileSizeBytes];
  DWORD numBytesRead;

  // initialize buffer to easily recognized values,
  // after a successful read all of them are overwritten
  memset( buffer, 'q', fileSizeBytes );

  if( !SFileReadFile( hFile, (void*)buffer, fileSizeBytes, &numBytesRead, NULL ) )
  {
    delete buffer;
    printWarning( "Could not read %s from archive.\n", strFilename );
    return -1;
  }

  if( numBytesRead != fileSizeBytes )
  {
    delete buffer;
    printWarning( "Could not read %s from archive. [NOT EXPECTING TO SEE THIS WARNING!]\n", strFilename );
    return -1;
  }

  *bufferOutSize = fileSizeBytes;
  *bufferOut     = buffer;

  SFileCloseFile( hFile );
  return 0;
}



int SC2Map::verifyMagicWord( const u8* magic, const u8* buffer )
{
  // a magic word (4 bytes) begins every file in an SC2Map
  // archive, to help verify its contents
  if( buffer[0] != magic[0] ||
      buffer[1] != magic[1] ||
      buffer[2] != magic[2] ||
      buffer[3] != magic[3] )
  {
    return -1;
  }

  return 0;
}



int SC2Map::readGameStrings_txt( const HANDLE archive )
{
  string gameStrings;
  string suffix( ".SC2Data\\LocalizedData\\GameStrings.txt" );

  list<string>::iterator itr;

  bool localeFound = false;

  // first check local user config
  itr = configUserLocal.localePreferences.begin();
  while( itr != configUserLocal.localePreferences.end() && !localeFound )
  {
    gameStrings.assign( *itr );
    gameStrings.append( suffix );

    if( SFileHasFile( archive, gameStrings.data() ) )
    {
      // gameStrings now holds name of valid file in archive
      localeFound = true;
    }

    ++itr;
  }

  // then global user config
  itr = configUserGlobal.localePreferences.begin();
  while( itr != configUserGlobal.localePreferences.end() && !localeFound )
  {
    gameStrings.assign( *itr );
    gameStrings.append( suffix );

    if( SFileHasFile( archive, gameStrings.data() ) )
    {
      // gameStrings now holds name of valid file in archive
      localeFound = true;
    }

    ++itr;
  }

  // last resort is global internal config
  itr = configInternal.localePreferences.begin();
  while( itr != configInternal.localePreferences.end() && !localeFound )
  {
    gameStrings.assign( *itr );
    gameStrings.append( suffix );

    if( SFileHasFile( archive, gameStrings.data() ) )
    {
      // gameStrings now holds name of valid file in archive
      localeFound = true;
    }

    ++itr;
  }

  int bufferSize;
  u8* bufferFreeAfterUse;

  if( readArchiveFile( archive, gameStrings.data(), &bufferSize, &bufferFreeAfterUse ) < 0 )
  {
    return -1;
  }

  // this is a plain text file, so copy the data into a null-terminated character
  // buffer so we can use string searching on it
  char bufferString[bufferSize + 1];
  memcpy( bufferString, bufferFreeAfterUse, bufferSize );
  bufferString[sizeof( bufferString ) - 1] = '\0';

  // release the buffer with the file's data
  delete bufferFreeAfterUse;

  // try to find the map's name here, otherwise use the
  // filename--the goal is to name the output files
  char strMapName[FILENAME_LENGTH];

  char attributeName[] = "DocInfo/Name=";

  char* findAttribute = strstr( bufferString, attributeName );

  if( findAttribute == NULL )
  {
    // return -1 because we should choose another name
    return -1;
  }

  // we did find a name
  int nameStartIndex = (int)findAttribute -
                       (int)bufferString  +
                       strlen( attributeName );
  int i = 0;
  while( nameStartIndex + i < bufferSize &&
         bufferString[nameStartIndex + i] != '\r' )
  {
    strMapName[i] = bufferString[nameStartIndex + i];
    ++i;
  }
  strMapName[i] = '\0';

  // put it in a nice string and done!
  mapName.assign( strMapName );

  return 0;
}


void SC2Map::makeMapNameValidForFilenames()
{
  mapNameInOutputFiles.assign( mapName );

  // remove invalid filename characters
  removeChars( "\\<>:\"/|?*\t\r\n ", &mapNameInOutputFiles );

  // gotta start with an alpha character
  if( !isalpha( mapNameInOutputFiles[0] ) )
  {
    string prefix( "sc2map" );
    prefix.append( mapNameInOutputFiles );
    mapNameInOutputFiles.assign( prefix );
  }

  set<string>::iterator sItr = mapFilenamesUsed.find( mapNameInOutputFiles );
  if( sItr != mapFilenamesUsed.end() )
  {
    int    number = 2;
    string alternateName;
    do
    {
      char alt[FILENAME_LENGTH];
      sprintf( alt, "%s%d", mapNameInOutputFiles.data(), number );
      alternateName.assign( alt );

      ++number;

      sItr = mapFilenamesUsed.find( alternateName );
    } while( sItr != mapFilenamesUsed.end() );

    printWarning( "Map name %s already used, using %s instead.\n",
                  mapNameInOutputFiles.data(),
                  alternateName.data() );

    mapNameInOutputFiles.assign( alternateName );
  }

  mapFilenamesUsed.insert( mapNameInOutputFiles );
}



int SC2Map::readMapInfo( const HANDLE archive )
{
  if( debugMapInfo > 0 )
  {
    printMessage( "Debugging MapInfo\n" );
  }

  char strFilename[] = "MapInfo";

  int bufferSize;
  u8* bufferFreeAfterUse;

  if( readArchiveFile( archive, strFilename, &bufferSize, &bufferFreeAfterUse ) < 0 )
  {
    return -1;
  }

  bool continueProcessing = true;

  // before continuing, verify a minimum size in bytes
  // we are expecting until the variable-length strings
  int expectedSize =
    4+ // magic
    4+ // file version
    4+ // width
    4+ // height
    4+ // unknown
    4; // unknown

  // the file may still be invalid, but don't even
  // proceed if there isn't already this much--it
  // helps from having to check how many bytes are
  // left in all the following processing steps
  if( bufferSize < expectedSize )
  {
    printWarning( "%s is invalid.\n", strFilename );
    continueProcessing = false;
  }

  // position in the data buffer
  int buffpos = 0;

  if( continueProcessing )
  {
    // magic word to help verify file contents, always at file offset 0
    u8 magic[] = { 'I', 'p', 'a', 'M' };

    if( verifyMagicWord( magic, bufferFreeAfterUse ) < 0 )
    {
      printWarning( "Incorrect magic word for %s.\n", strFilename );
      continueProcessing = false;
    }

    buffpos += 4;
  }

  // raw data to read in
  u32 fileVersion;
  u32 mcWidth;
  u32 mcHeight;
  u32 mcLeft;
  u32 mcBottom;
  u32 mcRight;
  u32 mcTop;

  if( continueProcessing )
  {
    // ignore file version (word)
    memcpy( (void*) &fileVersion, (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;
    if( debugMapInfo > 0 )
    {
      printMessage( "File version is %d.\n", fileVersion );
    }


    // two new words in version 24?
    if( fileVersion == 24 )
    {
      buffpos += 8;
    }


    // width and height of map (word)
    memcpy( (void*) &mcWidth,  (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;
    memcpy( (void*) &mcHeight, (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;

    if( mcWidth > 256 || mcHeight > 256 )
    {
      printWarning( "Unexpected format for %s, map size appears to be (%d, %d).\n",
                    strFilename,
                    mcWidth,
                    mcHeight );
      continueProcessing = false;
    }
  }


  if( continueProcessing )
  {
    // ignore two unknown words
    buffpos += 8;

    // This section has been troublesome--sometimes a group of
    // zero bytes preceeds the first of two strings, and the
    // string sequence is as follows:

    // now there are two char strings of unknown length--starting
    // from the first string's offset look for a zero byte to
    // terminate the first string, and then a zero byte to terminate
    // the second string, and read more info from there

    // SO, first walk over any zero bytes before walking the strings
    while( buffpos < bufferSize && bufferFreeAfterUse[buffpos] == 0 )
    {
      ++buffpos;
    }

    // then walk the first string
    while( buffpos < bufferSize && bufferFreeAfterUse[buffpos] != 0 )
    {
      ++buffpos;
    }

    // advance past the termination character
    ++buffpos;

    // find terminator of second string
    while( buffpos < bufferSize && bufferFreeAfterUse[buffpos] != 0 )
    {
      ++buffpos;
    }

    // advance past the termination character
    ++buffpos;

    // there should be enough leftover data for 4 4-byte words
    if( bufferSize - buffpos < 16 )
    {
      printWarning( "Unexpected end-of-file in %s.\n", strFilename );
      continueProcessing = false;
    }
  }

  if( continueProcessing )
  {
    // use variable filepos to align the rest of the data
    memcpy( (void*) &mcLeft,   (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;
    memcpy( (void*) &mcBottom, (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;
    memcpy( (void*) &mcRight,  (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;
    memcpy( (void*) &mcTop,    (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;

    // there is a bunch more data in this file, but ignoring it

    if( mcLeft   > mcRight ||
        mcRight  > mcWidth ||
        mcBottom > mcTop   ||
        mcTop    > mcHeight )
    {
      printWarning( "Unexpected format for %s--likely an unknown, new version.\n", strFilename );
      continueProcessing = false;
    }
  }

  // whether or not we continued processing we have to
  // release the buffer with the file's data
  delete bufferFreeAfterUse;

  if( continueProcessing )
  {
    // the image frame constants are already defined,
    // (not read in by this function) but pass them
    // in now to set up translation so we can start
    // using points
    point::setFrameTranslationConstants( mcLeft,
                                         mcBottom,
                                         iDimT,
                                         iDimC );
    cxDimMap = mcWidth;
    cyDimMap = mcHeight;
    txDimMap = cxDimMap + 1;
    tyDimMap = cyDimMap + 1;

    cxDimPlayable = mcRight - mcLeft;
    cyDimPlayable = mcTop   - mcBottom;
    txDimPlayable = cxDimPlayable + 1;
    tyDimPlayable = cyDimPlayable + 1;

    // set the coordinates of the playable area in cell
    // coordinates, which they are natively given in
    cLeftBottom.mcSet( mcLeft,  mcBottom );
    cRightTop  .mcSet( mcRight, mcTop    );

    // this is a purposeful translation of cell coordinates
    // to terrain coordinates so we know the playable terrain
    tLeftBottom.mtSet( mcLeft,      mcBottom     );
    tRightTop  .mtSet( mcWidth + 1, mcHeight + 1 );

    // if we processed to the end with no problem,
    // report no problem
    return 0;
  }

  // otherwise report error
  return -1;
}



int SC2Map::readt3HeightMap( const HANDLE archive )
{
  char strFilename[] = "t3HeightMap";

  int bufferSize;
  u8* bufferFreeAfterUse;

  if( readArchiveFile( archive, strFilename, &bufferSize, &bufferFreeAfterUse ) < 0 )
  {
    return -1;
  }

  bool continueProcessing = true;

  // before continuing, verify a minimum size in bytes
  // we are expecting for this file
  int expectedSize =
    4+ // magic
    4+ // file version
    4+ // width
    4+ // height
    16;// unknown

  // the file may still be invalid, but don't even
  // proceed if there isn't already this much--it
  // helps from having to check how many bytes are
  // left in all the following processing steps
  if( bufferSize < expectedSize )
  {
    printWarning( "%s is invalid.\n", strFilename );
    continueProcessing = false;
  }

  // position in the data buffer
  int buffpos = 0;

  if( continueProcessing )
  {
    // magic word to help verify file contents, always at file offset 0
    u8 magic[] = { 'H', 'M', 'A', 'P' };

    if( verifyMagicWord( magic, bufferFreeAfterUse ) < 0 )
    {
      printWarning( "Incorrect magic word for %s.\n", strFilename );
      continueProcessing = false;
    }

    buffpos += 4;
  }

  // raw data to read in
  u32 mtWidth;
  u32 mtHeight;

  if( continueProcessing )
  {
    // ignore file version (word)
    buffpos += 4;

    // width and height of map (word)
    memcpy( (void*) &mtWidth,  (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;
    memcpy( (void*) &mtHeight, (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;

    if( mtWidth != txDimMap || mtHeight != tyDimMap )
    {
      // we are trying to read in the map of height data, and it should
      // match the dimensions we read in from MapInfo
      printWarning( "Terrain dimensions %dx%d in %s do not match %dx%d from MapInfo.\n",
                    mtWidth, mtHeight,
                    strFilename,
                    txDimMap, tyDimMap );
      continueProcessing = false;
    }

    // ignore 16 unknown bytes
    buffpos += 16;
  }

  if( continueProcessing )
  {
    // we are expecting the terrain map to have 6 bytes per terrain unit,
    // check that there is enough remaining data
    expectedSize = mtWidth*mtHeight*6;

    int bytesLeft = bufferSize - buffpos;

    if( bytesLeft < expectedSize )
    {
      printWarning( "Unexpected end-of-file in %s.\n", strFilename );
      continueProcessing = false;

    } else if( bytesLeft != bufferSize - buffpos ) {
      printWarning( "More data than expected in %s, continuing anyway.\n", strFilename );
    }
  }

  if( continueProcessing )
  {
    // the data for analysis only includes the
    // playable area, but...
    mapHeight = new u8[txDimPlayable*tyDimPlayable];

    // we gotta scan the whole file to get what we want
    for( int mtj = 0; mtj < tyDimMap; ++mtj )
    {
      for( int mti = 0; mti < txDimMap; ++mti )
      {
        // height is the 5th byte of each 6-byte chunk
        u8 h = bufferFreeAfterUse[buffpos + 4]; buffpos += 6;

        if( h > 0x3 )
        {
          // only 0x0, 0x1, 0x2 and 0x3 are valid
          printWarning( "Unexpected value %u in %s\n", h, strFilename );
          continueProcessing = false;
          break;
        }

        point t;
        t.mtSet( mti, mtj );
        if( isPlayableTerrain( &t ) )
        {
          setHeight( &t, h );
        }
      }

      if( !continueProcessing )
      {
        break;
      }
    }
  }

  // whether or not we continued processing we have to
  // release the buffer with the file's data
  delete bufferFreeAfterUse;

  if( continueProcessing )
  {
    // if we processed to the end with no problem,
    // report no problem
    return 0;
  }

  // otherwise report error
  return -1;
}


void SC2Map::setHeight( point* t, u8 h )
{
  if( !isPlayableTerrain( t ) )
  {
    printError( "Attempt to access data from unplayable terrain (%d, %d)\n",
                t->ptx,
                t->pty );
    exit( -1 );
  }
  mapHeight[t->pty*txDimPlayable + t->ptx] = h;
}


u8 SC2Map::getHeight( point* t ) {

  if( !isPlayableTerrain( t ) )
  {
    printError( "Attempt to access data from unplayable terrain (%d, %d)\n",
                t->ptx,
                t->pty );
    exit( -1 );
  }
  
  if( t->ptx == point::ptNaN ) {
    printError( "Expected a terrain point.\n" );
    exit( -1 );
  }
  
  return mapHeight[t->pty*txDimPlayable + t->ptx];
}


u8 SC2Map::getHeightCell( point* c ) {

  if( !isPlayableCell( c ) )
  {
    printError( "Attempt to access data from unplayable cell (%d, %d)\n",
                c->pcx,
                c->pcy );
    exit( -1 );
  }
  
  if( c->pcx == point::pcNaN ) {
    printError( "Expected a cell point.\n" );
    exit( -1 );
  }

  point t_ur;  t_ur.ptSet( c->pcx + 1, c->pcy + 1 );
  point t_lr;  t_lr.ptSet( c->pcx + 1, c->pcy     );
  point t_ll;  t_ll.ptSet( c->pcx,     c->pcy     );
  point t_ul;  t_ul.ptSet( c->pcx,     c->pcy + 1 );

  // this isn't a well-defined concept for cells with
  // a bunch of neighboring heights, which is ok because
  // those kinds of cells don't usually render any interesting
  // game info because they are unpathable
  return (getHeight( &t_ur ) +
          getHeight( &t_lr ) +
          getHeight( &t_ll ) +
          getHeight( &t_ul )) / 4;
}
      


int SC2Map::readPaintedPathingLayer( const HANDLE archive )
{
  char strFilename[] = "PaintedPathingLayer";

  int bufferSize;
  u8* bufferFreeAfterUse;

  if( readArchiveFile( archive, strFilename, &bufferSize, &bufferFreeAfterUse ) < 0 )
  {
    return -1;
  }

  bool continueProcessing = true;

  int expectedSize = 16 + 8*256*256;
  if( bufferSize < expectedSize )
  {
    printWarning( "%s is invalid.\n", strFilename );
    continueProcessing = false;
  }


  // position in the data buffer, skip opening words
  int buffpos = 16;

  for( int i = 0; i < 8*256*256; ++i )
  {
    bool breakFromLoop = false;

    u8 type;
    memcpy( (void*) &type, (void*) &(bufferFreeAfterUse[buffpos]), 1 ); buffpos += 1;

    point c;

    int mcy = i / (8*256);
    int rmn = i % (4*256);
    int mcx = rmn / 4;

    c.mcSet( mcx, mcy );

    if( !isPlayableCell( &c ) )
    {
      continue; // just skip
    }


    switch( type )
    {
      case 0x80: // no painted pathing
      break;

      case 0x00: // no ground/cwalk/build pathing!
        for( int t = 0; t < NUM_PATH_TYPES; ++t )
        {
          setPathing( &c, (PathType)t, false );
        }
      break;

      case 0x01: // YES ground (overrides!)
        for( int t = 0; t < NUM_PATH_TYPES; ++t )
        {
          setPathing( &c, (PathType)t, true );
        }
      break;

      case 0x81: // no building
        setPathing( &c, PATH_BUILDABLE,      false );
        setPathing( &c, PATH_BUILDABLE_MAIN, false );
      break;

      default:
        printError( "Unknown triangle type in %s.\n", strFilename );
        breakFromLoop = true;
    }


    if( breakFromLoop )
    {
      continueProcessing = false;
      break;
    }
  }


  delete bufferFreeAfterUse;

  if( continueProcessing )
  {
    // if we processed to the end with no problem,
    // report no problem
    return 0;
  }

  // otherwise report error
  return -1;
}








int SC2Map::readObjects( const HANDLE archive )
{
  if( debugObjects > 0 )
  {
    printMessage( "Debugging Objects.\n" );
  }

  char strFilename[] = "Objects";

  int bufferSize;
  u8* bufferFreeAfterUse;

  if( readArchiveFile( archive, strFilename, &bufferSize, &bufferFreeAfterUse ) < 0 )
  {
    return -1;
  }

  bool continueProcessing = true;

  // we need a FILE* instead of data buffer for tinyxml lib
  char strTemp[] = "Objects.tmp";
  FILE* fileTemp = fopen( strTemp, "w+" );

  if( fileTemp == NULL )
  {
    printError( "Could not open temp file for %s.\n", strFilename );
    continueProcessing = false;
  }

  if( continueProcessing )
  {
    int bytesWritten = fwrite( (char*)bufferFreeAfterUse, 1, bufferSize, fileTemp );
    if( bytesWritten != bufferSize )
    {
      printError( "Could not dump %s to temp file.\n", strFilename );
      continueProcessing = false;
    }

    // from here on out a temporary file holds the XML
    // so after processing delete the file
    fclose( fileTemp );
  }

  // whether or not we continue processing we have to
  // release the buffer with the file's data
  delete bufferFreeAfterUse;


  bool continueXML = false;
  if( continueProcessing )
  {
    continueXML = true;
  }

  TiXmlDocument doc;

  if( continueXML )
  {
    // open temp file as XML stream
    if( !doc.LoadFile( strTemp ) )
    {
      printError( "Could not open temp file for %s.\n", strFilename );
      continueXML = false;
    }
  }

  TiXmlHandle docHandle( &doc );
  TiXmlElement* placedObjects = NULL;

  if( continueXML )
  {
    placedObjects = docHandle
      .FirstChildElement( "PlacedObjects" )
      .ToElement();
    if( !placedObjects )
    {
      printWarning( "%s is not valid.\n", strFilename );
      continueXML = false;
    }
  }

  ObjectMode objMode;

  if( continueXML )
  {
    const char* strVersion;

    // try the OBJMODE_BETAPH1_v26 format first
    const char* strVersion1 = placedObjects->Attribute( "version" );
    if( strVersion1 != NULL )
    {
      strVersion = strVersion1;
      objMode    = OBJMODE_BETAPH1_v26;

    } else {
      const char* strVersion2 = placedObjects->Attribute( "Version" );
      if( strVersion2 != NULL )
      {
        strVersion = strVersion2;
        objMode    = OBJMODE_BETAPH2_v26;
      }
    }

    if( strVersion == NULL )
    {
      printWarning( "%s has an unknown version.\n", strFilename );
      continueXML = false;
    }
    else if( strcmp( strVersion, "26" ) != 0 )
    {
      printWarning( "%s is version %s, file version 26 expected.\n", strFilename, strVersion );
    }
  }

  if( continueXML )
  {
    for( TiXmlElement* object = placedObjects->FirstChildElement();
         object;
         object = object->NextSiblingElement() )
    {
      // in placedobjects.cpp
      processPlacedObject( objMode, object );
    }
  }

  // delete the temporary file
  char cmd[512];
  sprintf( cmd, "if exist %s del %s", strTemp, strTemp );
  system( cmd );

  if( continueProcessing && continueXML )
  {
    return 0;
  }

  return -1;
}


int SC2Map::readt3CellFlags( const HANDLE archive )
{
  char strFilename[] = "t3CellFlags";

  int bufferSize;
  u8* bufferFreeAfterUse;

  if( readArchiveFile( archive, strFilename, &bufferSize, &bufferFreeAfterUse ) < 0 )
  {
    return -1;
  }

  bool continueProcessing = true;

  // before continuing, verify a minimum size in bytes
  // we are expecting for this file
  int expectedSize =
    4+ // magic
    4+ // file version
    16+// unknown
    4+ // width
    4; // height

  // the file may still be invalid, but don't even
  // proceed if there isn't already this much--it
  // helps from having to check how many bytes are
  // left in all the following processing steps
  if( bufferSize < expectedSize )
  {
    printWarning( "%s is invalid.\n", strFilename );
    continueProcessing = false;
  }

  int buffpos = 0;

  if( continueProcessing )
  {
    // magic word to help verify file contents, always at file offset 0
    u8 magic[] = { 'L', 'F', 'C', 'T' };

    if( verifyMagicWord( magic, bufferFreeAfterUse ) < 0 )
    {
      printWarning( "Incorrect magic word for %s.\n", strFilename );
      continueProcessing = false;
    }

    buffpos += 4;
  }

  // raw data to read in
  u32 mcWidth;
  u32 mcHeight;

  if( continueProcessing )
  {
    // ignore file version (word) and an unknown 16 bytes
    buffpos += 20;

    // width and height of map (word)
    memcpy( (void*) &mcWidth,  (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;
    memcpy( (void*) &mcHeight, (void*) &(bufferFreeAfterUse[buffpos]), 4 ); buffpos += 4;

    if( mcWidth != cxDimMap || mcHeight != cyDimMap )
    {
      // we are trying to read in the map of height data, and it should
      // match the dimensions we read in from MapInfo
      printWarning( "Cell dimensions %dx%d in %s do not match %dx%d from MapInfo.\n",
                    mcWidth, mcHeight,
                    strFilename,
                    cxDimMap, cyDimMap );
      continueProcessing = false;
    }
  }

  if( continueProcessing )
  {
    // we are expecting the cell map to have 1 byte per cell,
    // check that there is enough remaining data
    expectedSize = mcWidth*mcHeight;

    int bytesLeft = bufferSize - buffpos;

    if( bytesLeft < expectedSize )
    {
      printWarning( "Unexpected end-of-file in %s.\n", strFilename );
      continueProcessing = false;

    } else if( bytesLeft != bufferSize - buffpos ) {
      printWarning( "More data than expected in %s, continuing anyway.\n", strFilename );
    }
  }

  if( continueProcessing )
  {
    // an intermediate structure to get pathing right
    mapCliffChanges = new bool[(cxDimMap/2)*(cyDimMap/2)];
    for( int i = 0; i < (cxDimMap/2)*(cyDimMap/2); ++i )
    {
      mapCliffChanges[i] = false;
    }

    // scan the whole map
    for( int mcj = 0; mcj < cyDimMap; ++mcj )
    {
      for( int mci = 0; mci < cxDimMap; ++mci )
      {
        u8 flags = bufferFreeAfterUse[buffpos]; buffpos += 1;

        // take the 2nd LSB as indicator for not-pathable
        // so flip flags first to get "pathable by ground"
        bool groundPathable = ~flags & 0x1;

        point c;
        c.mcSet( mci, mcj );

        setMapCliffChange( &c, groundPathable );
      }
    }
  }


  // whether or not we continued processing we have to
  // release the buffer with the file's data
  delete bufferFreeAfterUse;


  if( continueProcessing )
  {
    computePathingFromCliffChanges();

    // if we processed to the end with no problem,
    // report no problem
    return 0;
  }

  // otherwise report error
  return -1;
}


// this file has a bunch of cosmetic stuff we don't need, but it
// does have ramps so skip right to them
int SC2Map::readt3Terrain_xml( const HANDLE archive )
{
  char strFilename[] = "t3Terrain.xml";

  int bufferSize;
  u8* bufferFreeAfterUse;

  if( readArchiveFile( archive, strFilename, &bufferSize, &bufferFreeAfterUse ) < 0 )
  {
    return -1;
  }

  bool continueProcessing = true;

  // we need a FILE* instead of data buffer for tinyxml lib
  char strTemp[] = "t3Terrain.tmp";
  FILE* fileTemp = fopen( strTemp, "w+" );

  if( fileTemp == NULL )
  {
    printError( "Could not open temp file for %s.\n", strFilename );
    continueProcessing = false;
  }

  if( continueProcessing )
  {
    int bytesWritten = fwrite( (char*)bufferFreeAfterUse, 1, bufferSize, fileTemp );
    if( bytesWritten != bufferSize )
    {
      printError( "Could not dump %s to temp file.\n", strFilename );
      continueProcessing = false;
    }

    // from here on out a temporary file holds the XML
    // so after processing delete the file
    fclose( fileTemp );
  }

  // whether or not we continue processing we have to
  // release the buffer with the file's data
  delete bufferFreeAfterUse;


  bool continueXML = false;
  if( continueProcessing )
  {
    continueXML = true;
  }


  TiXmlDocument doc;

  if( continueXML )
  {
    // open temp file as XML stream
    if( !doc.LoadFile( strTemp ) )
    {
      printError( "Could not open temp file for %s.\n", strFilename );
      continueXML = false;
    }
  }

  TiXmlHandle docHandle( &doc );
  TiXmlElement* terrain = NULL;

  if( continueXML )
  {
    terrain = docHandle
      .FirstChildElement( "terrain" )
      .ToElement();
    if( !terrain )
    {
      printWarning( "%s has no terrain element.\n", strFilename );
      continueXML = false;
    }
  }

  if( continueXML )
  {
    const char* strVersion = terrain->Attribute( "version" );
    if( strcmp( strVersion, "112" ) != 0 &&
        strcmp( strVersion, "113" ) != 0 )
    {
      printWarning( "%s is version %s, file version 112 or 113 expected.\n", strFilename, strVersion );
      continueXML = false;
    }
  }

  TiXmlElement* heightMap = NULL;
  const char* strDim;

  if( continueXML )
  {
    heightMap = terrain->FirstChildElement( "heightMap" );

    if( heightMap == NULL )
    {
      printWarning( "%s has no heightMap element.\n", strFilename );
      continueXML = false;
    }
  }

  if( continueXML )
  {
    strDim = heightMap->Attribute( "dim" );
    if( strDim == NULL )
    {
      printWarning( "%s does not specify dimensions for the heightMap.\n", strFilename );
      continueXML = false;
    }
  }

  if( continueXML )
  {
    u32 mtWidth;
    u32 mtHeight;

    sscanf( strDim, "%u %u", &mtWidth, &mtHeight );

    if( mtWidth != txDimMap || mtHeight != tyDimMap )
    {
      // we are trying to read in the map of height data, and it should
      // match the dimensions we read in from MapInfo
      printWarning( "Terrain dimensions %dx%d in %s do not match %dx%d from MapInfo.\n",
                    mtWidth, mtHeight,
                    strFilename,
                    txDimMap, tyDimMap );
      continueXML = false;
    }
  }



  TiXmlElement* rampList = NULL;

  if( continueXML )
  {
    rampList = heightMap->FirstChildElement( "rampList" );

    if( rampList == NULL )
    {
      // don't warn about this, if there's no list, no problem!
      continueXML = false;
    }
  }

  if( continueXML )
  {
    for( TiXmlElement* ramp = rampList->FirstChildElement();
         ramp;
         ramp = ramp->NextSiblingElement() )
    {
      processRamp( ramp );
    }
  }

  // delete the temporary file
  char cmd[512];
  sprintf( cmd, "if exist %s del %s", strTemp, strTemp );
  system( cmd );

  if( continueProcessing && continueXML )
  {
    return 0;
  }

  return -1;
}
