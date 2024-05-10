# Release Args
RELEASE?=1.0.0
PLATFORM?=linux/amd64,linux/arm64
DOCKERHUB_USER?=platinumcd
IMAGE_NAME?=analog-sst-crosssim

# Build Args
BUILD_SRC?=/src
BUILD_DEST?=/opt
NUM_THREADS?=32

# CrossSim Args
CROSSSIM_GIT?=https://github.com/sandialabs/cross-sim.git
CROSSSIM_BRANCH?=v3.0

# SST Core Args
SSTCORE_GIT?=https://github.com/sstsimulator/sst-core.git
SSTCORE_BRANCH?=master

# SST Elements Args
SSTELEMENTS_GIT?=https://github.com/PlatinumCD/sst-elements.git
SSTELEMENTS_BRANCH?=basic_rocc

all: image

image:
	docker build \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg NUM_THREADS=$(NUM_THREADS) \
		--build-arg CROSSSIM_GIT=$(CROSSSIM_GIT) \
		--build-arg CROSSSIM_BRANCH=$(CROSSSIM_BRANCH) \
		--build-arg SSTCORE_GIT=$(SSTCORE_GIT) \
		--build-arg SSTCORE_BRANCH=$(SSTCORE_BRANCH) \
		--build-arg SSTELEMENTS_GIT=$(SSTELEMENTS_GIT) \
		--build-arg SSTELEMENTS_BRANCH=$(SSTELEMENTS_BRANCH) \
		-t "$(DOCKERHUB_USER)/$(IMAGE_NAME):$(RELEASE)" \
		.

push:
	docker build \
		--build-arg BUILD_SRC=$(BUILD_SRC) \
		--build-arg BUILD_DEST=$(BUILD_DEST) \
		--build-arg NUM_THREADS=$(NUM_THREADS) \
		--build-arg CROSSSIM_GIT=$(CROSSSIM_GIT) \
		--build-arg CROSSSIM_BRANCH=$(CROSSSIM_BRANCH) \
		--build-arg SSTCORE_GIT=$(SSTCORE_GIT) \
		--build-arg SSTCORE_BRANCH=$(SSTCORE_BRANCH) \
		--build-arg SSTELEMENTS_GIT=$(SSTELEMENTS_GIT) \
		--build-arg SSTELEMENTS_BRANCH=$(SSTELEMENTS_BRANCH) \
		-t "$(DOCKERHUB_USER)/$(IMAGE_NAME):$(RELEASE)" \
		--platform "$(PLATFORM)" --push \
		.
