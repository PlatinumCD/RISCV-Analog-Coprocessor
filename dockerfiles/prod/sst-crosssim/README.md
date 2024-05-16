# SST CrossSim Production Environment

This directory contains the environment setup for the SST CrossSim production deployment. It includes a `Dockerfile` and a `Makefile` to build and manage the Docker image.

## Makefile Arguments

### Release Args
- `RELEASE` (default: `1.0.0`): The release version of the Docker image.
- `PLATFORM` (default: `linux/amd64,linux/arm64`): The target platform(s) for the Docker image.
- `DOCKERHUB_USER` (default: `platinumcd`): The Docker Hub username to tag the image.
- `SST_CROSSSIM_IMAGE_NAME` (default: `analog-sst-crosssim`): The name of the Docker image.

### Build Args
- `BUILD_SRC` (default: `/src`): Source build directory.
- `BUILD_DEST` (default: `/opt`): Destination build directory.
- `NUM_THREADS` (default: `32`): Number of threads to use for building.

### CrossSim Args
- `CROSSSIM_GIT` (default: `https://github.com/sandialabs/cross-sim.git`): Git repository for CrossSim.
- `CROSSSIM_BRANCH` (default: `v3.0`): Git branch for CrossSim.

### SST Core Args
- `SSTCORE_GIT` (default: `https://github.com/sstsimulator/sst-core.git`): Git repository for SST Core.
- `SSTCORE_BRANCH` (default: `master`): Git branch for SST Core.

### SST Elements Args
- `SSTELEMENTS_GIT` (default: `https://github.com/PlatinumCD/sst-elements.git`): Git repository for SST Elements.
- `SSTELEMENTS_BRANCH` (default: `basic_rocc`): Git branch for SST Elements.

## Makefile Targets

### `all`
This is the default target that builds the Docker image.

Usage:
```bash
make
```

### `image`
Builds the Docker image.

Usage:
```bash
make image
```

### `push`
Builds and pushes the Docker image to the registry for the specified platforms.

Usage:
```bash
make push
```

## Example Commands

Build the image:
```bash
make image
```

Build and push the image to Docker Hub:
```bash
make push
```