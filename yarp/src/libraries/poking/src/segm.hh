//Color Image Segmentation
//This is the implementation of the algorithm described in 
//D. Comaniciu, P. Meer, 
//Robust Analysis of Feature Spaces: Color Image Segmentation,
//http://www.caip.rutgers.edu/~meer/RIUL/PAPERS/feature.ps.gz
//appeared in Proceedings of CVPR'97, San Juan, Puerto Rico.
// ===========================================================================
// =====      Module: segm_hh
// ===== -------------------------------------------------------------- ======
// =====      Version 01   Date: 04/22/97
// ===== -------------------------------------------------------------- ======
// ===========================================================================
// =====      Written by Dorin Comaniciu
// =====      e-mail:  comanici@caip.rutgers.edu
// ===========================================================================
// Permission to use, copy, or modify this software and its documentation
// for educational and research purposes only is hereby granted without
// fee, provided that this copyright notice appear on all copies and
// related documentation.  For any other uses of this software, in original
// or modified form, including but not limited to distribution in whole
// or in part, specific prior permission must be obtained from
// the author(s).
//
// THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
// EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
// WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
// IN NO EVENT SHALL RUTGERS UNIVERSITY BE LIABLE FOR ANY SPECIAL,
// INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY
// DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY
// THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE
// OR PERFORMANCE OF THIS SOFTWARE.
// ===========================================================================
//
 

#include <assert.h>
#define         ORG_SIZE

#define		DEBUG

//#define	TRACE

#define         Max_rects       50

#define MIN(x,y)         ((x) <= (y) ? (x) : (y))
#define Min3(x,y,z)      (MIN((x),(y)) <= (z) ? MIN((x),(y)) : (z))


#  include <string.h>

#  define       bzero(b,len)            memset(b,'\0',len)
#  define       bcopy(b1,b2,len)        memmove(b2,b1,len)

typedef char Boolean;		// copied from <X11/Intrinsic.h>
typedef unsigned char Octet;

#define false 0
#define true !false
#define nil 0

class XfRaster {
 public:
    XfRaster() { };
    ~XfRaster() { };

    struct Info {
        int rows;
        int columns;
        int origin_x;
        int origin_y;
    };
};

enum DataType {			// Values of data type:
        eDATA_OCTET  =  10,	// unsigned char data
        eDATA_FLOAT		// floating point data
    };


class RasterIpChannels : public XfRaster {
 public:

    RasterIpChannels(
	const XfRaster::Info& info, const int n_channels,
	const DataType, Octet* data[], const Boolean
    );
    ~RasterIpChannels();

    Octet**	chdata() const;
    Octet*	chdata(int i) const;
    int		n_channels() const;
    DataType	dtype() const;
    void	clipf(Boolean cf);
    Boolean	clipf() const;

    void raster_info(Info& i);
    static unsigned char** raster_float_to_Octet( RasterIpChannels& ras );

    int		rows_;
    int		columns_;
    DataType	dtype_;
    Octet**	chdata_;
    int		n_channels_;
    Boolean	clipf_;		// flag that determines wheter to clip or not
				//   when converting from eDATA_FLOAT to Octet
};

inline Octet** RasterIpChannels::chdata() const   { return chdata_; }
inline Octet* RasterIpChannels::chdata(int i) const {
    assert((0 <= i) && (i < n_channels_));
    return chdata_[i];
}
inline int RasterIpChannels::n_channels() const   { return n_channels_; }
inline DataType RasterIpChannels::dtype() const   { return dtype_; }
inline void RasterIpChannels::clipf(Boolean cf)   { clipf_ = cf; }
inline Boolean RasterIpChannels::clipf() const   { return clipf_; }


const int	p_max	      =	3;	// max space dim;  MAX(p_max) == 6 !!

typedef struct region_str
    {
      int my_contor;
      int my_class;
      int my_label;
      int *my_region;
      struct region_str *next_region_str;
    }REGION;

class SegmenterMS {

 public:

  SegmenterMS( );
  ~SegmenterMS();

  struct sRectangle {
    int x, y;				// upper left corner
    unsigned int width, height;	        // rectangle dimensions
  };
 
  RasterIpChannels*	result_ras_;	// result of the visit to a raster

  Boolean	ms_segment( RasterIpChannels*, sRectangle*, int, long,
			      unsigned int, Boolean);

 protected:

  int		_p;		// parameter space dimension
  int		_p_ptr;		// counter for the number of components already
                                // stored in _data_all
  int**	        _data_all;	// input data
  int*_data0,*_data1,*_data2;   //to speed up
          
  int		_colms, _rows;	// width, height dimensions of the input data
  int           _ro_col;        // how many points in input data
  int**         _data;		// region of the input data currently segmented
  int		_rcolms, _rrows;// width, height dimensions of the region
  int		_n_points;	// number of data points (per channel)
  float		_min_per;       // minimum/maximum cluster size
  int		_NJ;		// maximum number of subsamples
  Boolean       auto_segm;
  int*          gen_remain;     //the map of remaining points            
  Octet*        gen_class;      //labels
  Octet*        taken;
  int*          conn_selected;  //used by connected component
  int           n_remain;       //# of remaining points 
  int           _n_colors;      //# of colors
  int**         _col_all;       //colors in the image (LUV)
  int*_col0,*_col1,*_col2;      //to speed up
  int *         _col_RGB;       //colors in the image (RGB)
  int *         _col_index;     //color address
  int *         _col_misc;      //misc use in histogram and conversion
  int*          _m_colors;      //how many of each color

  int*          _col_remain;    //the map of remaining colors 
  int*          _m_col_remain;  //table of remaining colors
  int           _n_col_remain;  //# of remaining colors

  void convert_RGB_LUV( RasterIpChannels* , long );
  void cut_rectangle( sRectangle* );
  void init_matr(void);
  float my_sampling( int, float T[Max_rects][p_max]);
  int  subsample( float* );
  void test_neigh(Octet* , int *, int*, int);
  void my_actual(Octet*);
  void init_neigh(void);
  void conn_comp(Octet *, int* , Octet* ,float [] [p_max],int,int);
  void find_other_neigh(int, int *, REGION *);
  void new_auto_loop(float *, Octet*);
  void nauto_loop(float *, int *, Octet *, int *); 
  void optimize(float [][p_max], int);
  void get_codeblock(float [][p_max], int);
  void get_codeblock1(float [][p_max], int);
  void new_codebook(float [][p_max], int);
  void my_clean(float [][p_max], int);
  void my_histogram(RasterIpChannels*, long);
  void eliminate_class(Octet *,int *,int *,int, Octet *, float [][p_max],REGION *);
  void eliminate_region(int *,int,float [][p_max],REGION *);
  void destroy_region_list(REGION *);
  REGION *create_region_list(int *, int);
  void initializations(RasterIpChannels*, sRectangle [], int *, long , int *);
};

// List of color components; the possible cases are:

#define RedColor		(1<<0)
#define GreenColor		(1<<1)
#define BlueColor		(1<<2)
#define Lightness		(1<<3)
#define Ustar   		(1<<4)
#define Vstar   		(1<<5)

#define MAXV 256
