##################################################
# First stage, build all the required components #
##################################################
FROM ubuntu:22.04 as builder
ENV DEBIAN_FRONTEND=noninteractive

# Init env
ENV RACS_SRC=/src
ENV RACS_BUILD=/opt
ENV NUM_THREADS=32

# Install Dependencies
RUN apt-get update
RUN apt-get install -y openmpi-bin openmpi-common libopenmpi-dev libtool libtool-bin autoconf python3 python3-pip automake git build-essential

# Install CrossSim
RUN python3 -m pip install pip -U && pip3 install numpy==1.24.3 scipy==1.11.1
RUN git clone -b 'v3.0' --depth 1 https://github.com/sandialabs/cross-sim.git ${RACS_SRC}/cross-sim
RUN cd ${RACS_SRC}/cross-sim && pip3 install .

# SST Core
RUN git clone https://github.com/sstsimulator/sst-core.git ${RACS_SRC}/sst-core
RUN pip install blessings pygments
RUN cd ${RACS_SRC}/sst-core && ./autogen.sh
RUN mkdir ${RACS_SRC}/sst-core/build
WORKDIR ${RACS_SRC}/sst-core/build
RUN ../configure \
        MPICC=/bin/mpicc \
        MPICXX=/bin/mpic++ \
        --prefix=${RACS_BUILD}/sst-core
RUN make -j ${NUM_THREADS} install
WORKDIR /
ENV PATH="${PATH}:${RACS_BUILD}/sst-core/bin"

# SST Elements
RUN git clone -b basic_rocc https://github.com/PlatinumCD/sst-elements.git ${RACS_SRC}/sst-elements
RUN cd ${RACS_SRC}/sst-elements && ./autogen.sh
RUN mkdir ${RACS_SRC}/sst-elements/build
WORKDIR ${RACS_SRC}/sst-elements/build
RUN ../configure \
        MPICC=/bin/mpicc \
        MPICXX=/bin/mpic++ \
        CPPFLAGS=-I/usr/local/lib/python3.10/dist-packages/numpy/core/include \
        --prefix=${RACS_BUILD}/sst-elements
RUN make -j ${NUM_THREADS} install
WORKDIR /

#################################################
# Second stage, only copy what we actually need #
#################################################
FROM ubuntu:22.04
ENV RACS_BUILD=/opt

RUN apt-get update
RUN apt-get install -y python3.10 libpython3.10-dev openmpi-bin openmpi-common libopenmpi-dev

# Add SST Core binaries and SST Elements binaries to $PATH
ENV PATH="${PATH}:${RACS_BUILD}/sst-core/bin

# Copy /opt
COPY --from=builder $RACS_BUILD/ $RACS_BUILD/
