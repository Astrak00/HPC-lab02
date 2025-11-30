#include "hist-equ.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void histogram(int * hist_out, unsigned char * img_in, int img_size, int nbr_bin) {
  int i;
#pragma omp parallel for
  for (i = 0; i < nbr_bin; i++) { hist_out[i] = 0; }

#pragma omp parallel for reduction(+ : hist_out[ : nbr_bin])
  for (i = 0; i < img_size; i++) { hist_out[img_in[i]]++; }
}

void histogram_equalization(unsigned char * img_out, unsigned char * img_in, int * hist_in,
                            int img_size, int nbr_bin) {
  int * lut = (int *) malloc(sizeof(int) * nbr_bin);
  int i = 0, cdf = 0, min = 0;
  /* Construct the LUT by calculating the CDF */

  while (min == 0) { min = hist_in[i++]; }

  float const scale = 255.0f / (img_size - min);
  for (i = 0; i < nbr_bin; i++) {
    cdf     += hist_in[i];
    float v  = (cdf - min) * scale;

    // Removed the possible branching when obtaining the new value (next loop)
    if (v < 0.0f) { v = 0.0f; }
    if (v > 255.0f) { v = 255.0f; }
    // This way we only do the multiplication if necessary
    lut[i] = (int) (v + 0.5f);
  }

  /* Get the result image */
#pragma omp parallel for schedule(static)
  for (i = 0; i < img_size; i++) {
    img_out[i] = (unsigned char) lut[img_in[i]];
    // if (lut[img_in[i]] > 255) {
    //   img_out[i] = 255;
    // } else {
    //   img_out[i] = (unsigned char) lut[img_in[i]];
    // }
  }
  free(lut);
}
