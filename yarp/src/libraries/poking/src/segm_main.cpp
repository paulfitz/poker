


//Color Image Segmentation
//This is the implementation of the algorithm described in 
//D. Comaniciu, P. Meer, 
//Robust Analysis of Feature Spaces: Color Image Segmentation,
//http://www.caip.rutgers.edu/~meer/RIUL/PAPERS/feature.ps.gz
//appeared in Proceedings of CVPR'97, San Juan, Puerto Rico.
// ===========================================================================
// =====      Module: segm_main.cc
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

#include	<stdlib.h>
#include	<math.h>
#include	<string.h>
#include 	<fstream.h>
#include 	<strstream.h>
#include	"segm.hh"
#include	<string.h>

#define bool int
#include "YARPImage.h"

extern int option;

static RasterIpChannels* read_PPM_file( istream* fp );
Boolean write_PPM_file( char* fname, RasterIpChannels* signal );
Boolean my_write_PGM_file( char* fname, Octet* signal,int _rows, int _colms );
static RasterIpChannels* read_fake_PPM_file( YARPImageOf<YarpPixelBGR>& img);
static Boolean write_fake_PPM_file( YARPImageOf<YarpPixelBGR>& img, RasterIpChannels* signal );




int segm_apply(YARPImageOf<YarpPixelBGR>& src,YARPImageOf<YarpPixelBGR>& dest)
{
  int i; 
  SegmenterMS::sRectangle rects[Max_rects];
  
  long	selects = 0L;
  selects |= Lightness; selects |= Ustar; selects |= Vstar; 
  int p_dim=3;
  
  assert( p_dim <= p_max );	// must be since all array dimensions are such
  Boolean       block = false;
  unsigned int	seed  = 29254088; //random # generator

  int		n_rect = 0;

  option = 2;

  RasterIpChannels*	signal = read_fake_PPM_file( src );
 
  SegmenterMS	segmenter;
  segmenter.ms_segment( signal, rects, n_rect, selects, seed, block);
  RasterIpChannels*	sig_result = segmenter.result_ras_;

  write_fake_PPM_file( dest, sig_result );
  
  delete signal;
  delete sig_result;
  return( 0 );
}






















/****************************************************************/
/*								*/
/*	Run with:						*/
/*	     segm image_name.ppm                                */
/*	                         			      	*/
/****************************************************************/

int segm_main( int argc, char **argv ) 
{
  int i; 
  SegmenterMS::sRectangle rects[Max_rects];
  
  istream*	fp;
  fp = new ifstream( argv[1], ios::in );
  if ( fp->fail() )
	{
	  cerr << __FILE__ << '(' << __LINE__ << "):  cannot open file " <<
	    argv[1] << endl;
          cerr << "Usage: segm image_name.ppm"<< endl;
	  exit(1);
	}
  ostrstream	cmd;

  long	selects = 0L;
  selects |= Lightness; selects |= Ustar; selects |= Vstar; 
  int p_dim=3;
  
  assert( p_dim <= p_max );	// must be since all array dimensions are such
  Boolean       block = false;
  unsigned int	seed  = 29254088; //random # generator

  int		n_rect = 0;
  Boolean	done = false;
  cout << "Select:"<< endl
       <<"      w = undersegmentation inside a window"<< endl
       <<"      u = undersegmentation" << endl
       <<"      o = oversegmentation" << endl
       <<"      q = quantization" << endl;
  char received;
  while(1)
   {  
     cin >> received;
     if((received =='w') || (received=='W'))
       {
         option=2; break;
       }
     else if((received =='u') || (received=='U'))
       {
         option=2; done=true; break;
       }
     else if((received =='o') || (received=='O'))
       {
         option=1; done=true; break;
       }
     else if((received =='q') || (received=='Q'))
       {
         option=0; done=true; break;
       }
     else
       { 
         cout << endl <<"Please type w, u, o or q !" << endl;
       }
   }
  #ifdef ORG_SIZE
      cmd << "xv " << argv[1] << " &" << ends;
  #else
      cmd << "xv -geometry 120x120 " << argv[1] << " &" << ends;
  #endif
      system( cmd.str() );
 
  while ( !done )
    {
      cout << "***********  This is the " << n_rect+1 <<
	  (n_rect==0 ? "st" : (n_rect==1 ? "nd" : (n_rect==2 ? "rd" : "th")))
	  << " Window!  ************" << endl;
      cout << "The Upper_Left Corner Coords:" << endl;
      int	x1, y1;
      cin >> x1 >> y1;
      cout << endl;
      if((x1 < 0) || (y1 < 0))
        {
	  cout << "********* " << n_rect << " Windows totally *******" << endl;
	  done = true;
	  break;
	}     
      cout << "The Width and Height of window:" << endl;
      int	d1, d2;
      cin >> d1 >> d2;
      cout << endl;
      if ( (d1 == 0) || (d2 == 0) )
	{
	  cout << "********* " << n_rect << " Windows totally *******" << endl;
	  done = true;
	  break;
	}
   
      rects[n_rect].x = x1; rects[n_rect].y = y1;
      rects[n_rect].width = d1; rects[n_rect].height = d2;
      n_rect++;
    }
  RasterIpChannels*	signal = read_PPM_file( fp );
  ((ifstream* )fp)->close();
 
  SegmenterMS	segmenter;
  segmenter.ms_segment( signal, rects, n_rect, selects, seed, block);
  RasterIpChannels*	sig_result = segmenter.result_ras_;

  write_PPM_file( "result.ppm", sig_result );
  ostrstream	cmd1;
  ostrstream	cmd2;
  
#ifdef ORG_SIZE
      cmd1 << "xv result.ppm &" << ends;
  if(option && !n_rect)
      cmd2 << "xv result.pgm &" << ends;
#else
      cmd1 << "xv -geometry 120x120 result.ppm &" << ends;
  if(option && !n_rect)
      cmd2 << "xv -geometry 120x120 result.pgm &" << ends;
      
#endif
      system( cmd1.str() );
  if(option && !n_rect)
      system( cmd2.str() );

  delete signal;
  delete sig_result;
  return( 0 );
}

static void skip( istream* fp )
{
  char	c1, c2;
  if ( (c1 = fp->get() ) != '#' ) {
    fp->putback( c1 );
    return;
  }
  while ( c1 == '#' )
    {
      while ( (c2 = fp->get()) != '\n' )
	;
      c1 = fp->get();
      if ( c1 != '#' )
	fp->putback( c1 );
    }
  return;
}

static RasterIpChannels* read_PPM_file( istream* fp )
{
  int	c1 = fp->get();
  int	c2 = fp->get();
  int	c3 = fp->get();

// test point
  
  if ( c1 == 'P' && c3 == '\n' )
    {
      Octet**	datain = new Octet*[p_max];
      int	w, h, m;

      switch ( c2 ) {
      case '3':
	{
	  skip( fp );
	  *fp >> w >> h;
	  int i;
	  for ( i = 0; i < p_max; i++ ) {
	    datain[i] = new Octet[w * h];
	  }
	  fp->get();
	  skip( fp );
	  *fp >> m;
          
          //need a test for # comments !!!
	  for ( int j = 0, idx = 0; j < h; j++ ) {
	    for ( i = 0; i < w; i++, idx++ ) {
	      *fp >> c1 >> c2 >> c3;   //ASCII decimal values
	      datain[0][idx] = c1;
	      datain[1][idx] = c2;
	      datain[2][idx] = c3;
	    }
	  }
	}
	break;

      case '6':
	{
	  skip( fp );
	  *fp >> w >> h;

	  for ( int i = 0; i < p_max; i++ ) {
	    datain[i] = new Octet[w * h];
	  }
	  fp->get();
	  skip( fp );
	  *fp >> m;

// test point
	  fp->get();
	  skip( fp );
       
          //cout << "Image Dim  h: " << h << "  w: " << w << endl;
          //cout << "Max Value  m: " << m << endl;
          Octet *temp_buf = new Octet[h*w*3];
	  fp->read(temp_buf, h*w*3); 

          Octet *temp0 = datain[0];
	  Octet *temp1 = datain[1];
	  Octet *temp2 = datain[2];
	            
	  for ( register int j = 0, idx = 0; j < h*w; j++) {
	      temp0[j] = temp_buf[idx++];
	      temp1[j] = temp_buf[idx++];
	      temp2[j] = temp_buf[idx++];
	    }
          delete [] temp_buf;
	}
	break;

      default:
	cerr << "File is not the PPM format." << endl;
	return NULL;
      }

      XfRaster::Info	info;
      info.rows = h;
      info.columns = w;
      info.origin_x = 0;
      info.origin_y = 0;
      return (new RasterIpChannels( info, p_max, eDATA_OCTET,
				    datain, true ) );
    }
  cerr << "File is not the PPM format." << endl;
  return NULL;
}

Boolean my_write_PGM_file( char* fname, Octet* signal,int _rows, int _colms )
{
  ofstream fp( fname, ios::out );
  if ( fp.fail() )
    return false;

  fp << "P5\n" << _colms << ' ' << _rows<< "\n255" << endl;
  fp.write(signal,_rows*_colms);
  fp.close( );
  return true;
}

Boolean write_PPM_file( char* fname, RasterIpChannels* signal )
{
  ofstream fp( fname, ios::out );
  if ( fp.fail() )
    return false;

  fp << "P6\n" << signal->columns_ << ' ' << signal->rows_ << "\n255" << endl;

  assert( signal->dtype_ == eDATA_OCTET );
  Octet *temp_buf = new Octet[signal->rows_*signal->columns_*3];
  Octet *temp0 = signal->chdata_[0];
  Octet *temp1 = signal->chdata_[1];
  Octet *temp2 = signal->chdata_[2];   
  for ( register int j = 0, idx = 0; j < signal->rows_*signal->columns_; j++) {
     temp_buf[idx++]=temp0[j];  //red
     temp_buf[idx++]=temp1[j];  //green
     temp_buf[idx++]=temp2[j];  //blue
   }
  fp.write(temp_buf,signal->rows_*signal->columns_*3);
  delete [] temp_buf;

  fp.close( );
  return true;
}

// Class constructor
// The `data' may be taken away from the caller in order to
// avoid time-consuming copying of the data.  However,
// the caller has to give explicit permission for this.
RasterIpChannels::RasterIpChannels(
    const XfRaster::Info& info, const int n_channels,
    const DataType dtype, Octet* data[], const Boolean do_take
) {
    rows_ = info.rows;
    columns_ = info.columns;
    dtype_ = dtype;
    clipf_ = false;
    n_channels_ = n_channels;
    if (n_channels_ == 0) {
	n_channels_ = 1;
    }
    size_t size = (size_t)(rows_ * columns_);
    chdata_ = new Octet*[n_channels_];
    for (int channel = 0; channel < n_channels_; channel++) {
	if ( do_take == true ) {	// take over the data from the caller
	    chdata_[channel] = (Octet* )data[channel];
	    data[channel] = nil;
	} else {
	    if ( dtype_ == eDATA_FLOAT )	    size *= sizeof(float);
	    chdata_[channel] = new Octet[size];
	    if (data != nil && data[channel] != nil) {
	      memmove( chdata_[channel], data[channel], size );
	    } else {
	      memset( chdata_[channel], 0, size );
	    }
	}
    }
  delete [] data;     
}

RasterIpChannels::~RasterIpChannels() {
    for (int channel = 0; channel < n_channels_; channel++) {
	if (chdata_[channel])   delete [] chdata_[channel];
	chdata_[channel] = nil;
    }
    delete [] chdata_;
}

// RANGE forces `a' to be in the range [b..c] (inclusive)
inline void RANGE( int& a, const int b, const int c )
{
  if ( a < b ) {
      a = b;
  } else if ( a > c ) {
      a = c;
  }
}

void RasterIpChannels::raster_info(Info& i) {
    i.rows = rows_;
    i.columns = columns_;
    i.origin_x = 0;
    i.origin_y = 0;
}

// Move floating point array to Octet array, i.e. to [0..255]
// The result is either scaled to the range [0..255] or
// clipped to this range, depending on the flag `clipf'.
// Note:  Handles only 1-Octet per pixel pictures
// (i.e. mono/pseudo color pictures)
Octet** RasterIpChannels::raster_float_to_Octet(
    RasterIpChannels& ras
) {
    assert( ras.dtype() == eDATA_FLOAT );

    float	maxv = -1.0e38;
    XfRaster::Info	info;
    ras.raster_info(info);
    size_t	size = (size_t)(info.rows * info.columns);

    Octet**	data = ras.chdata();
    int			channels = ras.n_channels();
    Octet**	picRes = new Octet*[channels];
    int			i;
    for ( i = 0; i < channels; i++ )
      picRes[i] = new Octet[ size ];

    if ( ras.clipf() == true ) {  // clip the values outside the range [0..255]
	int	p;
	for ( i = 0; i < channels; i++ ) {
	    register float*		ptr1 = (float* )data;
	    register Octet*	ptr2 = picRes[i];
	    for ( register int off = 0; off < size; off++, ptr1++, ptr2++ ) {
		p = int(*ptr1);
		RANGE( p, 0, 255 );
		*ptr2 = Octet(p);
	    }
	}
    } else {			// scale the values to the range [0..255]
	for ( i = 0; i < channels; i++ ) {
	    float			minv = (float) 1.e38;
	    float			maxv = (float) -1.e38;
	    register float*		ptr1 = (float* ) data[i];
	    register Octet*	ptr2 = picRes[i];
	    register int	off;
	    for ( off = 0; off < size; off++, ptr1++ ) {
		if ( *ptr1 < minv )   minv = *ptr1;
		if ( *ptr1 > maxv )   maxv = *ptr1;
	    }
	    ptr1 = (float* ) data[i];
	    float	ratio = (float) 255.0 / (maxv - minv);
	    for ( off = 0; off < size; off++, ptr1++, ptr2++ )
	        *ptr2 = Octet( (*ptr1 - minv) * ratio );
	}
    }
    return ( picRes );
}






















static RasterIpChannels* read_fake_PPM_file( YARPImageOf<YarpPixelBGR>& img)
{
  Octet**	datain = new Octet*[p_max];
  int	w, h, m = 255;

  w = img.GetWidth();
  h = img.GetHeight();
  
  for ( int i = 0; i < p_max; i++ ) {
    datain[i] = new Octet[w * h];
  }
  
  //cout << "Image Dim  h: " << h << "  w: " << w << endl;
  //cout << "Max Value  m: " << m << endl;
  Octet *temp_buf = (Octet*)(&img(0,0));
  
  Octet *temp0 = datain[0];
  Octet *temp1 = datain[1];
  Octet *temp2 = datain[2];
  
  for ( int j = 0, idx = 0; j < h*w; j++) {
    temp0[j] = temp_buf[idx++];
    temp1[j] = temp_buf[idx++];
    temp2[j] = temp_buf[idx++];
  }

  XfRaster::Info	info;
  info.rows = h;
  info.columns = w;
  info.origin_x = 0;
  info.origin_y = 0;
  return (new RasterIpChannels( info, p_max, eDATA_OCTET,
				datain, true ) );
}

static Boolean write_fake_PPM_file( YARPImageOf<YarpPixelBGR>& img, RasterIpChannels* signal )
{
  assert( signal->dtype_ == eDATA_OCTET );
  img.Resize(signal->columns_,signal->rows_);
  Octet *temp_buf = (Octet*)(&img(0,0));
  Octet *temp0 = signal->chdata_[0];
  Octet *temp1 = signal->chdata_[1];
  Octet *temp2 = signal->chdata_[2];   
  for (int j = 0, idx = 0; j < signal->rows_*signal->columns_; j++) {
     temp_buf[idx++]=temp0[j];  //red
     temp_buf[idx++]=temp1[j];  //green
     temp_buf[idx++]=temp2[j];  //blue
   }

  return true;
}
