# Obtain a clean Ubuntu distribution
FROM ubuntu:22.04

# ------------------------------------------------------------------------------- #
# Environment & Basic Setup
# ------------------------------------------------------------------------------- #
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /workspace # Directorio de trabajo para tu proyecto PETGEM
ENV PETSC_DIR=/usr/local/petsc
ENV PETSC_ARCH=arch-linux-c-opt

# ------------------------------------------------------------------------------- #
# Install System & Python Base Dependencies 
# ------------------------------------------------------------------------------- #

RUN apt-get update && apt-get install -y --no-install-recommends \
    # System tools
    git \
    make \
    cmake \
    gcc \
    gfortran \
    g++ \
    lcov \
    flex \
    bison \
    libtool \
    automake \
    autoconf \
    pkg-config \
    wget \
    tar \
    # Python
    python3 \
    python3-pip \
    python3-distutils \
    
    && rm -rf /var/lib/apt/lists/*

# Install Python packages using pip
RUN pip3 install --no-cache-dir \
    meshio \
    h5py \
    numpy \
    matplotlib \
    cython \
    sphinx \
    breathe \
    sphinx_rtd_theme

# ------------------------------------------------------------------------------- #
# Install Extrae and its dependencies 
# ------------------------------------------------------------------------------- #
# Common directory for Extrae and its components
ENV EXTRAE_PREFIX=/usr/local/extrae
RUN mkdir -p ${EXTRAE_PREFIX}/libunwind ${EXTRAE_PREFIX}/papi ${EXTRAE_PREFIX}/libxml2 ${EXTRAE_PREFIX}/binutils

# Install libunwind
RUN cd /tmp && \
    git clone https://github.com/libunwind/libunwind.git && \
    cd libunwind && \
    autoreconf -i && \
    ./configure --prefix=${EXTRAE_PREFIX}/libunwind && \
    make && make install && \
    cd / && rm -rf /tmp/libunwind

# Install PAPI
RUN cd /tmp && \
    git clone https://github.com/icl-utk-edu/papi && \
    cd papi/src && \
    ./configure --prefix=${EXTRAE_PREFIX}/papi CC=gcc F77=gfortran && \
    make && make install && \
    cd / && rm -rf /tmp/papi

# Install binutils (versión específica)
RUN cd /tmp && \
    wget http://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.gz && \
    tar -xf binutils-2.42.tar.gz && \
    cd binutils-2.42 && \
    ./configure --prefix=${EXTRAE_PREFIX}/binutils --enable-shared CFLAGS="-g -O2 -Wno-error" CXXFLAGS="-g -O2 -Wno-error" && \
    make && make install && \
    cp libiberty/pic/libiberty.a ${EXTRAE_PREFIX}/binutils/lib && \
    cd / && rm -rf /tmp/binutils-2.42 /tmp/binutils-2.42.tar.gz

# Install libxml2 (versión específica)
RUN cd /tmp && \
    git clone --branch v2.13.0 https://gitlab.gnome.org/GNOME/libxml2.git && \
    cd libxml2 && \
    ./autogen.sh --prefix=${EXTRAE_PREFIX}/libxml2 --without-python && \
    make && make install && \
    cd / && rm -rf /tmp/libxml2

# Install Extrae (itself)
RUN cd /tmp && \
    git clone https://github.com/bsc-performance-tools/extrae.git && \
    cd extrae && \
    autoreconf -f -i && \
    ./configure --prefix=${EXTRAE_PREFIX} \
                --with-mpi=${PETSC_DIR}/${PETSC_ARCH}/ \
                --with-papi=${EXTRAE_PREFIX}/papi \
                --with-xml-prefix=${EXTRAE_PREFIX}/libxml2 \
                --with-unwind=${EXTRAE_PREFIX}/libunwind \
                --with-binutils=${EXTRAE_PREFIX}/binutils \
                --without-dyninst && \
    make && make install && \
    cd / && rm -rf /tmp/extrae

# ------------------------------------------------------------------------------- #
# Install PETSc
# ------------------------------------------------------------------------------- #
RUN mkdir -p ${PETSC_DIR} && \
    cd ${PETSC_DIR} && \
    git clone -b release https://gitlab.com/petsc/petsc.git . && \
    ./configure --with-cc=gcc --with-cxx=g++ --with-fc=gfortran --download-mpich --download-fblaslapack \
                --with-scalar-type=complex --download-mumps --download-scalapack --download-ptscotch    \
                --download-cmake --with-debugging=1 --download-hdf5 --download-triangle                 \
                --PETSC_ARCH=${PETSC_ARCH} && \
    make all check && \
    # Install petsc4py
    pip3 install --no-cache-dir ./src/binding/petsc4py

# ------------------------------------------------------------------------------- #
# Install Unity (Framework para tests en C)
# ------------------------------------------------------------------------------- #
RUN git clone https://github.com/ThrowTheSwitch/Unity.git /opt/unity

# ------------------------------------------------------------------------------- #
# Alias & Final Configurations
# ------------------------------------------------------------------------------- #
ENV PATH="${PETSC_DIR}/${PETSC_ARCH}/bin:${PATH}"

# Define entry point
WORKDIR /workspace

#ENTRYPOINT ["bash"]