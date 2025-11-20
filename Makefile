default:
	cd build/ && cmake .. && make -j && cd .. && srun -p gpus -N 1 -n 4 ./build/contrast >/dev/null && ./test_diff_images.sh 

c-build:
	cmake .. -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/mpicxx

compile:
	cd build && make -j && cd ..

run:
	mpirun.mpich -n 4 ./build/contrast

run-dist:
	srun -p gpus -N 4 -n 48 -mpi=mpi ./build/contrast
