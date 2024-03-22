# Get the ID of the running docker container
CONTAINER_ID := $(shell docker ps -aqf "name=llvm-riscv-toolchain")

# Get the ID of the docker Image
IMAGE_ID := $(shell docker images -q llvm-riscv-toolchain)

.PHONY: all clean

all: 
	@echo "Building LLVM RISC-V Toolchain..."
	docker build -t llvm-riscv-toolchain .

clean:
	@echo "Cleaning Docker Container and Image..."
	# Check if docker container is running, if yes then stop and remove it
	ifneq ($(CONTAINER_ID),)
		docker stop $(CONTAINER_ID)
		docker rm $(CONTAINER_ID)
	endif
	# Check if docker image exists, If yes then remove it
	ifneq ($(IMAGE_ID),)
		docker rmi $(IMAGE_ID)
	endif
