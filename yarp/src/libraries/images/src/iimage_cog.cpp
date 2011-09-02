#include <malloc.h>


typedef struct {
   unsigned int **ptr;
   int w;
   int h;
} IIMAGE;


void compute_integral_image(unsigned char *src, IIMAGE *dest)
{
  int i, j;

  for (i=0; i<dest->h; i++) {
    for (j=0; j<dest->w; j++) {
      if (j>0)
        dest->ptr[i][j] = dest->ptr[i][j-1] + (int)src[i*dest->w+j];
      else
        dest->ptr[i][j] = (int)src[i*dest->w+j];
    }
  }

  for (j=0; j<dest->w; j++) {
    for (i=1; i<dest->h; i++) {
      dest->ptr[i][j] += dest->ptr[i-1][j];
    }
  }
}


void compute_squared_integral_image(unsigned char *src, IIMAGE *dest)
{
  int i, j;
  
  for (i=0; i<dest->h; i++) {
    for (j=0; j<dest->w; j++) {
      if (j>0)
        dest->ptr[i][j] = dest->ptr[i][j-1] + (int)src[i*dest->w+j] * (int)src[i*dest->w+j];
      else
        dest->ptr[i][j] = (int)src[i*dest->w+j] * (int)src[i*dest->w+j];
      }
    }

  for (j=0; j<dest->w; j++) {
    for (i=1; i<dest->h; i++) {
      dest->ptr[i][j] += dest->ptr[i-1][j];
    }
  }
}


double compute_mean(IIMAGE *iimage, int tlx, int tly, int length)
{
  unsigned int s=0;

  if ((tlx > 0) && (tly > 0))
    s += iimage->ptr[tly-1][tlx-1];

  if ((tly > 0) && (tlx+length < iimage->w))
    s -= iimage->ptr[tly-1][tlx+length];

  if ((tlx > 0) && (tly+length < iimage->h))
    s -= iimage->ptr[tly+length][tlx-1];

  s += iimage->ptr[tly+length][tlx+length];

  return (double)s/(double)((length+1)*(length+1));
}


void alloc_iimage(IIMAGE *img)
{
  int i;

  img->ptr = (unsigned int **)malloc(img->h*sizeof(unsigned int *));
  for (i=0; i<img->h; i++)
    img->ptr[i] = (unsigned int *)malloc(img->w*sizeof(unsigned int));
}


void free_iimage(IIMAGE *img)
{
  int i;

  for (i=0; i<img->h; i++)
    free(img->ptr[i]);
  free(img->ptr);
}

