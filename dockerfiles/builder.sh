
RELEASE=1.0.0
DOCKERHUB_USER=platinumcd
IMAGE_NAME=analog-master

DOCKERFILES=(Dockerfile.deps Dockerfile.riscv Dockerfile.pyvenv Dockerfile.llvm Dockerfile.openmp Dockerfile.torch Dockerfile.sst)

for file in "${DOCKERFILES[@]}"; do
    stage="${file#Dockerfile.}"
    echo "Processing $stage Stage: $file "
    docker build \
        -f $file \
        -t "$IMAGE_NAME-$stage:$RELEASE" \
        --build-arg BUILD_SRC="/src" \
        --build-arg BUILD_DEST="/opt" \
        --build-arg NUM_THREADS=32 \
        .
done

