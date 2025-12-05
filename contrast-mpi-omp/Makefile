NUMBER_NODES = 2
NUMBER_PROCESSES = 12

compile:
	cd build && cmake .. && make -j && cd ..

run:
	OMP_NUM_THREADS=$(NUMBER_PROCESSES) srun -p gpus -N $(NUMBER_NODES) -n $(NUMBER_PROCESSES) ./build/contrast >/dev/null

all: compile run
	./test_diff_images.sh 
