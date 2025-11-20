default:
	srun -p gpus -N 1 -n 12 ./build/contrast >/dev/null && ./test_diff_images.sh 

compile:
	cd build && cmake .. && make -j && cd ..

run:
	srun -p gpus -N 1 -n 12 ./build/contrast >/dev/null

all: compile run
	./test_diff_images.sh 
