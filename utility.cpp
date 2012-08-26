#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "outstreams.hpp"
#include "utility.hpp"
#include "sc2mapTypes.hpp"


const char* projectURL = "http://www.sc2mapster.com/assets/sc2-map-analyzer";
const char* adminEmail = "dimfish.mapper@gmail.com";



void removeChars( const char* chars, string* s )
{
  string::size_type i = s->find_first_of( chars );

  while( i != string::npos )
  {
    s->erase( i, 1 );
    i = s->find_first_of( chars, i );
  }
}


void replaceChars( const char* chars, const char* with, string* s )
{
  string::size_type i = s->find_first_of( chars );

  while( i != string::npos )
  {
    s->erase( i, 1 );

    s->insert( i, with );

    i = s->find_first_of( chars, i + strlen( with ) );
  }
}


void makeLower( string* s )
{
  string lowered;

  for( int i = 0; i < s->size(); ++i )
  {
    char c = s->at( i );

    if( isupper( c ) )
    {
      lowered.append( 1, tolower( c ) );
    } else {
      lowered.append( 1, c );
    }
  }

  s->assign( lowered );
}


void trimLeadingSpaces( string* s )
{
  while( s->length() > 0 && s->at( 0 ) == ' ' )
  {
    s->erase( 0, 1 );
  }
}


void trimTrailingSpaces( string* s )
{
  while( s->length() > 0 && s->at( s->length() - 1 ) == ' ' )
  {
    s->erase( s->length() - 1, 1 );
  }
}


float xtof_helper( char c )
{
  if( isdigit( c ) )
  {
    return (float) (c-48);
  }

  switch( c )
  {
    case 'a': case 'A': return 10.0f;
    case 'b': case 'B': return 11.0f;
    case 'c': case 'C': return 12.0f;
    case 'd': case 'D': return 13.0f;
    case 'e': case 'E': return 14.0f;
    case 'f': case 'F': return 15.0f;
  }
}

float xtof( char c1, char c0 )
{
  if( !isxdigit( c1 ) ) { printError( "'%c' is not a hex digit.\n", c1 ); exit( -1 ); }
  if( !isxdigit( c0 ) ) { printError( "'%c' is not a hex digit.\n", c0 ); exit( -1 ); }

  float f = 16.0f*xtof_helper( c1 ) + xtof_helper( c0 );

  return f / 255.0f;
}


void formatPath( string* s )
{
  // paths should have
  // replace all forward slashes with backslashes to
  // ensure consistent path characters
  size_t lookHere = 0;
  size_t foundHere;
  while( (foundHere = s->find( "/", lookHere ))
         != string::npos )
  {
    s->replace( foundHere, 1, "\\" );
    lookHere = foundHere + 1;
  }

  // if the filename ends in a "\" we will interpret
  // it as a directory, but the trailing slash wigs out the
  // call to stat so trim any here
  while( s->size() > 0 &&
         s->at( s->size() - 1 ) == '\\' )
  {
    s->erase( s->size() - 1 );
  }
}


float snapIfNearInt( float x )
{
  // if a float is within 0.1 of an integer
  // value, snap it to the integer

  if( fabs( x - ceil( x ) ) < 0.1 )
  {
    return ceil( x );
  }

  if( fabs( x - floor( x ) ) < 0.1 )
  {
    return floor( x );
  }

  return x;
}


float p2pDistance( point* p1, point* p2 )
{
  return sqrt( (p1->mx - p2->mx)*(p1->mx - p2->mx) +
               (p1->my - p2->my)*(p1->my - p2->my) );
}


float alterSaturationComponent( float zeroToOne, float component ) {
  float dGrey = component - 0.5f;
  return zeroToOne*dGrey + 0.5f;
}

void alterSaturation( float zeroToOne, Color* c ) {
  c->r = alterSaturationComponent( zeroToOne, c->r );
  c->g = alterSaturationComponent( zeroToOne, c->g );
  c->b = alterSaturationComponent( zeroToOne, c->b );
}

void mixWith( Color* changing, Color* mixer ) {
  changing->r = 0.5f * (changing->r + mixer->r);
  changing->g = 0.5f * (changing->g + mixer->g);
  changing->b = 0.5f * (changing->b + mixer->b);
}



void gradient( float  zeroToOne,
               Color* c1,
               Color* c2,
               Color* c3,
               Color* cOut )
{
  // there are 2 gradients between 3 colors
  float d = 2.0f * zeroToOne;

  Color* low;
  Color* high;

  if( d < 0.0f ) {
    // min out at low color
    low = c1;
    cOut->r = low->r;
    cOut->g = low->g;
    cOut->b = low->b;
    return;

  } else if( d < 1.0f ) {
    low  = c1;
    high = c2;

  } else if( d < 2.0f ) {
    low  = c2;
    high = c3;
    d -= 1.0f;

  } else {
    // max out at high color
    high = c3;
    cOut->r = high->r;
    cOut->g = high->g;
    cOut->b = high->b;
    return;
  }

  // otherwise use 0.0 <= d <= 1.0 to calculate
  // a gradient between the chosen low/high color
  cOut->r = low->r + d*(high->r - low->r);
  cOut->g = low->g + d*(high->g - low->g);
  cOut->b = low->b + d*(high->b - low->b);
}


void gradient( float  zeroToOne,
               Color* c1,
               Color* c2,
               Color* c3,
               Color* c4,
               Color* cOut )
{
  // there are 3 gradients between 4 colors
  float d = 3.0f * zeroToOne;

  Color* low;
  Color* high;

  if( d < 0.0f ) {
    // min out at low color
    low = c1;
    cOut->r = low->r;
    cOut->g = low->g;
    cOut->b = low->b;
    return;

  } else if( d < 1.0f ) {
    low  = c1;
    high = c2;

  } else if( d < 2.0f ) {
    low  = c2;
    high = c3;
    d -= 1.0f;

  } else if( d < 3.0f ) {
    low  = c3;
    high = c4;
    d -= 2.0f;

  } else {
    // max out at high color
    high = c4;
    cOut->r = high->r;
    cOut->g = high->g;
    cOut->b = high->b;
    return;
  }

  // otherwise use 0.0 <= d <= 1.0 to calculate
  // a gradient between the chosen low/high color
  cOut->r = low->r + d*(high->r - low->r);
  cOut->g = low->g + d*(high->g - low->g);
  cOut->b = low->b + d*(high->b - low->b);
}


void gradient( float  zeroToOne,
               Color* c1,
               Color* c2,
               Color* c3,
               Color* c4,
               Color* c5,
               Color* c6,
               Color* c7,
               Color* cOut )
{
  // there are 6 gradients between 7 colors
  float d = 6.0f * zeroToOne;

  Color* low;
  Color* high;

  if( d < 0.0f ) {
    // min out at low color
    low = c1;
    cOut->r = low->r;
    cOut->g = low->g;
    cOut->b = low->b;
    return;

  } else if( d < 1.0f ) {
    low  = c1;
    high = c2;

  } else if( d < 2.0f ) {
    low  = c2;
    high = c3;
    d -= 1.0f;

  } else if( d < 3.0f ) {
    low  = c3;
    high = c4;
    d -= 2.0f;

  } else if( d < 4.0f ) {
    low  = c4;
    high = c5;
    d -= 3.0f;

  } else if( d < 5.0f ) {
    low  = c5;
    high = c6;
    d -= 4.0f;

  } else if( d < 6.0f ) {
    low  = c6;
    high = c7;
    d -= 5.0f;

  } else {
    // max out at high color
    high = c7;
    cOut->r = high->r;
    cOut->g = high->g;
    cOut->b = high->b;
    return;
  }

  // otherwise use 0.0 <= d <= 1.0 to calculate
  // a gradient between the chosen low/high color
  cOut->r = low->r + d*(high->r - low->r);
  cOut->g = low->g + d*(high->g - low->g);
  cOut->b = low->b + d*(high->b - low->b);
}



void gradient( float  zeroToOne,
               Color* c1,
               Color* c2,
               Color* c3,
               Color* c4,
               Color* c5,
               Color* c6,
               Color* c7,
               Color* c8,
               Color* c9,
               Color* c10,
               Color* c11,
               Color* cOut )
{
  // there are 10 gradients between 11 colors, using
  // a non-uniform scale (because middle gradients
  // are stretch across bigger portions of the map,
  // generally
  //
  //  0    6   11   15   18   20   22   25   29   34   40
  //  * 6  * 5  * 4  * 3  * 2  * 2  * 3  * 4  * 5  * 6  *
  //  ^
  //  |
  // 1st color
  float d = 40.0f * zeroToOne;

  Color* low;
  Color* high;

  if( d < 0.0f ) {
    // min out at low color
    low = c1;
    cOut->r = low->r;
    cOut->g = low->g;
    cOut->b = low->b;
    return;

  } else if( d < 6.0f ) {
    low  = c1;
    high = c2;
    d = (d - 0.0f) / 6.0f;

  } else if( d < 11.0f ) {
    low  = c2;
    high = c3;
    d = (d - 6.0f) / 5.0f;

  } else if( d < 15.0f ) {
    low  = c3;
    high = c4;
    d = (d - 11.0f) / 4.0f;

  } else if( d < 18.0f ) {
    low  = c4;
    high = c5;
    d = (d - 15.0f) / 3.0f;

  } else if( d < 20.0f ) {
    low  = c5;
    high = c6;
    d = (d - 18.0f) / 2.0f;

  } else if( d < 22.0f ) {
    low  = c6;
    high = c7;
    d = (d - 20.0f) / 2.0f;

  } else if( d < 25.0f ) {
    low  = c7;
    high = c8;
    d = (d - 22.0f) / 3.0f;

  } else if( d < 29.0f ) {
    low  = c8;
    high = c9;
    d = (d - 25.0f) / 4.0f;

  } else if( d < 34.0f ) {
    low  = c9;
    high = c10;
    d = (d - 29.0f) / 5.0f;

  } else if( d < 40.0f ) {
    low  = c10;
    high = c11;
    d = (d - 34.0f) / 6.0f;

  } else {
    // max out at high color
    high = c11;
    cOut->r = high->r;
    cOut->g = high->g;
    cOut->b = high->b;
    return;
  }

  // otherwise use 0.0 <= d <= 1.0 to calculate
  // a gradient between the chosen low/high color
  cOut->r = low->r + d*(high->r - low->r);
  cOut->g = low->g + d*(high->g - low->g);
  cOut->b = low->b + d*(high->b - low->b);
}



void gradient( float  zeroToOne,
               int    repeat,
               Color* c1,
               Color* c2,
               Color* c3,
               Color* cOut )
{
  float r = 3.0f * (float) repeat;

  // end up with 0 <= d <= 3
  float d = fmodf( r * 2.0f * fabs( zeroToOne - 0.5f ), 3.0f );

  Color* low;
  Color* high;

  if( d < 0.5f ) {
    low  = c1;
    high = c2;
    d = d + 0.5f;

  } else if( d < 1.5f ) {
    low  = c2;
    high = c3;
    d = d - 0.5f;

  } else if( d < 2.5f ) {
    low  = c3;
    high = c1;
    d = d - 1.5f;

  } else { //if( d < 3.5f ) {
    low  = c1;
    high = c2;
    d = d - 2.5f;
  }

  // otherwise use 0.0 <= d <= 1.0 to calculate
  // a gradient between the chosen low/high color
  cOut->r = low->r + d*(high->r - low->r);
  cOut->g = low->g + d*(high->g - low->g);
  cOut->b = low->b + d*(high->b - low->b);
}


void gradientNoBlend( float  zeroToOne,
                      int    repeat,
                      Color* c1,
                      Color* c2,
                      Color* c3,
                      Color* cOut )
{
  float r = 3.0f * (float) repeat;

  // end up with 0 <= d <= 3
  float d = fmodf( r * 2.0f * fabs( zeroToOne - 0.5f ), 3.0f );

  if( d < 0.5f ) {
    cOut->r = c1->r;
    cOut->g = c1->g;
    cOut->b = c1->b;

  } else if( d < 1.5f ) {
    cOut->r = c2->r;
    cOut->g = c2->g;
    cOut->b = c2->b;

  } else if( d < 2.5f ) {
    cOut->r = c3->r;
    cOut->g = c3->g;
    cOut->b = c3->b;

  } else { //if( d < 3.5f ) {
    cOut->r = c1->r;
    cOut->g = c1->g;
    cOut->b = c1->b;
  }
}
