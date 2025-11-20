NUMBER_NODES = 4
NUMBER_PROCESSES = 4

default:
	srun -p gpus -N $(NUMBER_NODES) -n $(NUMBER_PROCESSES) ./build/contrast >/dev/null && ./test_diff_images.sh 

all:
	cd build/ && cmake .. && make -j && cd .. && srun -p gpus -N $(NUMBER_NODES) -n $(NUMBER_PROCESSES) ./build/contrast >/dev/null && ./test_diff_images.sh 

c-build:
	cmake .. -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/mpicxx

compile:
	cd build && make -j && cd ..

run:
	mpirun.mpich -n $(NUMBER_PROCESSES) ./build/contrast

run-dist:
	srun -p gpus -N $(NUMBER_NODES) -n $(NUMBER_PROCESSES) -mpi=mpi ./build/contrast