#ifndef ___utility_hpp___
#define ___utility_hpp___

#include <string>
using namespace std;

#include "coordinates.hpp"



template<typename T, int size>
int getArrLength(T(&)[size]){return size;}



void removeChars( const char* chars, string* s );

void replaceChars( const char* chars, const char* with, string* s );

void makeLower( string* s );

void trimLeadingSpaces ( string* s );
void trimTrailingSpaces( string* s );

float xtof( char c1, char c0 );

void formatPath( string* s );

float snapIfNearInt( float x );

float p2pDistance( point* p1, point* p2 );

// forward declaration
struct Color;

void alterSaturation( float zeroToOne, Color* c );

void mixWith( Color* changing, Color* mixer );



void gradient( float  zeroToOne,
               Color* c1,
               Color* c2,
               Color* c3,
               Color* cOut );

void gradient( float  zeroToOne,
               Color* c1,
               Color* c2,
               Color* c3,
               Color* c4,
               Color* cOut );

void gradient( float  zeroToOne,
               Color* c1,
               Color* c2,
               Color* c3,
               Color* c4,
               Color* c5,
               Color* c6,
               Color* c7,
               Color* cOut );

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
               Color* cOut );

void gradient( float  zeroToOne,
               int    repeat,
               Color* c1,
               Color* c2,
               Color* c3,
               Color* cOut );

void gradientNoBlend( float  zeroToOne,
                      int    repeat,
                      Color* c1,
                      Color* c2,
                      Color* c3,
                      Color* cOut );


// for turning the version macro values
// into string literals in output strings
#define QUOTEHELPER( x ) #x
#define QUOTEMACRO( x ) QUOTEHELPER( x )


extern const char* projectURL;
extern const char* adminEmail;


#endif // ___utility_hpp___
