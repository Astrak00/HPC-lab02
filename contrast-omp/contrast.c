#include "hist-equ.h"

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void run_cpu_color_test(PPM_IMG img_in);
void run_cpu_gray_test(PGM_IMG img_in);

static inline void log_timing(FILE * stream, char const * label, double seconds) {
  fprintf(stream, "%.3f (ms) \t taken for %s\n", seconds * 1000.0, label);
}

int main(int argc, char * argv[]) {
  int err;
  double start_time_global = omp_get_wtime();

  PGM_IMG img_ibuf_g;
  PPM_IMG img_ibuf_c;

  // printf("Running contrast enhancement for gray-scale images.\n");
  img_ibuf_g = read_pgm("in.pgm");
  run_cpu_gray_test(img_ibuf_g);
  free_pgm(img_ibuf_g);

  // printf("Running contrast enhancement for color images.\n");
  img_ibuf_c = read_ppm("in.ppm");
  run_cpu_color_test(img_ibuf_c);
  free_ppm(img_ibuf_c);
  double end_time_global = omp_get_wtime();
  log_timing(stderr, "Total execution", end_time_global - start_time_global);

  return 0;
}

void run_cpu_color_test(PPM_IMG img_in) {
  PPM_IMG img_obuf_hsl, img_obuf_yuv;
  double start_time, end_time;

  start_time   = omp_get_wtime();
  img_obuf_hsl = contrast_enhancement_c_hsl(img_in);
  end_time     = omp_get_wtime();
  log_timing(stderr, "HSL processing", end_time - start_time);
  start_time = omp_get_wtime();
  write_ppm(img_obuf_hsl, "out_hsl.ppm");
  end_time = omp_get_wtime();
  log_timing(stderr, "HSL write", end_time - start_time);

  start_time   = omp_get_wtime();
  img_obuf_yuv = contrast_enhancement_c_yuv(img_in);
  end_time     = omp_get_wtime();
  log_timing(stderr, "YUV processing", end_time - start_time);

  start_time = omp_get_wtime();
  write_ppm(img_obuf_yuv, "out_yuv.ppm");
  end_time = omp_get_wtime();
  log_timing(stderr, "YUV write", end_time - start_time);

  free_ppm(img_obuf_hsl);
  free_ppm(img_obuf_yuv);
}

void run_cpu_gray_test(PGM_IMG img_in) {
  PGM_IMG img_obuf;

  double start_time, end_time;
  start_time = omp_get_wtime();
  img_obuf   = contrast_enhancement_g(img_in);
  end_time   = omp_get_wtime();
  log_timing(stderr, "Grey processing", end_time - start_time);

  start_time = omp_get_wtime();
  write_pgm(img_obuf, "out.pgm");
  end_time = omp_get_wtime();
  log_timing(stderr, "Grey write", end_time - start_time);
  free_pgm(img_obuf);
}

PPM_IMG read_ppm(char const * path) {
  FILE * in_file;
  char sbuf[256];

  char * ibuf;
  PPM_IMG result;
  int v_max, i;
  in_file = fopen(path, "r");
  if (in_file == NULL) {
    printf("Input file not found!\n");
    exit(1);
  }
  /*Skip the magic number*/
  fscanf(in_file, "%s", sbuf);

  // result = malloc(sizeof(PPM_IMG));
  fscanf(in_file, "%d", &result.w);
  fscanf(in_file, "%d", &result.h);
  fscanf(in_file, "%d\n", &v_max);
  printf("Image size: %d x %d\n", result.w, result.h);

  result.img_r = (unsigned char *) malloc(result.w * result.h * sizeof(unsigned char));
  result.img_g = (unsigned char *) malloc(result.w * result.h * sizeof(unsigned char));
  result.img_b = (unsigned char *) malloc(result.w * result.h * sizeof(unsigned char));
  ibuf         = (char *) malloc(3 * result.w * result.h * sizeof(char));

  fread(ibuf, sizeof(unsigned char), 3 * result.w * result.h, in_file);

  for (i = 0; i < result.w * result.h; i++) {
    result.img_r[i] = ibuf[3 * i + 0];
    result.img_g[i] = ibuf[3 * i + 1];
    result.img_b[i] = ibuf[3 * i + 2];
  }

  fclose(in_file);
  free(ibuf);

  return result;
}

void write_ppm(PPM_IMG img, char const * path) {
  FILE * out_file;
  int i;

  char * obuf = (char *) malloc(3 * img.w * img.h * sizeof(char));

  for (i = 0; i < img.w * img.h; i++) {
    obuf[3 * i + 0] = img.img_r[i];
    obuf[3 * i + 1] = img.img_g[i];
    obuf[3 * i + 2] = img.img_b[i];
  }
  out_file = fopen(path, "wb");
  fprintf(out_file, "P6\n");
  fprintf(out_file, "%d %d\n255\n", img.w, img.h);
  fwrite(obuf, sizeof(unsigned char), 3 * img.w * img.h, out_file);

  fclose(out_file);
  free(obuf);
}

void free_ppm(PPM_IMG img) {
  free(img.img_r);
  free(img.img_g);
  free(img.img_b);
}

PGM_IMG read_pgm(char const * path) {
  FILE * in_file;
  char sbuf[256];

  PGM_IMG result;
  int v_max;  //, i;
  in_file = fopen(path, "r");
  if (in_file == NULL) {
    printf("Input file not found!\n");
    exit(1);
  }

  fscanf(in_file, "%s", sbuf); /*Skip the magic number*/
  fscanf(in_file, "%d", &result.w);
  fscanf(in_file, "%d", &result.h);
  fscanf(in_file, "%d\n", &v_max);
  printf("Image size: %d x %d\n", result.w, result.h);

  result.img = (unsigned char *) malloc(result.w * result.h * sizeof(unsigned char));

  fread(result.img, sizeof(unsigned char), result.w * result.h, in_file);
  fclose(in_file);

  return result;
}

void write_pgm(PGM_IMG img, char const * path) {
  FILE * out_file;
  out_file = fopen(path, "wb");
  fprintf(out_file, "P5\n");
  fprintf(out_file, "%d %d\n255\n", img.w, img.h);
  fwrite(img.img, sizeof(unsigned char), img.w * img.h, out_file);
  fclose(out_file);
}

void free_pgm(PGM_IMG img) {
  free(img.img);
}
