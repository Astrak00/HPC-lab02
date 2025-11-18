c-build:
	cmake .. -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/mpicxx

compile:
	cd build && make -j && cd ..

run:
	srun mpirun -np 4 ./build/contrast

run-single:
	srun -p gpus ./build/contrast

test:
	./tests_diff_images.sh