#!/bin/bash

# CARPETAS=("contrast-mpi-omp" "contrast-mpi" "contrast-omp")
# NODOS=(1 2 3 4)
# NPROC=(1 2 4 8 12 16)

CARPETAS=("contrast-omp")
NODOS=(1)
NPROC=(4)


# Compilar antes de ejecutar para evitar condiciones de carrera
for CARPETA_PROYECTO in "${CARPETAS[@]}"
do
    echo "Compilando $CARPETA_PROYECTO..."
    if [ -d "$CARPETA_PROYECTO/build" ]; then
        rm -rf "$CARPETA_PROYECTO/build"
    fi
    mkdir -p "$CARPETA_PROYECTO/build"
    mkdir -p "$CARPETA_PROYECTO/resultados"
    
    # Subshell para compilar
    (
        cd "$CARPETA_PROYECTO/build"
        cmake ..
        make -j
    )
done

function lanzar_tiempo {
  CARPETA_PROYECTO=$1
  NODOS=$2
  NPROC=$3
  ITER=$4
  
  OUTPUT_FILE="resultados/tiempo_${CARPETA_PROYECTO}_${NODOS}n_${NPROC}p_iter${ITER}.txt"
  
  echo "Lanzando $CARPETA_PROYECTO con $NODOS nodos, $NPROC procesos, iteración $ITER"
  
  # Ejecutar srun con nohup para que persista tras cerrar la terminal
  # Se ejecuta desde el directorio del proyecto para encontrar los archivos de entrada
  (
    cd $CARPETA_PROYECTO
    if [[ "$CARPETA_PROYECTO" == *omp ]]; then
      echo "Usando OMP_NUM_THREADS=$NPROC"
      export OMP_NUM_THREADS=$NPROC
    else
      unset OMP_NUM_THREADS
    fi
    srun -p gpus -N $NODOS -n $NPROC ./build/contrast >/dev/null 2> $OUTPUT_FILE
  )
} 
  

for CARPETA_PROYECTO in "${CARPETAS[@]}"
do
  for NODOS in "${NODOS[@]}"
  do
    for NPROC in "${NPROC[@]}"
    do
      # Compute max allowed processes for this node count (12 per node)
      MAX_PROC=$((12 * NODOS))
      if [ "$NPROC" -gt "$MAX_PROC" ]; then
        echo "Skipping ${CARPETA_PROYECTO} with ${NODOS} nodes: ${NPROC} processes exceeds max ${MAX_PROC}"
        continue
      fi
      for ((i=1; i<=5; i++))
      do
        lanzar_tiempo $CARPETA_PROYECTO $NODOS $NPROC $i
      done
    done
  done
done

