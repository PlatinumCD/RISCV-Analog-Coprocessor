# Get the ID of the running docker container
CONTAINER_ID := $(shell docker ps -aqf "name=llvm-riscv-toolchain-dev")

# Get the ID of the docker Image
IMAGE_ID := $(shell docker images -q llvm-riscv-toolchain-dev)

.PHONY: all clean

all: 
	@echo "Building LLVM RISC-V Toolchain Dev Image..."
	docker build -t llvm-riscv-toolchain-dev .

clean:
	@echo "Cleaning LLVM RISC-V Toolchain Dev Container and Image..."
	# Check if Docker container is running, if yes then stop and remove it
	if [ "$(CONTAINER_ID)" ]; then \
		docker stop $(CONTAINER_ID); \
		docker rm $(CONTAINER_ID); \
	fi
	# Check if Docker image exists, If yes then remove it
	if [ "$(IMAGE_ID)" ]; then \
		docker rmi $(IMAGE_ID); \
	fi

run:
	@echo "Running LLVM RISC-V Toolchain Dev Container..."
	docker run -it -v $(shell pwd):/local llvm-riscv-toolchain-dev

test:
	@echo "Running LLVM RISC-V Toolchain Dev Tests..."
	./c_compiler tests/hello_world.c
	./c_compiler tests/mvm_intrinsics.c
	./cpp_compiler tests/hello_world.cpp
	./cpp_compiler tests/mvm_intrinsics.cpp


test-clean:
	@echo "Cleaning LLVM RISC-V Toolchain Dev Test Directory..."
	rm ./tests/*.out
