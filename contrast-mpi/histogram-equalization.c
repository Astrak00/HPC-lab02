#include "hist-equ.h"

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Add here SIMD,
void histogram(int * hist_out, unsigned char * img_in, int img_size, int nbr_bin) {
  int i;
  memset(hist_out, 0, sizeof(int) * nbr_bin);

#pragma omp parallel for reduction(+ : hist_out[ : nbr_bin])
  for (i = 0; i < img_size; i++) { hist_out[img_in[i]]++; }
}

void histogram_equalization(unsigned char * img_out, unsigned char * img_in, int * hist_in,
                            int img_size, int nbr_bin) {
  int * lut = (int *) malloc(sizeof(int) * nbr_bin);
  int i, cdf, min, d;
  int global_img_size = 0;
  /* Construct the LUT by calculating the CDF */
  cdf = 0;
  min = 0;
  i   = 0;
  while (i < nbr_bin && min == 0) { min = hist_in[i++]; }

  for (i = 0; i < nbr_bin; i++) { global_img_size += hist_in[i]; }

  d = global_img_size - min;
  if (d <= 0) {
    memcpy(img_out, img_in, sizeof(unsigned char) * img_size);
    free(lut);
    return;
  }
  for (i = 0; i < nbr_bin; i++) {
    cdf += hist_in[i];
    // lut[i] = (cdf - min)*(nbr_bin - 1)/d;
    lut[i] = (int) (((float) cdf - min) * 255 / d + 0.5);
    if (lut[i] < 0) { lut[i] = 0; }
  }

/* Get the result image */
#pragma omp parallel for
  for (i = 0; i < img_size; i++) {
    if (lut[img_in[i]] > 255) {
      img_out[i] = 255;
    } else {
      img_out[i] = (unsigned char) lut[img_in[i]];
    }
  }

  free(lut);
}
