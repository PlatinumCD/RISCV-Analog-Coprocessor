# Release Args
RELEASE?=1.0.0
DOCKERHUB_USER?=platinumcd
IMAGE_NAME?=analog-master

all: 

docker:
	docker build \
		-t "$(DOCKERHUB_USER)/$(IMAGE_NAME):$(RELEASE)" \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg NUM_THREADS=$(NUM_THREADS) \
		.


