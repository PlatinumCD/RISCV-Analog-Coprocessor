
RELEASE=1.0.0
DOCKERHUB_USER=platinumcd
IMAGE_NAME=analog-platform-master

docker build \
    -f Dockerfile.master \
    -t "$IMAGE_NAME:$RELEASE" \
    --build-arg BUILD_SRC="/src" \
    --build-arg BUILD_DEST="/opt" \
    --build-arg NUM_THREADS=32 \
    .
