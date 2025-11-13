c-build:
	cmake .. -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/mpicxx

compile:
	cd build && make -j && cd ..

run:
	mpirun.mpich -n 3 ./build/contrast
