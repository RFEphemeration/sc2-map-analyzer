#ifndef ___outstreams_hpp___
#define ___outstreams_hpp___

void printOutstreamStatus();

void printMessage( const char* format, ... );
void printWarning( const char* format, ... );
void printError  ( const char* format, ... );

void enableMessages();
void enableWarnings();
void enableErrors();

void disableMessages();
void disableWarnings();
void disableErrors();

#endif // ___outstreams_hpp___
