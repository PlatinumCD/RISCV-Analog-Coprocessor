FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# Build Args
ARG BUILD_SRC
ARG BUILD_DEST
ARG NUM_THREADS

# CrossSim Args
ARG CROSSSIM_GIT
ARG CROSSSIM_BRANCH

# SST Core Args
ARG SSTCORE_GIT
ARG SSTCORE_BRANCH

# SST Elements Args
ARG SSTELEMENTS_GIT
ARG SSTELEMENTS_BRANCH

# Install Dependencies
RUN apt-get update -y && \
    apt-get install -y openmpi-bin openmpi-common \
    libopenmpi-dev libtool libtool-bin autoconf python3 \
    python3-pip automake git build-essential

# Install CrossSim
RUN python3 -m pip install pip -U && pip3 install numpy==1.24.3 scipy==1.11.1
RUN git clone -b ${CROSSSIM_BRANCH} --depth 1 ${CROSSSIM_GIT} ${BUILD_SRC}/cross-sim
RUN cd ${BUILD_SRC}/cross-sim && pip3 install .

# SST Core
RUN git clone -b ${SSTCORE_BRANCH} ${SSTCORE_GIT} ${BUILD_SRC}/sst-core
RUN pip install blessings pygments
RUN cd ${BUILD_SRC}/sst-core && ./autogen.sh
RUN mkdir ${BUILD_SRC}/sst-core/build
WORKDIR ${BUILD_SRC}/sst-core/build
RUN ../configure \
        MPICC=/bin/mpicc \
        MPICXX=/bin/mpic++ \
        --prefix=${BUILD_DEST}/sst-core
RUN make -j ${NUM_THREADS} install
WORKDIR /
ENV PATH="${PATH}:${BUILD_DEST}/sst-core/bin"

# SST Elements
RUN git clone -b ${SSTELEMENTS_BRANCH} ${SSTELEMENTS_GIT} ${BUILD_SRC}/sst-elements
RUN cd ${BUILD_SRC}/sst-elements && ./autogen.sh
RUN mkdir ${BUILD_SRC}/sst-elements/build
WORKDIR ${BUILD_SRC}/sst-elements/build
RUN ../configure \
        MPICC=/bin/mpicc \
        MPICXX=/bin/mpic++ \
        CPPFLAGS=-I/usr/local/lib/python3.10/dist-packages/numpy/core/include \
        --prefix=${BUILD_DEST}/sst-elements
RUN make -j ${NUM_THREADS} install
WORKDIR /
