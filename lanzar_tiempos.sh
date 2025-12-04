#!/bin/bash

# CARPETAS = ("contrast-mpi" "contrast-mpi-omp" "contrast-omp")
# NODOS = (1 2 3 4)
# NPROC = (1 2 4 8 12 16 20 24 28 32 36 40 44 48)
# CARPETAS=("contrast-mpi")
# NODOS=(3 4)
# NPROC=(1 2 4 8 12 16)

CARPETAS=("contrast-mpi-omp")
NODOS=(3 4)
NPROC=(1 2 4 8 12 16)

NODOS=(3)
NPROC=(6)

# CARPETAS=("contrast-mpi-omp")
# NODOS=(1)
# NPROC=(1 2 4 8 12)



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
  NODES_DISTRIBUTION=""
  
  OUTPUT_FILE="resultados/tiempo_${CARPETA_PROYECTO}_${NODOS}n_${NPROC}p_iter${ITER}.txt"
  
  echo "Lanzando $CARPETA_PROYECTO con $NODOS nodos, $NPROC procesos, iteración $ITER"
  
  # Ejecutar srun con nohup para que persista tras cerrar la terminal
  # Se ejecuta desde el directorio del proyecto para encontrar los archivos de entrada
  (
    cd $CARPETA_PROYECTO
    # Configurar la distribución de nodos y procesos según el proyecto
    if [[ "$CARPETA_PROYECTO" == "contrast-mpi-omp" ]]; then
      NODES_DISTRIBUTION="-N $NODOS -n $NPROC"
    elif [[ "$CARPETA_PROYECTO" == "contrast-mpi" ]]; then
      NODES_DISTRIBUTION="-N $NODOS -n $NPROC"
    fi

    # Configuración para contrast-omp pasarle la varible de entorno OMP_NUM_THREADS
    if [[ "$CARPETA_PROYECTO" == "contrast-omp" ]]; then
      export OMP_NUM_THREADS=$NPROC
    fi

    echo "OMP_NUM_THREADS=$NPROC"
    echo "srun -p gpus $NODES_DISTRIBUTION ./build/contrast >/dev/null 2> $OUTPUT_FILE"
    srun -p gpus $NODES_DISTRIBUTION ./build/contrast >/dev/null 2> $OUTPUT_FILE
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
      if [ "$NPROC" -lt "$NODOS" ]; then
        echo "Skipping ${CARPETA_PROYECTO} with ${NODOS} nodes: ${NPROC} processes less than nodes"
        continue
      fi
      for ((i=1; i<=3; i++))
      do
        lanzar_tiempo $CARPETA_PROYECTO $NODOS $NPROC $i
      done
    done
  done
done

