///////////////////////////////////////////
//
//  This file is for bookkeeping tasks that
//  don't fall into the more focused analysis
//  categories.
//
///////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "utility.hpp"
#include "outstreams.hpp"
#include "SC2Map.hpp"


void SC2Map::countPathableCells()
{
  for( int pci = 0; pci < cxDimPlayable; ++pci )
  {
    for( int pcj = 0; pcj < cyDimPlayable; ++pcj )
    {
      point c;
      c.pcSet( pci, pcj );

      for( int t = 0; t < NUM_PATH_TYPES; ++t )
      {
        if( getPathing( &c, (PathType)t ) )
        {
          ++numPathableCells[t];
        }
      }
    }
  }
}
