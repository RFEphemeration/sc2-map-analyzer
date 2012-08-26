#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "utility.hpp"
#include "outstreams.hpp"

static bool warningOrErrorReported = false;

static bool printMessages = true;
static bool printWarnings = true;
static bool printErrors   = true;

static int numWarnings = 0;
static int numErrors   = 0;


void printOutstreamStatus()
{
  printf( "\n%d warnings and %d errors were reported.\n",
          numWarnings,
          numErrors );
  fflush( stdout );
}


void printMessage( const char* format, ... )
{
  if( !printMessages ) { return; }
  va_list argptr;
  va_start( argptr, format );
  vfprintf( stdout, format, argptr );
  va_end( argptr );
  fflush( stdout );
  warningOrErrorReported = true;
}

void printWarning( const char* format, ... )
{
  ++numWarnings;
  if( !printWarnings ) { return; }
  fprintf( stderr, "Warning: " );
  va_list argptr;
  va_start( argptr, format );
  vfprintf( stderr, format, argptr );
  va_end( argptr );
  fflush( stderr );
}

void printError( const char* format, ... )
{
  ++numErrors;
  if( !printErrors ) { return; }
  fprintf( stderr, "ERROR: " );
  va_list argptr;
  va_start( argptr, format );
  vfprintf( stderr, format, argptr );
  va_end( argptr );
  fprintf( stderr,
           "Help improve the map analyzer: send map files that cause errors to %s.\n",
           adminEmail );
  fflush( stderr );
  warningOrErrorReported = true;
}

void enableMessages() { printMessages = true;  }
void enableWarnings() { printWarnings = true;  }
void enableErrors()   { printErrors   = true;  }
void disableMessages(){ printMessages = false; }
void disableWarnings(){ printWarnings = false; }
void disableErrors()  { printErrors   = false; }
