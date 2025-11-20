NUMBER_NODES = 1
NUMBER_PROCESSES = 12

default:
	srun -p gpus -N $(NUMBER_NODES) -n $(NUMBER_PROCESSES) ./build/contrast >/dev/null && ./test_diff_images.sh 

compile:
	cd build && cmake .. && make -j && cd ..

run:
	srun -p gpus -N $(NUMBER_NODES) -n $(NUMBER_PROCESSES) ./build/contrast >/dev/null

all: compile run
	./test_diff_images.sh 
