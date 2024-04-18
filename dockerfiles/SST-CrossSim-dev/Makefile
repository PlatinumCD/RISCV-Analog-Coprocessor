# Get the ID of the running docker container
CONTAINER_ID := $(shell docker ps -aqf "name=sst-crosssim-dev")

# Get the ID of the docker Image
IMAGE_ID := $(shell docker images -q sst-crosssim-dev)

.PHONY: all clean

all: 
	@echo "Building SST-CrossSim-dev Image..."
	docker build -t sst-crosssim-dev .

clean:
	@echo "Cleaning SST-CrossSim Container and Image..."
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
	@echo "Running SST-CrossSim Container..."
	docker run -it -v $(shell pwd):/local sst-crosssim-dev
