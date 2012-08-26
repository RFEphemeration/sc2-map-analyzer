#include <stdlib.h>
#include <stdio.h>

#include "coordinates.hpp"


// some frame translations are invalid
// and should generate the appropriate
// Not-a-Number values
const float point::mNaN  = -987.654321f;
const int   point::mtNaN = -12345;
const int   point::mcNaN = -23456;
const int   point::ptNaN = -34567;
const int   point::pcNaN = -45678;


void point::set( point* p )
{
  mx  = p->mx;  my  = p->my;
  mtx = p->mtx; mty = p->mty;
  mcx = p->mcx; mcy = p->mcy;
  ptx = p->ptx; pty = p->pty;
  pcx = p->pcx; pcy = p->pcy;
  ix  = p->ix;  iy  = p->iy;
}

void point::mSet( float mxIn, float myIn )
{
  mx  = mxIn;           my  = myIn;
  mtx = m2mt  ( mxIn ); mty = m2mt  ( myIn );
  mcx = m2mc  ( mxIn ); mcy = m2mc  ( myIn );
  ptx = mx2ptx( mxIn ); pty = my2pty( myIn );
  pcx = mx2pcx( mxIn ); pcy = my2pcy( myIn );
  ix  = mx2ix ( mxIn ); iy  = my2iy ( myIn );
}

void point::mtSet( int mtxIn, int mtyIn )
{
  mx  = mt2m   ( mtxIn ); my  = mt2m   ( mtyIn );
  mtx = mtxIn;            mty = mtyIn;
  mcx = mcNaN;            mcy = mcNaN;
  ptx = mtx2ptx( mtxIn ); pty = mty2pty( mtyIn );
  pcx = pcNaN;            pcy = pcNaN;
  ix  = mtx2ix ( mtxIn ); iy  = mty2iy ( mtyIn );
}

void point::mcSet( int mcxIn, int mcyIn )
{
  mx  = mc2m   ( mcxIn ); my  = mc2m   ( mcyIn );
  mtx = mtNaN;            mty = mtNaN;
  mcx = mcxIn;            mcy = mcyIn;
  ptx = ptNaN;            pty = ptNaN;
  pcx = mcx2pcx( mcxIn ); pcy = mcy2pcy( mcyIn );
  ix  = mcx2ix ( mcxIn ); iy  = mcy2iy ( mcyIn );
}

void point::ptSet( int ptxIn, int ptyIn )
{
  mx  = ptx2mx ( ptxIn ); my  = pty2my ( ptyIn );
  mtx = ptx2mtx( ptxIn ); mty = pty2mty( ptyIn );
  mcx = mcNaN;            mcy = mcNaN;
  ptx = ptxIn;            pty = ptyIn;
  pcx = pcNaN;            pcy = pcNaN;
  ix  = ptx2ix ( ptxIn ); iy  = pty2iy ( ptyIn );
}

void point::pcSet( int pcxIn, int pcyIn )
{
  mx  = pcx2mx ( pcxIn ); my  = pty2my ( pcyIn );
  mtx = mtNaN;            mty = mtNaN;
  mcx = pcx2mcx( pcxIn ); mcy = pcy2mcy( pcyIn );
  ptx = ptNaN;            pty = ptNaN;
  pcx = pcxIn;            pcy = pcyIn;
  ix  = pcx2ix ( pcxIn ); iy  = pcy2iy ( pcyIn );
}

// never a need to translate back from image frame!
void point::iSet( int ixIn, int iyIn )
{
  mx  = mNaN;  my  = mNaN;
  mtx = mtNaN; mty = mtNaN;
  mcx = mcNaN; mcy = mcNaN;
  ptx = ptNaN; pty = ptNaN;
  pcx = pcNaN; pcy = pcNaN;
  ix  = ixIn;  iy  = iyIn;
}


// translation constants
int point::mcLeft;
int point::mcBottom;
int point::iDimT;
int point::iDimC;

float point::mcLeftf;
float point::mcBottomf;
float point::iDimTf;

void point::setFrameTranslationConstants( int mcLeftIn,
                                          int mcBottomIn,
                                          int iDimTIn,
                                          int iDimCIn )
{
  mcLeft   = mcLeftIn;
  mcBottom = mcBottomIn;
  iDimT    = iDimTIn;
  iDimC    = iDimCIn;

  // better to do this cast once as use in some
  // translations below many times
  mcLeftf   = (float)mcLeft;
  mcBottomf = (float)mcBottom;
  iDimTf    = (float)iDimT;
}


int point::m2mt( float m )
{
  // the area of [3.5, 7.5] to (4.5, 8.5) in
  // map coordinates is the terrain point (4, 8)
  // so shift a map coordinate by 0.5 then truncate
  // to get the terrain coordinate that covers it
  return (int)(m + 0.5f);
}

int point::m2mc( float m )
{
  // the area of [3.0, 7.0] to (4.0, 8.0) in
  // map coordinates is the cell (3, 7) so just
  // truncate a map coordinate to get the
  // cell coordinate it lies in
  return (int)m;
}

float point::mt2m( int mt )
{
  // the terrain coordinate can be viewed as
  // already describing the exact center of the
  // terrain point in map coordinates already
  return (float)mt;
}

float point::mc2m( int mc )
{
  // the center of a cell is offset by (0.5, 0.5)
  // from the corresponding map coordinates
  return (float)mc + 0.5f;
}


// The conversion constants mcLeft and mcBottom
// are especially useful because all of the
// coordinate frames except the image frame are
// of the same scale.  The rest of the conversions
// are simple translations by the constants, with
// maybe a call to the above core conversions.

int point::mtx2ptx( int mtx )
{
  return mtx - mcLeft;
}

int point::mty2pty( int mty )
{
  return mty - mcBottom;
}

int point::ptx2mtx( int ptx )
{
  return ptx + mcLeft;
}

int point::pty2mty( int pty )
{
  return pty + mcBottom;
}

int point::mcx2pcx( int mcx )
{
  return mcx - mcLeft;
}

int point::mcy2pcy( int mcy )
{
  return mcy - mcBottom;
}

int point::pcx2mcx( int pcx )
{
  return pcx + mcLeft;
}

int point::pcy2mcy( int pcy )
{
  return pcy + mcBottom;
}

int point::mx2ptx( float mx )
{
  return mtx2ptx( m2mt( mx ) );
}

int point::my2pty( float my )
{
  return mty2pty( m2mt( my ) );
}

float point::ptx2mx( int ptx )
{
  return mt2m( ptx2mtx( ptx ) );
}

float point::pty2my( int pty )
{
  return mt2m( pty2mty( pty ) );
}

int point::mx2pcx( float mx )
{
  return mcx2pcx( m2mc( mx ) );
}

int point::my2pcy( float my )
{
  return mcy2pcy( m2mc( my ) );
}

float point::pcx2mx( int pcx )
{
  return mc2m( pcx2mcx( pcx ) );
}

float point::pcy2my( int pcy )
{
  return mc2m( pcy2mcy( pcy ) );
}

// when translating to the image frame,
// give the CENTER pixel of the
// appropriate area of a terrain or cell
// coordinate, and for a general map
// coordinate just give the closest pixel
int point::mx2ix( float mx )
{
  return (int)(iDimTf*(mx - mcLeftf));
}

int point::my2iy( float my )
{
  return (int)(iDimTf*(my - mcBottomf));
}

int point::mtx2ix( int mtx )
{
  return iDimT*mtx2ptx( mtx );
}

int point::mty2iy( int mty )
{
  return iDimT*mty2pty( mty );
}

int point::mcx2ix( int mcx )
{
  return iDimT*mcx2pcx( mcx ) + iDimT/2;
}

int point::mcy2iy( int mcy )
{
  return iDimT*mcy2pcy( mcy ) + iDimT/2;
}

int point::ptx2ix( int ptx )
{
  return iDimT*ptx;
}

int point::pty2iy( int pty )
{
  return iDimT*pty;
}

int point::pcx2ix( int pcx )
{
  return iDimT*pcx + iDimT/2;
}

int point::pcy2iy( int pcy )
{
  return iDimT*pcy + iDimT/2;
}
