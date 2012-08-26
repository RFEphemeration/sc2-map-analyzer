#ifndef ___coordinates_hpp___
#define ___coordinates_hpp___

//////////////////////////////////////////////////////////////
//
//  There are several coordinate frames for
//  managing the data in this tool.
//
//
//  the Map Coordinate Frame is what all Galaxy Editor-
//  generated map data is expressed in.  The origin is in
//  the extreme bottom-left of the map.  These values are
//  floating point and have the prefix (m) in this tool.
//
//  the Map's Terrain Coordinate Frame is an integer frame
//  expressing the location of cliffs.  The origin of this
//  frame is over the Map Coordinate Frame and in this tool
//  points in this frame have prefix (mt).
//
//  the Map's Cell Coordinate Frame (mc) is an integer frame
//  expressing the location of cells, which are the most
//  relevant areas to the game play.  For instance, when
//  placing a building, the grid you see is in cells.
//  The origin of this frame is at m(0.5, 0.5) and when you
//  examine doodads or start locations you'll notice their
//  position always ends in a .5--because doodads are aligned
//  cells, but the position is given in map coordinates.
//
//  the MapInfo data file contains a width and height that
//  determine the width and height of the CELLS frame! It also
//  has the left, right, top and bottom boundaries of the
//  playable area, also measured in the cell frame.
//
//  All of the above frames are defined by Blizzard.  This is
//  a visualization of the relationships.
//
//
//
//
//           Some object (see OBJ in playable area),
//           is aligned to cells, and its map coordinates
//           always end in .5
//
//                           |                       |
//                  #     #  |  #     #     #     #  |  #     #     # <--- mt(width+1, height+1)
//                           |                       |
//                     @     @     @     @     @     @     @     @ <--- mc(width, height)
//                           |                       |
//                  #     #  |  #     #     #     #  |  #     #     #
//                           |                       |
//    boundaries ------@-----@-----@-----@-----@-----@-----@-----@-----
//    top and                |                       |                      "%" marks a cell in the
//    bottom are    #     #  |  #     #     #     #  |  #     #     #       playable area and
//    integers               |      playable area    |                      "@" marks a cell in the
//    in the           @     %    OBJ    %     %     @     @     @          unplayable area
//    cell frame             |                       |
//                  #     #  |  #     #     #     #  |  #     #     #       Note that cells along the
//                           |                       |                      bottom and left boundaries
//               ------@-----%-----%-----%-----%-----@-----@-----@-----     are playable, but cells
//                           |                       |                      along top and right are not
//                  #     #  |  #     #     #     #  |  #     #     #
//                           |                       |
// mc(0, 0) is at ---> @     @     @     @     @     @     @     @ <--.
// m(0.5, 0.5)               |                       |                 \
//             .--> #     #  |  #     #     #     #  |  #     #     #    Every cell is bordered by
//            /              |                       |                   terrain points, so the MapInfo
// This is both              |                       |                   width x height of the cell area
// the origin               boundaries left and right are                is "surrounded" by a terrain
// m(0.0, 0.0) and          integers in the cell frame                   area (width+1) x (height+1)
// mt(0, 0)
//
//
//
//
//  I define some coordinate frames for the SC2Map class:
//
//  The playable terrain frame (pt) and the playable cell frame
//  (pc) begin at the left, bottom playable boundaries and logically
//  extend to the right, top boundaries.  These frames express
//  coordinates for terrain and cells that are in-play and
//  therefore worth analyzing.
//
//  There is also an image pixels frame (i) for translating a point
//  into a pixel of the output PNG images.  Each terrain unit is
//  8 pixels on a side, and each cell is 6 pixels on a side, but the
//  cell grid has the same spacing as the terrain grid, only
//  shifted.  If you arbitrarily numbered some terrain and cell
//  points as t1, t2, etc. and C1, C2, etc. and plotted them in the
//  image frame, they look like this (where each two-chars "t1" in
//  this ASCII art is one pixel):
//
//  IMPORTANT NOTE: When you have a terrain or cell coordinate
//  and you translate it to the image coordinate frame, you
//  get the CENTER pixel of the cell or terrain coordinate.
//  For example in the image below, cell C2 translated to the
//  image frame is the pixel marked ##
//
//
//        t4t4            t5t5            t6t6
//    t4t4t4t4t4t4    t5t5t5t5t5t5    t6t6t6t6t6t6
//    t4t4t4t4t4t4    t5t5t5t5t5t5    t6t6t6t6t6t6
//  t4t4t4t4t4t4t4t4t5t5t5t5t5t5t5t5t6t6t6t6t6t6t6t6
//  t4t4t4t4t4t4t4t4t5t5t5t5t5t5t5t5t6t6t6t6t6t6t6t6
//    t4t4t4t4t4t4C1C1t5t5t5t5t5t5C2C2t6t6t6t6t6t6
//    t4t4t4t4t4t4C1C1t5t5t5t5t5t5C2C2t6t6t6t6t6t6
//        t4t4C1C1C1C1C1C1t5t5C2C2C2C2C2C2t6t6
//        t1t1C1C1C1C1C1C1t2t2C2C2##C2C2C2t3t3
//    t1t1t1t1t1t1C1C1t2t2t2t2t2t2C2C2t3t3t3t3t3t3
//    t1t1t1t1t1t1C1C1t2t2t2t2t2t2C2C2t3t3t3t3t3t3
//  t1t1t1t1t1t1t1t1t2t2t2t2t2t2t2t2t3t3t3t3t3t3t3t3
//  t1t1t1t1t1t1t1t1t2t2t2t2t2t2t2t2t3t3t3t3t3t3t3t3
//    t1t1t1t1t1t1    t2t2t2t2t2t2    t3t3t3t3t3t3
//    t1t1t1t1t1t1    t2t2t2t2t2t2    t3t3t3t3t3t3
//        t1t1            t2t2            t3t3
//
//
//  This way the terrain and cell grids overlap, but the
//  points from both are visible, so we can display both
//  data sets at the same time.
//
//////////////////////////////////////////////////////////////


struct point
{
  // Not a Number constants for invalid conversions
  static const float mNaN;
  static const int   mtNaN;
  static const int   mcNaN;
  static const int   ptNaN;
  static const int   pcNaN;

  // frame translation constants
  static int mcLeft;
  static int mcBottom;
  static int iDimT;
  static int iDimC;

  // just convenient casts of above constants
  static float mcLeftf;
  static float mcBottomf;
  static float iDimTf;

  static void setFrameTranslationConstants( int mcLeftIn,
                                            int mcBottomIn,
                                            int iDimTIn,
                                            int iDimCIn );

  void set( point* p );

  float mx;
  float my;
  void mSet( float mxIn, float myIn );

  int mtx;
  int mty;
  void mtSet( int mtxIn, int mtyIn );

  int mcx;
  int mcy;
  void mcSet( int mcxIn, int mcyIn );

  int ptx;
  int pty;
  void ptSet( int ptxIn, int ptyIn );

  int pcx;
  int pcy;
  void pcSet( int pcxIn, int pcyIn );

  int ix;
  int iy;
  void iSet( int ixIn, int iyIn );


private:

  // the map, terrain and cell frames
  // defined by Blizzard can be converted
  // between without any x- or y-coordinate
  // specific constants
  inline int   m2mt ( float m  );
  inline int   m2mc ( float m  );
  inline float mt2m ( int   mt );
  inline float mc2m ( int   mc );

  // break the rest of the conversion up
  // into x and y
  inline int mtx2ptx( int mtx );
  inline int mty2pty( int mty );
  inline int ptx2mtx( int ptx );
  inline int pty2mty( int pty );

  inline int mcx2pcx( int mcx );
  inline int mcy2pcy( int mcy );
  inline int pcx2mcx( int pcx );
  inline int pcy2mcy( int pcy );

  inline int   mx2ptx( float mx  );
  inline int   my2pty( float my  );
  inline float ptx2mx( int   ptx );
  inline float pty2my( int   pty );
  inline int   mx2pcx( float mx  );
  inline int   my2pcy( float my  );
  inline float pcx2mx( int   pcx );
  inline float pcy2my( int   pcy );

  // no need to ever translate back
  // from image coordinates
  inline int mx2ix ( float mx  );
  inline int my2iy ( float my  );
  inline int mtx2ix( int   mtx );
  inline int mty2iy( int   mty );
  inline int mcx2ix( int   mcx );
  inline int mcy2iy( int   mcy );
  inline int ptx2ix( int   ptx );
  inline int pty2iy( int   pty );
  inline int pcx2ix( int   pcx );
  inline int pcy2iy( int   pcy );
};


#endif // ___coordinates_hpp___
