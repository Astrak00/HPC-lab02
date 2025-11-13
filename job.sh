#!/bin/bash
#SBATCH -p gpus           # Partition
#SBATCH -N 3              # Number of nodes
#SBATCH -n 3              # Number of tasks
#SBATCH -t 00:01:00       # Time limit
#SBATCH -J mpi_contrast   # Job name

mpirun.mpich -n 3 /home/alumnos/a0472175/HPC-Lab02/build/contrast
# srun -n 3 /home/alumnos/a0472175/HPC-Lab02/build/contrast
