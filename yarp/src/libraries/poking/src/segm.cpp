//Color Image Segmentation
//This is the implementation of the algorithm described in 
//D. Comaniciu, P. Meer, 
//Robust Analysis of Feature Spaces: Color Image Segmentation,
//http://www.caip.rutgers.edu/~meer/RIUL/PAPERS/feature.ps.gz
//appeared in Proceedings of CVPR'97, San Juan, Puerto Rico.
// ===========================================================================
// =====      Module: segm.cc
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

#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <fstream.h>
#include <strstream.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "segm.hh"


int option = 2;
static const int rect_gen_contor=3;

// Radius of the searching window
static float gen_RADIUS[3]={2, 3, 4};
static float rect_RADIUS[rect_gen_contor]={8, 6, 4};
static float fix_RADIUS[rect_gen_contor];
static float final_RADIUS;
static float RADIUS2;
static float RADIUS;

static float max_dist;

static int my_threshold[3]={50, 100, 400};
static int my_rap[3]={4, 2, 1};
static int act_threshold;

#define my_abs(a) ((a) > 0 ? (a): (-a)) 
#define SQRT2 1.4142
#define SQRT3 1.7321
static const float	BIG_NUM      =	1.0e+20;

// Coefficient matrix for xyz and rgb spaces
static const int    XYZ[3][3] = { { 4125, 3576, 1804 },
                                  { 2125, 7154,  721 },
                                  {  193, 1192, 9502 } };
static const float  RGB[3][3] = { { 3.2405, -1.5371, -0.4985 },
                                  {-0.9693,  1.8760,  0.0416 },
                                  { 0.0556, -0.2040,  1.0573 } };
 
// Constants for LUV transformation 
static const float     Xn = 0.9505;
static const float     Yn = 1.0;
static const float     Zn = 1.0888;
static const float     Un_prime = 0.1978;
static const float     Vn_prime = 0.4683;
static const float     Lt = 0.008856;

// # of samples
static const int	Max_J	     =	25;

// Limit of the number of failed trials
static const int       Max_trials    =  50;

// Defaults values for the parameters.
static const int	sam_max = 60;

// Few more trials at the end
static const int        MAX_TRIAL=10;

// Used in 3-D histogram computation
static const int FIRST_SIZE=262144; // 2^18
static const int SEC_SIZE=64;  // 2^6

// Make coordinate computation faster
// my_neigh is for auto_segm, my_neigh_r works with region
static int my_neigh[8];
static int my_neigh_r[8];


// Results
static int ORIG_COLORS;
static int SEGM_COLORS;

Boolean my_write_PGM_file( char*, Octet*,int,int);
void    covariance_w(const int N, int M, const int p, int ** data, 
                   int *w, float T[],  float C[p_max][p_max]);
void    mean_s(const int N, const int p, int J[], int **data, float T[]);
void    my_convert(int, float *, int *);
void    reverse_map(Octet *, Octet *, int *, Octet *, float T[][p_max]);

// Class constructor
SegmenterMS::SegmenterMS( )
{
  _p = 0;
  _p_ptr = 0;
  _rrows  = 0;
  _rcolms = 0;
  _data_all = nil;
  _data = nil;
  _NJ = Max_J;
}

// Class destructor
SegmenterMS::~SegmenterMS( )
{
  if ( _data_all ) {
    for ( register int i = 0; i < _p; i++ )
      if ( _data_all[i] )   delete [] _data_all[i];
    delete [] _data_all;
  }
  if ( _data ) {
    for ( register int i = 0; i < _p; i++ )
      if ( _data[i] )   delete [] _data[i];
    delete [] _data;
  }
}

// LUV (final_T[]) to RGB (TI[]) conversion
void my_convert(int selects, float *final_T, int *TI)
{
  // this condition is always true
  if ( selects & Lightness && selects & Ustar && selects & Vstar )
    {
      if(final_T[0]<0.1)
        {
	  TI[0]=0;TI[1]=0;TI[2]=0;
	}
      else
	{
          float my_x, my_y, my_z;
	  if(final_T[0]< 8.0)
            my_y = Yn * final_T[0] / 903.3;
          else
            my_y = Yn * pow((final_T[0] + 16.0) / 116.0, 3);

          float u_prime = final_T[1] / (13 * final_T[0]) + Un_prime;
          float v_prime = final_T[2] / (13 * final_T[0]) + Vn_prime;

          my_x = 9 * u_prime * my_y / (4 * v_prime);
          my_z = (12 - 3 * u_prime - 20 * v_prime) * my_y / (4 * v_prime);

          TI[0] =int((RGB[0][0]*my_x + RGB[0][1]*my_y + RGB[0][2]*my_z)*255.0);
          TI[1] =int((RGB[1][0]*my_x + RGB[1][1]*my_y + RGB[1][2]*my_z)*255.0);
          TI[2] =int((RGB[2][0]*my_x + RGB[2][1]*my_y + RGB[2][2]*my_z)*255.0);
 
          if(TI[0]>255) TI[0]=255;
          else if(TI[0]<0) TI[0]=0;
	  
	  if(TI[1]>255) TI[1]=255;
          else if(TI[1]<0) TI[1]=0;
	  
	  if(TI[2]>255) TI[2]=255;
	  else if(TI[2]<0) TI[2]=0;
	}
    }
  else
    {
       TI[0]=(int)final_T[0];
       TI[1]=(int)final_T[1];
       TI[2]=(int)final_T[2];
    }
}

// RGB to LUV conversion
// To gain speed the conversion works on a table of colors (_col_RGB[])
// rather than on the whole image
void SegmenterMS::convert_RGB_LUV( RasterIpChannels* signal, long selects )
{
  int x, y, z, my_temp;
  
  float l_star, u_star, v_star;
  float u_prime, v_prime;
  register int temp_col, temp_index, temp_ind;
  register int j,k;
  
  int a00=XYZ[0][0], a01=XYZ[0][1], a02=XYZ[0][2];
  int a10=XYZ[1][0], a11=XYZ[1][1], a12=XYZ[1][2];
  int a20=XYZ[2][0], a21=XYZ[2][1], a22=XYZ[2][2];
  
  int *A00 = new int[MAXV]; int *A01 = new int[MAXV]; int *A02 = new int[MAXV];
  int *A10 = new int[MAXV]; int *A11 = new int[MAXV]; int *A12 = new int[MAXV];
  int *A20 = new int[MAXV]; int *A21 = new int[MAXV]; int *A22 = new int[MAXV];
  
  for(j=0; j<MAXV;j++)
    {
      A00[j]=a00*j; A01[j]=a01*j; A02[j]=a02*j;
      A10[j]=a10*j; A11[j]=a11*j; A12[j]=a12*j;
      A20[j]=a20*j; A21[j]=a21*j; A22[j]=a22*j;
    }
 
  float *my_pow = new float[MAXV];
  for(j=0; j<MAXV;j++)
    my_pow[j]= 116.0 * pow(j/255.0, 0.3333333) - 16;
 
  Octet* temp_ch0 = signal->chdata(0);
  Octet* temp_ch1 = signal->chdata(1);
  Octet* temp_ch2 = signal->chdata(2);

  int pp;    
  int *temp0, *temp1, *temp2;
  pp = _p_ptr;
  if ( selects & Lightness ) temp0 = _data_all[pp++];
  if ( selects & Ustar ) temp1 = _data_all[pp++];
  if ( selects & Vstar ) temp2 = _data_all[pp++]; 
  _p_ptr=pp;

  for ( j = 0; j < _n_colors; j++)
    {
        temp_col=_col_RGB[j];
	int R=temp_col>>16; int G=(temp_col>>8) & 255; int B=temp_col & 255;

        x = A00[R] + A01[G] + A02[B];
        y = A10[R] + A11[G] + A12[B];
        z = A20[R] + A21[G] + A22[B];
	
        float  tval = y / 2550000.0; //Yn==1
	if ( tval >  Lt)  l_star = my_pow[(int)(tval*255+0.5)];
        else  l_star = 903.3 * tval;

        my_temp = x + 15 * y + 3 * z;
	if(my_temp)
	  {
	    u_prime = (float)(x << 2) / (float)(my_temp);
	    v_prime = (float)(9 * y) / (float)(my_temp);
	  }
	else
	  {
	    u_prime=4.0;
	    v_prime=9.0/15.0;
	  }
	
	tval=13*l_star;
        u_star = tval* (u_prime - Un_prime); // Un_prime = 0.1978
        v_star = tval* (v_prime - Vn_prime); // Vn_prime = 0.4683

	_col0[j] = (int)(l_star+0.5);
	if(u_star>0) _col1[j] = (int)(u_star+0.5);
        else _col1[j] = (int)(u_star-0.5);
  
	if(v_star>0) _col2[j] = (int)(v_star+0.5);
        else _col2[j] = (int)(v_star-0.5);
    }
  for(j=0;j<_ro_col;j++)
    {
      temp_col=(((((int)temp_ch0[j])<<8)+(int)temp_ch1[j])<<8)+(int)temp_ch2[j];
      temp_ind=_col_misc[temp_col>>6];
      for(k=temp_ind;k<temp_ind+SEC_SIZE;k++)
	if(_col_RGB[k]==temp_col)
	  {
           temp_index=_col_index[j]=k;
           break;
	 }
      temp0[j]=_col0[temp_index];
      temp1[j]=_col1[temp_index];
      temp2[j]=_col2[temp_index];
    }   
  delete [] my_pow;
  delete [] A22; delete [] A21; delete [] A20;
  delete [] A12; delete [] A11; delete [] A10;
  delete [] A02; delete [] A01; delete [] A00;
  delete [] _col_misc;
  delete [] _col_RGB;
  //cerr<<":";
}

// 3-D Histogram computation
// Implement a trade-off between speed and required memory
void SegmenterMS::my_histogram(RasterIpChannels* signal, long selects)
{
  int *first_tab= new int[FIRST_SIZE];
  _col_misc= new int[FIRST_SIZE]; 
  int **third_tab;
  int *fourth_tab;
  int *fifth_tab=new int[SEC_SIZE];
  _n_colors=0;
  
  register int k,j,p,r;
  int temp_ind, sec_ind, third_ind;
  
  int first_contor=0, third_contor=0;
  
  memset(first_tab,0,sizeof(int)*FIRST_SIZE);
  memset(_col_misc,0,sizeof(int)*FIRST_SIZE);
  
  register Octet* ch0 = signal->chdata(0);
  register Octet* ch1 = signal->chdata(1);
  register Octet* ch2 = signal->chdata(2);
  
  //first_tab -> how many
  for(k=0;k<_ro_col;k++)
    {
      temp_ind=(((ch0[k]<<8)+ch1[k])<<2)+(ch2[k]>>6);
      first_tab[temp_ind]++;
    }
  //_col_misc -> memo position
  for(k=0;k<FIRST_SIZE;k++)
    if(first_tab[k])
      {
        _col_misc[k]=first_contor;
	first_contor++;
      }
  //contors
  fourth_tab=new int[first_contor];
  memset(fourth_tab,0,sizeof(int)*first_contor);
  //tab of pointers to reduced colors
  third_tab=new int *[first_contor]; 
  first_contor=0;
  for(k=0;k<FIRST_SIZE;k++)
    if(first_tab[k])
      { 
        third_tab[first_contor]=new int[first_tab[k]];       
	first_contor++;
      }
        
  for(k=0;k<_ro_col;k++)
    {
      temp_ind=(((ch0[k]<<8)+ch1[k])<<2)+(ch2[k]>>6);
      sec_ind=ch2[k] & 63;
      third_ind=_col_misc[temp_ind];      
      third_tab[third_ind][fourth_tab[third_ind]]=sec_ind;
      fourth_tab[third_ind]++;      
    }
  for(k=0;k<first_contor;k++)
    {
      memset(fifth_tab,0,sizeof(int)*SEC_SIZE);
      for(j=0;j<fourth_tab[k];j++)
        fifth_tab[third_tab[k][j]]++;
      for(j=0;j<SEC_SIZE;j++)
        if(fifth_tab[j])
	   _n_colors++;
      }
  _col_RGB=new int[_n_colors];
  _m_colors=new int[_n_colors];
  
  k=0;p=0;
  for(r=0;r<FIRST_SIZE;r++)
    if(first_tab[r])
     {
       memset(fifth_tab,0,sizeof(int)*SEC_SIZE);
       for(j=0;j<fourth_tab[k];j++)
         fifth_tab[third_tab[k][j]]++;
       _col_misc[r]=p;
       for(j=0;j<SEC_SIZE;j++)
         if(fifth_tab[j])
	   {	     
	     _col_RGB[p]=(r<<6)+j;
	     _m_colors[p]=fifth_tab[j];
             p++;
	   }
       delete [] third_tab[k];
       k++;
     }
  delete [] third_tab;
  delete [] fourth_tab;
  delete [] fifth_tab;
  delete [] first_tab;
  
  _col_all = new int*[3];
  _col0=_col_all[0] = new int[_n_colors];
  _col1=_col_all[1] = new int[_n_colors];
  _col2=_col_all[2] = new int[_n_colors];
  _col_index = new int[_ro_col];
  //cerr<<":";
}    

// Update _col_remain[], _m_col_remain, and _n_col_remain
void SegmenterMS::my_actual(Octet *my_class)
{ 
  register int i;
  int temp_contor=n_remain;
  register int *temp_rem= new int[_ro_col];
  memcpy(temp_rem,gen_remain,sizeof(int)*temp_contor);
  n_remain=0;	
  for(i=0;i<temp_contor;i++)
    if(!my_class[temp_rem[i]])
      gen_remain[n_remain++]=temp_rem[i];
  delete [] temp_rem;
  memset(_col_remain,0,sizeof(int)*_n_col_remain); 
  memset(_m_col_remain,0,sizeof(int)*_n_colors);  
  _n_col_remain=0; 
  for(i=0;i<n_remain;i++)
    _m_col_remain[_col_index[gen_remain[i]]]++;
  for(i=0;i<_n_colors;i++)
     if(_m_col_remain[i])
       {
	 _col_remain[_n_col_remain]=i;
         _n_col_remain++;
       }
}

// if more than "how_many"  neighbors, consider the point
void SegmenterMS::test_neigh(Octet* my_class, int *selected, int* my_contor, int how_many)
{ 
  register int i,j,p,k;
  register Octet* local_class=my_class;
  register int temp_contor=*my_contor;
  register int my_index;
  if(auto_segm) my_index=n_remain;
  else          my_index=_ro_col;
  for ( p = 0, i; p < my_index; p++ )
    {
      if(auto_segm) i=gen_remain[p];
      else          i=p;
      if(!local_class[i])
	{
          int neigh_contor=0, no_neigh=1;
          for(j=0;j<8;j++)
            {
              k=i+my_neigh[j];
              if(k>=0 && k<_ro_col && local_class[k])
	        {
                  if(auto_segm  && gen_class[k]!=255) continue;
	          neigh_contor++;
                  if(neigh_contor>how_many)
		    {
		      no_neigh=0;
                      break;
		    }
	        }
	    }
        if(!no_neigh)
	  {
            if(auto_segm) selected[*my_contor]=i;
	    *my_contor=*my_contor+1;
	  }
	}
    }
  for(i=temp_contor;i<*my_contor;i++)
    local_class[selected[i]]=1;
}


// Find the feature vectors inside the given window
// Use Improved Absolute Error Inequality Criterion 
// when computing Euclidean distance
// See J.S.Pan el al, Fast Clustering Alg. for VQ, Pattern Recognition,
// Vol. 29, No. 3, pp. 511-518, 1996

void SegmenterMS::new_auto_loop(float *final_T, Octet *sel_col)
{
  float L,U,V,RAD2,R;
  register int TT0=0, TT1=0, TT2=0;
  register int local_contor=0;
  float final_T0=final_T[0], final_T1=final_T[1], final_T2=final_T[2];
  float RADIUS_S2=SQRT2*RADIUS, RADIUS_S3=SQRT3*RADIUS;

  for ( register int p = 0, k; p < _n_col_remain; p++ )
	  {
	      k=_col_remain[p];
              L=_col0[k]-final_T0; if((L=my_abs(L))>=RADIUS) continue;
              U=_col1[k]-final_T1; if((R=my_abs(U)+L)>=RADIUS_S2) continue;
	      V=_col2[k]-final_T2; if(R+my_abs(V)>=RADIUS_S3) continue; 
              RAD2=L*L+U*U+V*V; 
              if(RAD2<RADIUS2)
		{
                  register int r=_m_col_remain[k];
                  TT0+=_col0[k]*r; TT1+=_col1[k]*r; TT2+=_col2[k]*r;
		  local_contor+=r;
                  sel_col[k]=1;
		}
	   }
  final_T[0]=(float)TT0/(float)local_contor;
  final_T[1]=(float)TT1/(float)local_contor;
  final_T[2]=(float)TT2/(float)local_contor;
}

// The same as above, but for non auto_segmentation
void SegmenterMS::nauto_loop(float *final_T, int *selected, 
                              Octet *my_class, int *my_contor)
{
  float L,U,V,RAD2,R;
  register int local_contor=*my_contor;
  float final_T0=final_T[0], final_T1=final_T[1], final_T2=final_T[2]; 
  float RADIUS_S2=SQRT2*RADIUS, RADIUS_S3=SQRT3*RADIUS;

  for ( register int k = 0; k < _n_points; k++ )
	    {  
              L=_data[0][k]-final_T0; if((L=my_abs(L))>=RADIUS)  continue; 
              U=_data[1][k]-final_T1; if((R=my_abs(U)+L)>=RADIUS_S2)  continue;
	      V=_data[2][k]-final_T2; if(R+my_abs(V)>=RADIUS_S3)  continue;
              RAD2=L*L+U*U+V*V; 
              if(RAD2<RADIUS2)
		{
		  selected[local_contor++]=k;
                  my_class[k]=1;
		}
	    }
  *my_contor=local_contor;
}

// Set the Radius of the window
void set_RADIUS(int gen_gen_contor, int final)
{   
  if(final==2)      RADIUS=final_RADIUS*1.26;
  else if(final==1) RADIUS=final_RADIUS;
  else              RADIUS=fix_RADIUS[gen_gen_contor];
  RADIUS2=RADIUS*RADIUS;
}
  
// Test if the clusters have the same mean
int test_same_cluster(int rect, float T[][p_max])
{
  float L,U,V,RAD2;
  for(register int k=0; k<rect;k++)
    {
      L=T[k][0]-T[rect][0]; U=T[k][1]-T[rect][1]; V=T[k][2]-T[rect][2];
      RAD2=L*L+U*U+V*V; 
      if(RAD2<1)
        return 1;
    }
  return 0;
} 

// First take only pixels inside the search windows at their final locations
// Then inflate windows to double volume and retain only pixels which are
// neighbors with the previous 
void SegmenterMS::get_codeblock1(float T[][p_max], int n_rects)
{
  float L,U,V,RAD2, R, min_RAD2;
  int min_ind;
  register int i,k,u;
  register int pres_class, my_flag;
  register float *ptr;
  
  if(auto_segm) set_RADIUS(0,0);
  else          set_RADIUS(2,0);

  for(k=0;k<_ro_col;k++)
    {
      min_RAD2=BIG_NUM; min_ind=0;
      for(i=0;i<n_rects;i++)
	{
          ptr=T[i];
          L=_data0[k]-ptr[0]; if(my_abs(L)>=RADIUS)  continue;
          U=_data1[k]-ptr[1]; if(my_abs(U)>=RADIUS)  continue;
	  V=_data2[k]-ptr[2]; if(my_abs(V)>=RADIUS)  continue;
          RAD2=L*L+U*U+V*V;
          if(RAD2<min_RAD2)
	    {
	      min_RAD2=RAD2;
	      min_ind=i;
	    }
	}
      if(min_RAD2<RADIUS2) gen_class[k]=min_ind;
      else                 gen_class[k]=n_rects;
    }

  if(auto_segm) set_RADIUS(0,1);
  else          set_RADIUS(0,0);
  
  for(k=0;k<_ro_col;k++)
    if(gen_class[k]==n_rects)
      for(i=0;i<8;i++)
	{	     
          u=k+my_neigh[i];
          if(u>=0 && u<_ro_col)
            if((pres_class=gen_class[u])!=n_rects)
	      {
                ptr=T[pres_class];
                L=_data0[k]-ptr[0]; if(my_abs(L)>=RADIUS)  continue;
                U=_data1[k]-ptr[1]; if(my_abs(U)>=RADIUS)  continue;
                V=_data2[k]-ptr[2]; if(my_abs(V)>=RADIUS)  continue;
                RAD2=L*L+U*U+V*V;
	        if(RAD2<RADIUS2) gen_class[k]=pres_class;
	      }
	  }
}

// Final allocation
void SegmenterMS::get_codeblock(float T[][p_max], int n_rects)
{
  float L,U,V,RAD2, min_RAD2;
  register int min_ind;
  register int i,k;
  register float *ptr; 
  
  for(k=0;k<_ro_col;k++)
    {
      min_RAD2=BIG_NUM;
      min_ind=0;
      for(i=0;i<n_rects;i++)
	{
          ptr=T[i];
	  L=_data0[k]-ptr[0]; U=_data1[k]-ptr[1]; V=_data2[k]-ptr[2]; 
          RAD2=L*L+U*U+V*V;
          if(RAD2<min_RAD2)
	    {
	      min_RAD2=RAD2;
	      min_ind=i;
	    }
	}
      gen_class[k]=min_ind;
     } 
}

// Compute the mean of feature vectors mapped into the same color
void SegmenterMS::new_codebook(float T[][p_max], int n_rects)
{
  register int i,k;
  register int *tab_contor = new int[n_rects];
  register int prez_class;
  register float *ptr;
  
  memset(tab_contor,0,sizeof(int)*n_rects); 
  for(i=0;i<n_rects;i++)
    {
      T[i][0]=0.0; T[i][1]=0.0; T[i][2]=0.0;
    }
  for(k=0;k<_ro_col;k++)
   if((prez_class=gen_class[k])!=n_rects)
    {	
      ptr=T[prez_class];
      ptr[0]+=_data0[k]; ptr[1]+=_data1[k]; ptr[2]+=_data2[k];
      tab_contor[prez_class]++;
    }
   for(i=0;i<n_rects;i++)
    {
      T[i][0]/=tab_contor[i]; T[i][1]/=tab_contor[i]; T[i][2]/=tab_contor[i];
    }  
  delete [] tab_contor;
} 

// Determine the final feature palette
void SegmenterMS::optimize(float T[][p_max], int n_rects)
{
  get_codeblock1(T,n_rects);
  new_codebook(T,n_rects);
  if(auto_segm)
    get_codeblock(T,n_rects);
  //cerr<<":";
}


// Inverse of the mapping array used in color elimination
void reverse_map(Octet *inv_map, Octet *my_map, int *n_rects, Octet *valid_class, float T[][p_max])
{
  float sec_T[Max_rects][p_max];
  register int u=0, k, j;
  for(j=0;j<*n_rects;j++)
    {
      if(valid_class[j])
	{
          for(k=0;k<3;k++)
            sec_T[u][k]=T[j][k];
          my_map[j]=u;
          inv_map[u]=j;
          u++;
        }
    }
  my_map[*n_rects]=u;
  inv_map[u]=*n_rects;
  *n_rects=u;
  for(j=0;j<*n_rects;j++)
    for(k=0;k<3;k++) 
      T[j][k]=sec_T[j][k];
}

// Eliminate colors that have less than "my_lim" connected pixels
void SegmenterMS::eliminate_class(Octet *my_class,int *my_max_region, int *n_rects,int my_lim, Octet* inv_map, float T[][p_max], REGION *first_region)
{
  register int j, k;
  register Octet *valid_class; 
  register REGION *current_region=first_region;
  
  valid_class=new Octet[*n_rects]; 
  for(j=0;j<*n_rects;j++)
    {
      if(my_max_region[j]<my_lim) valid_class[j]=0;
      else                        valid_class[j]=1;
    }
  while(1)
    {
      if((current_region->my_class<*n_rects && 
          !valid_class[current_region->my_class]))
        for(k=0;k<current_region->my_contor;k++)
           gen_class[current_region->my_region[k]]=*n_rects;
      if(current_region->next_region_str)  
        current_region=current_region->next_region_str;
      else break;
    }
  Octet my_map[Max_rects];
  reverse_map(inv_map,my_map,n_rects,valid_class,T);
  for(k=0;k<_ro_col;k++)
    my_class[k]=my_map[gen_class[k]];
  delete [] valid_class;
  memcpy(gen_class,my_class,_ro_col);
}

// Eliminate regions with less than "my_lim" pixels
void SegmenterMS::eliminate_region(int *n_rects,int my_lim, float T[][p_max], REGION* first_region)
{    
  register int j,u,k,p, pres_class, min_ind;
  register REGION *current_region=first_region;
  register int* region;
  float *ptr;
  float L,U,V,RAD2,minRAD2;
  int increm;
  
  while(1)
    {
      if(current_region->my_contor<my_lim)
	{
          set_RADIUS(0,0); increm=4;
	  region=current_region->my_region;
          for(k=0;k<current_region->my_contor;k++)
            gen_class[region[k]]=*n_rects;
	  while(1)
            { 
              Boolean my_flag=0;	
              RADIUS+=increm; RADIUS2=RADIUS*RADIUS; increm+=4;
              for(k=0;k<current_region->my_contor;k++)
	        if(gen_class[p=region[k]]==(*n_rects))
	          {  
	            minRAD2=RADIUS2;
                    for(j=1;j<8;j+=2)
                      {
                        u=p+my_neigh[j];
                        if(u>=0 && u<_ro_col)
                        if((pres_class=gen_class[u])!=(*n_rects))
		          {
                            ptr=T[pres_class];
		            L=_data0[p]-ptr[0]; U=_data1[p]-ptr[1];
                            V=_data2[p]-ptr[2]; RAD2=L*L+U*U+V*V;
		            if(RAD2<minRAD2)
		              {
                                minRAD2=RAD2; min_ind=pres_class;
			      }
		          }
	              }
	           if(minRAD2<RADIUS2) gen_class[p]=min_ind;
                   my_flag=1;
	         }
	       if(!my_flag) break;
	    }
	}
      if(current_region->next_region_str) 
        current_region=current_region->next_region_str;
      else break;
    }
}  

// Destroy the region list
void SegmenterMS::destroy_region_list(REGION *first_region)
{ 
  register REGION *current_region=first_region;
  while(1)
    {
      delete [] current_region->my_region;
      first_region=current_region;
      if(current_region->next_region_str)
        {
          current_region=current_region->next_region_str;
          delete first_region;
	}
      else
        { 
          delete first_region; 
          break;
	}
    }
}

// Connected component main routine
void SegmenterMS::find_other_neigh(int k, int *my_ptr_tab, REGION *current_region)
{
  register int *ptr_tab=my_ptr_tab;
  register int i,u, j=k, sec_signal;
  register int contor=0;
  register int region_contor=current_region->my_contor;
  register int region_class=current_region->my_class;
  ptr_tab[contor]=j;
         
  while(1)
    {
      sec_signal=0;
      for(i=1;i<9;i+=2)
	{
	  u=j+my_neigh[i];  
          if(u>=0 && u<_ro_col)      
          if(gen_class[u]==region_class && !taken[u])
	    {
              sec_signal=1;
	      conn_selected[region_contor++]=u;
	      taken[u]=1;
              ptr_tab[++contor]=u;
            }
	}
      if(sec_signal) j=ptr_tab[contor];
      else
	 {	     
           if(contor>1) j=ptr_tab[--contor];
	   else break;
	 }
    }
  current_region->my_contor=region_contor;
}

// Create the region list
REGION *SegmenterMS::create_region_list(int *my_max_region, int change_type)
{
  register int k, local_label=0;
  register REGION *first_region, *prev_region, *current_region;
  taken = new Octet[_ro_col];
  memset(taken,0,_ro_col);
  conn_selected = new int[_ro_col];
  int *ptr_tab=new int[_ro_col];
  
  for(k=0;k<_ro_col;k++)
    if(!taken[k])
      {	
        current_region=new REGION;
        current_region->my_contor=0;
        current_region->my_class=gen_class[k];
        current_region->my_label=local_label;
        if(k!=0) prev_region->next_region_str=current_region;
	if(k==0){ first_region=current_region;}
	
	local_label++;
	conn_selected[current_region->my_contor++]=k;
        taken[k]=1;
        find_other_neigh(k,ptr_tab,current_region);
        if(change_type==0)
	  if(my_max_region[current_region->my_class]<current_region->my_contor)
            my_max_region[current_region->my_class]=current_region->my_contor; 
        current_region->my_region=new int[current_region->my_contor];

        memcpy(current_region->my_region,conn_selected,sizeof(int)*current_region->my_contor);    
        prev_region=current_region;
       }  
  current_region->next_region_str=0;
  
  delete [] ptr_tab; delete [] taken; delete [] conn_selected;
  return first_region;
}  

// Find connected components and remove small regions of classes 
// with small regions 
void SegmenterMS::conn_comp(Octet *my_class, int *n_rects, Octet *inv_map, float T[][p_max],int my_lim, int change_type)
{
  REGION *first_region; 
  int *my_max_region;
  if(change_type==0)
    {
      my_max_region = new int[(*n_rects)+1];
      memset(my_max_region,0,sizeof(int)*((*n_rects)+1));
    }
  first_region=create_region_list(my_max_region, change_type);
  if(change_type==0)  //elliminate classes with small regions
    eliminate_class(my_class,my_max_region,n_rects,my_lim,inv_map,T,first_region); 
  else if(change_type==1)  //elliminate small regions
    eliminate_region(n_rects,my_lim,T,first_region);
  destroy_region_list(first_region);
  if(change_type==0) delete [] my_max_region;
  //cerr<<":";
}
   

// Cut a rectangle from the entire input data
// Deletes the previous rectangle, if any
void SegmenterMS::cut_rectangle( sRectangle* rect )
{
  if ( _data ) {
    for ( register int i = 0; i < _p; i++ )
      if ( _data[i] )   delete [] _data[i];
    delete [] _data;
  }

  // Set the dimensions of the currently processed region.
  _rrows  = rect->height;
  _rcolms = rect->width;
  _data = new int*[_p];

  register int my_x = rect->x;
  register int my_y = rect->y;
  register int i, j, d;
  for ( i = 0; i < _p; i++ )
      _data[i] = new int[_rcolms*_rrows];

  if(auto_segm)
    for ( d = 0; d < _p; d++ )
      memcpy(_data[d], _data_all[d],sizeof(int)*_ro_col);
  else
    { 
      int	idx1 = my_y * _colms + my_x;
      int	idx2 = 0;
      for ( j = my_y, d;
            j < my_y + _rrows; j++, idx1 += _colms - _rcolms )
        for ( i = my_x; i < my_x + _rcolms; i++, idx1++, idx2++ )
          {
            for ( d = 0; d < _p; d++ )
	      _data[d][idx2] = _data_all[d][idx1];
          }
    }
  //cerr<<":";
}

// Compute the mean of N points given by J[]
void mean_s(const int N, const int p, int J[], int **data, float T[])
{
  int TT[p_max];
  register int k, i, j;
  for ( i = 0; i < p; i++ )
    TT[i] = 0;
  for ( i = 0; i < N; i++ )
    {
      k = J[i];
      for ( j = 0; j < p; j++ )
        TT[j] += data[j][k];
     }
   for ( i = 0; i < p; i++ )
      T[i] = (float)TT[i] / (float)N;
}

// Build a subsample set of 9 points
int SegmenterMS::subsample(float *Xmean )
{
  int J[9];
  register int my_contor=0, uj, i0;
  if(auto_segm)
    i0=J[my_contor]=
       gen_remain[int(float(n_remain)*float(rand())/float(RAND_MAX))]; 
  else
    i0=J[my_contor]=int(float(_n_points)*float(rand())/float(RAND_MAX));
  my_contor++;
  for(int i=0;i<8;i++){
    uj=i0 + my_neigh_r[i];
    if(uj>=0 && uj<_n_points)
      {
        if((auto_segm && gen_class[uj]!=255)) break;
	else
          {        
            J[my_contor] = uj;
            my_contor++;
	  }      
      }
    }
  mean_s(my_contor, _p, J, _data, Xmean);
  return 1;
} 

// Sampling routine with all needed tests
float SegmenterMS::my_sampling( int rect, float T[Max_rects][p_max])
{ 
  register int  k, c;
  register float L,U,V,Res;
  register float my_dist=max_dist, my_sqrt_dist=fix_RADIUS[0];
  float	TJ[Max_J][p_max];
  int	l =  0;       //contor of number of subsample sets
  int	ll = 0;       //contor of trials
  float	Xmean[p_max];
  float	Obj_fct[Max_J];
 
  //Max_trials = max number of failed trials
  //_NJ = max number of subsample sets
   
  while ( (ll < Max_trials) && (l < _NJ ) )
    {
      if ( subsample(Xmean) )    // the subsample procedure succeeded
	{ 
	      ll = 0; c=0;

	      // Save the mean
	      for ( k = 0; k < _p; k++ ) TJ[l][k] = Xmean[k];

	      // Compute the square residuals (Euclid dist.)
	      if(auto_segm)
                  for ( register int p = 0; p < _n_col_remain; p++ )
		    {
		     k=_col_remain[p];
                     L=_col0[k]-Xmean[0]; if(my_abs(L)>=my_sqrt_dist) continue;
		     U=_col1[k]-Xmean[1]; if(my_abs(U)>=my_sqrt_dist) continue;
		     V=_col2[k]-Xmean[2]; if(my_abs(V)>=my_sqrt_dist) continue;
		     if(L*L+U*U+V*V<my_dist) c+=_m_col_remain[k];
		    }
	      else
	          for ( k = 0; k < _n_points; k++ )
		    {
                     L=_data[0][k]-Xmean[0]; 
                     if(my_abs(L)>=my_sqrt_dist) continue; 
		     U=_data[1][k]-Xmean[1]; 
                     if(my_abs(U)>=my_sqrt_dist) continue; 
		     V=_data[2][k]-Xmean[2]; 
                     if(my_abs(V)>=my_sqrt_dist) continue; 
                     if(L*L+U*U+V*V<my_dist) c++;
		    }
	      
              //  Objective functions
	      Obj_fct[l]=c;
              l++;
	    }
      else ++ll;	
    }
  if ( ll == Max_trials && l < 1) return( BIG_NUM ); // Cannot find a kernel

  //  Choose the highest density  
  L = -BIG_NUM; c=0;
  for ( k = 0; k < _NJ; k++ )
      if ( Obj_fct[k] > L)
	{
	  L = Obj_fct[k];
	  c = k;
	}
  if(Obj_fct[c]>0)
    for(k=0;k<_p;k++)
       T[rect][k]=TJ[c][k];
  else return -BIG_NUM; // Not enough points
  return ( 0 );
}

// Compute the weighted covariance of N points 
void covariance_w(const int N, int M, const int p,  int **data, 
               int *w, float T[], float C[p_max][p_max])
{
  register int i, j, k, l;
  int TT[p_max];
  for ( i = 0; i < p; i++ )
    TT[i] = 0;
  for ( i = 0; i < M; i++ )
    for ( j = 0; j < p; j++ )
      TT[j] += w[i]*data[j][i]; 
  for ( i = 0; i < p; i++ )
      T[i] = (float) TT[i] / (float)N;

  for ( i = 0; i < p; i++ )
    for ( j = i; j < p; j++ )
      C[i][j] = 0.0;
  for ( i = 0; i < M; i++ )
    {
      for ( k = 0; k < p; k++ )
	for ( l = k; l < p; l++ )
	   C[k][l]+=w[i]*(data[k][i]-T[k])*(data[l][i]-T[l]);
    }
  for ( k = 0; k < p; k++ )
    {
    for ( l = k; l < p; l++ )
      C[k][l] /= (float)(N-1);
    for ( l = 0; l < k; l++ )
      C[k][l] = C[l][k];
    }
}

// initialization
void SegmenterMS::init_neigh(void)
{
  my_neigh[0]= -_colms-1;  my_neigh[1]= -_colms;
  my_neigh[2]= -_colms+1;  my_neigh[3]= +1;
  my_neigh[4]= +_colms+1;  my_neigh[5]= +_colms;
  my_neigh[6]= +_colms-1;  my_neigh[7]= -1;

  my_neigh_r[0]= -_rcolms-1;  my_neigh_r[1]= -_rcolms;
  my_neigh_r[2]= -_rcolms+1;  my_neigh_r[3]= +1;
  my_neigh_r[4]= +_rcolms+1;  my_neigh_r[5]= +_rcolms;
  my_neigh_r[6]= +_rcolms-1;  my_neigh_r[7]= -1;
}

// Init matrices parameters
void SegmenterMS::init_matr(void)
{
  // General statistic parameters for X.
  float	Mg[p_max];         //sample mean of X
  float	C[p_max][p_max];   //sample covariance matrix of X
  covariance_w(_ro_col, _n_colors, _p, _col_all, _m_colors, Mg, C);

  // Adaptation
  float my_th=C[0][0]+C[1][1]+C[2][2];
  int active_gen_contor=1;
  
  if(auto_segm)
     fix_RADIUS[0]=gen_RADIUS[option]*sqrt(my_th/100);
  else
    {  
      active_gen_contor=rect_gen_contor;
      for(int i=0;i<active_gen_contor;i++)
        fix_RADIUS[i]=rect_RADIUS[i]*sqrt(my_th/100);
    }
  final_RADIUS=fix_RADIUS[active_gen_contor-1]*1.26;
  max_dist=fix_RADIUS[0]*fix_RADIUS[0];
#ifdef TRACE
  printf("\n %.2f %.2f ", fix_RADIUS[0], final_RADIUS);
#endif
  act_threshold=(int)((my_threshold[option]>sqrt(_ro_col)/my_rap[option])? 
                 my_threshold[option]:sqrt(_ro_col)/my_rap[option]);
  //cerr<<":";
} 

// Init
void SegmenterMS::initializations(RasterIpChannels* pic, sRectangle rects[], int *n_rects, long selects, int *active_gen_contor)
{
  register int i; 
  XfRaster::Info        info;
  pic->raster_info(info);
  _colms = info.columns; _rows  = info.rows; _ro_col = _rows * _colms;

  _data_all = new int*[_p];
  for ( i = 0; i < _p; i++ )
      _data_all[i] = new int[_ro_col];
  _data0=_data_all[0]; _data1=_data_all[1]; _data2=_data_all[2];
  init_neigh();
  my_histogram(pic, selects);
  convert_RGB_LUV( pic, selects );
  gen_class = new Octet[_ro_col];
  memset(gen_class,255,_ro_col);
  if(!(*n_rects))
    {
      auto_segm=1;
      *n_rects=Max_rects;
      n_remain=_ro_col;
      _n_col_remain=_n_colors;
      gen_remain = new int[_ro_col];
      _col_remain = new int[_n_colors];
      _m_col_remain = new int[_n_colors];
      for ( i = 0; i< _ro_col ; i++ )
        gen_remain[i] = i;
      for ( i = 0; i< _n_colors ; i++ )
        _col_remain[i] = i;
      memcpy(_m_col_remain,_m_colors,sizeof(int)*_n_colors);

      for ( i = 0; i < Max_rects ; i++ )
        {
          rects[i].width=_colms; rects[i].height=_rows;
          rects[i].x = 0;        rects[i].y = 0;
        }
      *active_gen_contor=1;
    }
  else
    {
      auto_segm=0;
      n_remain=0;
      *active_gen_contor=rect_gen_contor;
      option=2;
    }
  init_matr();
  delete [] _m_colors;
}

// Mean shift segmentation applied on the selected region(s) of an image or
// on the whole image
Boolean SegmenterMS::ms_segment( RasterIpChannels* pic, sRectangle rects[],
				     int n_rects, long selects , 
                                     unsigned int seed_default, Boolean block)
{
  int t=time(0);
  if (n_rects > Max_rects) return false;
  if ( selects & Lightness || selects & Ustar || selects & Vstar ) 
    _p=3;
  else return false;

  int contor_trials=0, active_gen_contor;
  float	T[Max_rects][p_max];
  int TI[Max_rects][p_max];
  float L,U,V,RAD2,q;
  register int i,k;
  
  srand( seed_default ); 
  //cerr<<":";
  initializations(pic, rects, &n_rects, selects, &active_gen_contor);

  // Mean shift algorithm and removal of the detected feature
  int rect;
  for ( rect = 0; rect < n_rects; rect++ )
    {
      _n_points = rects[rect].width * rects[rect].height;
      cut_rectangle( &rects[rect] );
     
    MS:
      if(auto_segm && contor_trials) _NJ=1;
      
      q =  my_sampling( rect, T);
      if(q == -BIG_NUM || q == BIG_NUM)
	  if(contor_trials++<MAX_TRIAL)  goto MS;
	  else                           break;
      
      float final_T[p_max];
      for ( i = 0; i < _p; i++ ) final_T[i]=T[rect][i];

      int *selected = new int[_ro_col];
      Octet *sel_col = new Octet[_n_colors];
      Octet *my_class = new Octet[_ro_col];
       
      int my_contor=0, gen_gen_contor=-1, how_many=10; 
      while(gen_gen_contor++<active_gen_contor-1)
        {
          set_RADIUS(gen_gen_contor, 0);
	  int gen_contor=0; 
          Boolean last_loop=0;
	 
          while(gen_contor++<how_many)
            { 
              if(auto_segm)
		{
                  memset(sel_col,0,_n_colors);
                  new_auto_loop(final_T,sel_col); 
                  L=T[rect][0]-final_T[0]; U=T[rect][1]-final_T[1];
                  V=T[rect][2]-final_T[2]; RAD2=L*L+U*U+V*V; 
                  if(RAD2<0.1){
                    my_contor=0;
                    memset(my_class,0,_ro_col);     
                    for(k=0;k<n_remain;k++)
		      {
                        register int p=gen_remain[k];
			if(sel_col[_col_index[p]])
			  {
			    selected[my_contor++]=p;
                            my_class[p]=1;
			  }
		      }
                    break;
		    }
		  else
                  {
	            T[rect][0]=final_T[0];T[rect][1]=final_T[1];
                    T[rect][2]=final_T[2];
                   }
	         }
	       else
		 {
		    my_contor=0;
                    memset(my_class,0,_ro_col);     
	            nauto_loop(final_T, selected, my_class,&my_contor);
                    mean_s(my_contor, _p, selected, _data, final_T);
                    L=T[rect][0]-final_T[0]; U=T[rect][1]-final_T[1];
                    V=T[rect][2]-final_T[2]; RAD2=L*L+U*U+V*V; 
    
                    if(RAD2<0.1)
                      break;
                    else
                     {
	               T[rect][0]=final_T[0];T[rect][1]=final_T[1];
                       T[rect][2]=final_T[2];
                     } 
		  }
	    }
	}
      if(auto_segm)
	{
          if(option==0) test_neigh(my_class, selected, &my_contor,2);
          else if(option==1) test_neigh(my_class, selected, &my_contor,1);
          else if(option==2) test_neigh(my_class, selected, &my_contor,0);
	}      
#ifdef TRACE   
      printf("my_contor = %d contor_trials = %d",my_contor,contor_trials);  
#endif	
      if(!auto_segm)
        if(test_same_cluster(rect,T))
	  {
             delete [] my_class; delete [] sel_col; delete [] selected;
             continue;
	  }
      if(auto_segm && my_contor<act_threshold)
        {
          delete [] my_class; delete [] sel_col; delete [] selected;
          if(contor_trials++<MAX_TRIAL) goto MS;
          else                          break;
        }
      
      if(auto_segm) my_actual(my_class);

      my_convert(selects, final_T, TI[rect]);  
      for ( k = 0; k < _ro_col; k++ )
        if(my_class[k])
          gen_class[k]=rect;
     
      delete [] my_class; delete [] sel_col; delete [] selected;
    }     			
  n_rects=rect;
  Octet** isegment = new Octet*[3];	
  register int j;
  for ( j = 0; j < 3; j++ )
    {
      isegment[j] = new Octet[_ro_col];
      memset( isegment[j], 0, _ro_col );
    }
  Octet *isegment0=isegment[0]; Octet *isegment1=isegment[1];
  Octet *isegment2=isegment[2];
 
  if(auto_segm)
    {
      Octet *my_class = new Octet[_ro_col];
      for ( k = 0; k < _ro_col; k++ )
         if(gen_class[k]==255) gen_class[k]=n_rects;

      Octet inv_map[Max_rects];
      for(k=0;k<n_rects;k++) inv_map[k]=k;

      if(option==0)  conn_comp(my_class,&n_rects,inv_map,T,10,0);
      else if(option==1) conn_comp(my_class,&n_rects,inv_map,T,20,0); 
      else conn_comp(my_class,&n_rects,inv_map,T,act_threshold,0); 
      optimize(T,n_rects);
      //printf("\n");
      for(k=0;k<n_rects;k++) 
	{
	  my_convert(selects, T[k], TI[k]);
	  //printf("--> color %d (%d,%d,%d)\n", k, TI[k][0], TI[k][1], TI[k][2]);
	}
      
      // Postprocessing
      if(option==1 || option ==2)
          for(k=2;k<10;k+=2) conn_comp(my_class,&n_rects,inv_map,T,k,1);
#ifdef TRADE
      printf("\nTime = %d seconds\n",time(0)-t);
#endif 
      register Octet my_max_ind; 
      for ( k = 0; k < _ro_col; k++ )
        {
          if((my_max_ind=gen_class[k])!=n_rects)
	    {
              int *ti=TI[my_max_ind];
	      isegment0[k]=ti[0]; isegment1[k]=ti[1]; isegment2[k]=ti[2];
	    } 
	}

      // Save the borders
      memset(my_class,255,_ro_col); 
      for ( k = 0; k < _ro_col; k++ )
	{
          int pres_class=gen_class[k];
          int pos_colms=k%_colms;
	      
          if(pos_colms==0 || pos_colms==(_colms-1) ||
             k<(_colms-1) || k>(_ro_col-_colms))
             my_class[k] = 0;
          else
             for(j=1;j<8;j+=2)
               {
                 int u=k+my_neigh[j];
                 if(u>=0 && u<_ro_col &&  (pres_class<gen_class[u]))
		   {
		     my_class[k] = 0; break;
		   }
	        }
	}
      //my_write_PGM_file( "result.pgm", my_class,_rows,_colms);
      delete [] my_class;
    }
  else   // if not auto_segmentation
    {
      optimize(T,n_rects);
      for(k=0;k<n_rects;k++)
        my_convert(selects, T[k], TI[k]);
      register int temp_class;
      for (k=0;k<_ro_col;k++)
	  {
            if((temp_class=gen_class[k])<n_rects)
	      {
                 isegment0[k] = /*TI[temp_class][0];*/pic->chdata(0)[k];
	         isegment1[k] = /*TI[temp_class][1];*/pic->chdata(1)[k];
	         isegment2[k] = /*TI[temp_class][2];*/pic->chdata(2)[k];
	       }
	    } 
     } 
  if(auto_segm){
    delete [] _m_col_remain; 
    delete [] _col_remain;
    delete [] gen_remain;
  }
  delete [] gen_class;
  delete [] _col_index;
  delete [] _col0; delete [] _col1; delete [] _col2; 
  delete [] _col_all;
  
  XfRaster::Info	info;
  pic->raster_info(info);
  result_ras_ = new RasterIpChannels( info, 3, eDATA_OCTET,
				      isegment, true );
  //cerr << endl << "Original Colors: " << _n_colors << endl;
  //cerr << "Segment. Colors: " << n_rects << endl;
  
  return true;  
}







