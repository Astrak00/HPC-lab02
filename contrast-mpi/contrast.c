#include "hist-equ.h"

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEASURE_TIME(variable, function)                          \
  do {                                                            \
    double start_time_macro = MPI_Wtime();                        \
    function;                                                     \
    double end_time_macro  = MPI_Wtime();                         \
    variable              += (end_time_macro - start_time_macro); \
  } while (0)

void run_cpu_color_test(PPM_IMG img_in);
void run_cpu_gray_test(PGM_IMG img_in);

static inline void log_timing(FILE * stream, char const * label, double seconds) {
  fprintf(stream, "%.3f (ms) \t taken for %s\n", seconds * 1000.0, label);
}

double IO_time = 0.0, processing_time = 0.0, comms_time = 0.0;

int main(int argc, char * argv[]) {
  PGM_IMG img_ibuf_g_complete;
  PPM_IMG img_ibuf_c_complete;
  PGM_IMG img_ibuf_g;
  PPM_IMG img_ibuf_c;

  int numprocs, rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Para una parte 2:
  // Hacer chunks,
  // Hacer que cada uno escriba su chunk - SI, hacer que lea, si es 256 lineas, y 4 procesos, 65 + 3
  // cada uno. Puede que no haya que hacer un scatter. Si hacemos lecturas distribuidas mejor.
  // printf("Number of processes: %d - Rank: %d\n", numprocs, rank);
  double const start_time_global = MPI_Wtime();

  int total_w_g, total_h_g, total_w_c, total_h_c;
  if (rank == 0) {
    MEASURE_TIME(IO_time, img_ibuf_g_complete = read_pgm("in.pgm"));
    MEASURE_TIME(IO_time, img_ibuf_c_complete = read_ppm("in.ppm"));
    total_w_g = img_ibuf_g_complete.w;
    total_h_g = img_ibuf_g_complete.h;
    total_w_c = img_ibuf_c_complete.w;
    total_h_c = img_ibuf_c_complete.h;
  }

  int dimensions[4] = {total_w_g, total_h_g, total_w_c, total_h_c};
  MEASURE_TIME(comms_time, MPI_Bcast(dimensions, 4, MPI_INT, 0, MPI_COMM_WORLD));

  img_ibuf_g.w              = dimensions[0];
  int const rows_per_proc_g = dimensions[1] / numprocs;
  int const remainder_g     = dimensions[1] % numprocs;
  img_ibuf_g.h              = rows_per_proc_g + (rank < remainder_g ? 1 : 0);

  img_ibuf_c.w              = dimensions[2];
  int const rows_per_proc_c = dimensions[3] / numprocs;
  int const remainder_c     = dimensions[3] % numprocs;
  img_ibuf_c.h              = rows_per_proc_c + (rank < remainder_c ? 1 : 0);

  // Cambiar a vector de img_g para hacer un solo malloc o un * muy grande.
  int const grey_dim = img_ibuf_g.w * img_ibuf_g.h;
  img_ibuf_g.img     = (unsigned char *) malloc(grey_dim * sizeof(unsigned char));

  int const color_dim = img_ibuf_c.w * img_ibuf_c.h;
  img_ibuf_c.img_r    = (unsigned char *) malloc(color_dim * 3 * sizeof(unsigned char));
  img_ibuf_c.img_g    = img_ibuf_c.img_r + color_dim;
  img_ibuf_c.img_b    = img_ibuf_c.img_r + 2 * color_dim;

  // Grey
  int * sendcounts_g = NULL;  // This array holds the number of elements to send to each process
  int * sendcounts_c = NULL;  // This array holds the number of elements to send to each process
  int * displs_g     = NULL;  // This array holds the displacements for each process
  int * displs_c     = NULL;  // This array holds the displacements for each process
  if (rank == 0) {
    sendcounts_g = (int *) malloc(numprocs * 4 * sizeof(int));
    sendcounts_c = sendcounts_g + numprocs;
    displs_g     = sendcounts_g + (numprocs * 2);
    displs_c     = sendcounts_g + (numprocs * 3);
    int offset_g = 0;
    int offset_c = 0;
    for (int i = 0; i < numprocs; i++) {
      int rows_g       = rows_per_proc_g + (i < remainder_g ? 1 : 0);
      sendcounts_g[i]  = rows_g * total_w_g;  // Number of elements per process (rows * width).
      displs_g[i]      = offset_g;            // Displacement is the offset in the complete image
      offset_g        += sendcounts_g[i];     // Update offset for next process

      // Color
      int rows_c       = rows_per_proc_c + (i < remainder_c ? 1 : 0);
      sendcounts_c[i]  = rows_c * total_w_c;  // Number of elements per process (rows * width).
      displs_c[i]      = offset_c;            // Displacement is the offset in the complete image
      offset_c        += sendcounts_c[i];     // Update offset for next process
    }
  }

  // We use Scatterv to distribute different amounts of data to each process, in case the image
  // can't be split evenly into the processes.
  // sendcounts and displs are only valid on rank 0, and NULL on other ranks.
  // they show how many elements to send to each process, and the displacement in the source array.

  MEASURE_TIME(comms_time, MPI_Scatterv(rank == 0 ? img_ibuf_g_complete.img : NULL, sendcounts_g,
                                        displs_g, MPI_UNSIGNED_CHAR, img_ibuf_g.img, grey_dim,
                                        MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD));

  // Color
  MEASURE_TIME(comms_time, MPI_Scatterv(rank == 0 ? img_ibuf_c_complete.img_r : NULL, sendcounts_c,
                                        displs_c, MPI_UNSIGNED_CHAR, img_ibuf_c.img_r, color_dim,
                                        MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD));

  MEASURE_TIME(comms_time, MPI_Scatterv(rank == 0 ? img_ibuf_c_complete.img_g : NULL, sendcounts_c,
                                        displs_c, MPI_UNSIGNED_CHAR, img_ibuf_c.img_g, color_dim,
                                        MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD));

  MEASURE_TIME(comms_time, MPI_Scatterv(rank == 0 ? img_ibuf_c_complete.img_b : NULL, sendcounts_c,
                                        displs_c, MPI_UNSIGNED_CHAR, img_ibuf_c.img_b, color_dim,
                                        MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD));

  if (rank == 0) { free(sendcounts_g); }

  // printf("Running contrast enhancement for gray-scale images.\n");
  run_cpu_gray_test(img_ibuf_g);
  // printf("Running contrast enhancement for color images.\n");
  run_cpu_color_test(img_ibuf_c);

  MPI_Finalize();

  // Free chunk buffers
  free(img_ibuf_g.img);
  free(img_ibuf_c.img_r);

  // Free complete images only on rank 0
  if (rank == 0) {
    double const end_time_global = MPI_Wtime();
    free_pgm(img_ibuf_g_complete);
    free_ppm(img_ibuf_c_complete);
    log_timing(stderr, "IO time", IO_time);
    log_timing(stderr, "Processing time", processing_time);
    log_timing(stderr, "Comms time", comms_time);
    log_timing(stderr, "Total execution", end_time_global - start_time_global);
  }
  return 0;
}

void run_cpu_color_test(PPM_IMG img_in) {
  PPM_IMG img_obuf_hsl, img_obuf_yuv;
  PPM_IMG img_obuf_hsl_complete, img_obuf_yuv_complete;

  int rank, numprocs, total_h;
  double start_time, end_time;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

  // printf("Starting CPU processing hsl ...\n");

  MEASURE_TIME(processing_time, img_obuf_hsl = contrast_enhancement_c_hsl(img_in));

  MEASURE_TIME(comms_time, MPI_Allreduce(&img_in.h, &total_h, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD));

  int * recvcounts = NULL;
  int * displs     = NULL;

  if (rank == 0) {
    img_obuf_hsl_complete.w = img_in.w;
    img_obuf_hsl_complete.h = total_h;
    int img_complete_dim    = img_obuf_hsl_complete.w * img_obuf_hsl_complete.h;

    img_obuf_hsl_complete.img_r =
        (unsigned char *) malloc(img_complete_dim * 3 * sizeof(unsigned char));
    img_obuf_hsl_complete.img_g = img_obuf_hsl_complete.img_r + img_complete_dim;
    img_obuf_hsl_complete.img_b = img_obuf_hsl_complete.img_r + 2 * img_complete_dim;

    recvcounts        = (int *) malloc(numprocs * sizeof(int));
    displs            = (int *) malloc(numprocs * sizeof(int));
    int rows_per_proc = total_h / numprocs;
    int remainder     = total_h % numprocs;
    int offset        = 0;
    for (int i = 0; i < numprocs; i++) {
      int r          = rows_per_proc + (i < remainder ? 1 : 0);
      recvcounts[i]  = r * img_in.w;
      displs[i]      = offset;
      offset        += recvcounts[i];
    }
  }

  MEASURE_TIME(comms_time, MPI_Gatherv(img_obuf_hsl.img_r, img_obuf_hsl.w * img_obuf_hsl.h,
                                       MPI_UNSIGNED_CHAR, img_obuf_hsl_complete.img_r, recvcounts,
                                       displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD));
  MEASURE_TIME(comms_time, MPI_Gatherv(img_obuf_hsl.img_g, img_obuf_hsl.w * img_obuf_hsl.h,
                                       MPI_UNSIGNED_CHAR, img_obuf_hsl_complete.img_g, recvcounts,
                                       displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD));
  MEASURE_TIME(comms_time, MPI_Gatherv(img_obuf_hsl.img_b, img_obuf_hsl.w * img_obuf_hsl.h,
                                       MPI_UNSIGNED_CHAR, img_obuf_hsl_complete.img_b, recvcounts,
                                       displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD));

  if (rank == 0) {
    MEASURE_TIME(IO_time, write_ppm(img_obuf_hsl_complete, "out_hsl.ppm"));
    free(img_obuf_hsl_complete.img_r);
  }
  free_ppm(img_obuf_hsl);

  // printf("Starting CPU processing yuv ...\n");
  MEASURE_TIME(processing_time, img_obuf_yuv = contrast_enhancement_c_yuv(img_in));

  if (rank == 0) {
    img_obuf_yuv_complete.w    = img_in.w;
    img_obuf_yuv_complete.h    = total_h;
    int const img_complete_dim = img_obuf_yuv_complete.w * img_obuf_yuv_complete.h;
    img_obuf_yuv_complete.img_r =
        (unsigned char *) malloc(img_complete_dim * 3 * sizeof(unsigned char));
    img_obuf_yuv_complete.img_g = img_obuf_yuv_complete.img_r + img_complete_dim;
    img_obuf_yuv_complete.img_b = img_obuf_yuv_complete.img_r + img_complete_dim * 2;
  }

  MEASURE_TIME(comms_time, MPI_Gatherv(img_obuf_yuv.img_r, img_obuf_yuv.w * img_obuf_yuv.h,
                                       MPI_UNSIGNED_CHAR, img_obuf_yuv_complete.img_r, recvcounts,
                                       displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD));
  MEASURE_TIME(comms_time, MPI_Gatherv(img_obuf_yuv.img_g, img_obuf_yuv.w * img_obuf_yuv.h,
                                       MPI_UNSIGNED_CHAR, img_obuf_yuv_complete.img_g, recvcounts,
                                       displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD));
  MEASURE_TIME(comms_time, MPI_Gatherv(img_obuf_yuv.img_b, img_obuf_yuv.w * img_obuf_yuv.h,
                                       MPI_UNSIGNED_CHAR, img_obuf_yuv_complete.img_b, recvcounts,
                                       displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD));

  if (rank == 0) {
    MEASURE_TIME(IO_time, write_ppm(img_obuf_yuv_complete, "out_yuv.ppm"));
    free(img_obuf_yuv_complete.img_r);
    free(recvcounts);
    free(displs);
  }
  free_ppm(img_obuf_yuv);
}

void run_cpu_gray_test(PGM_IMG img_in) {
  int rank, numprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

  PGM_IMG img_obuf;
  PGM_IMG img_obuf_complete;

  // printf("Starting CPU processing, on rank %d of %d\n", rank, numprocs);

  MEASURE_TIME(processing_time, img_obuf = contrast_enhancement_g(img_in));
  int total_h = 0;
  MEASURE_TIME(comms_time, MPI_Allreduce(&img_in.h, &total_h, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD));

  if (rank == 0) {
    // printf("Finished CPU processing gray.\n");
    img_obuf_complete.w = img_in.w;
    img_obuf_complete.h = total_h;
    img_obuf_complete.img =
        (unsigned char *) malloc(img_obuf_complete.w * img_obuf_complete.h * sizeof(unsigned char));
  }

  int * recvcounts = NULL;
  int * displs     = NULL;
  // Esto hace lo mismo que el scatterv pero al reves
  if (rank == 0) {
    recvcounts        = (int *) malloc(numprocs * sizeof(int));
    displs            = (int *) malloc(numprocs * sizeof(int));
    int rows_per_proc = total_h / numprocs;
    int remainder     = total_h % numprocs;
    int offset        = 0;
    for (int i = 0; i < numprocs; i++) {
      int r          = rows_per_proc + (i < remainder ? 1 : 0);
      recvcounts[i]  = r * img_in.w;
      displs[i]      = offset;
      offset        += recvcounts[i];
    }
  }

  MEASURE_TIME(comms_time, MPI_Gatherv(img_obuf.img, img_obuf.w * img_obuf.h, MPI_UNSIGNED_CHAR,
                                       img_obuf_complete.img, recvcounts, displs, MPI_UNSIGNED_CHAR,
                                       0, MPI_COMM_WORLD));
  if (rank == 0) {
    MEASURE_TIME(IO_time, write_pgm(img_obuf_complete, "out.pgm"));
    free_pgm(img_obuf_complete);
    free(recvcounts);
    free(displs);
  }

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
