# syntax=docker/dockerfile:1

FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG CMAKE_VERSION=4.4.2
ARG NINJA_VERSION=1.11.1.1
ARG VCPKG_COMMIT=6f29f12e82a8293156836ad81cc9bf5af41fe836

RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        build-essential \
        ca-certificates \
        curl \
        g++-multilib \
        gcc-multilib \
        git \
        pkg-config \
        python3 \
        python3-pip \
        tar \
        unzip \
        zip \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --no-cache-dir \
        "cmake==${CMAKE_VERSION}" \
        "ninja==${NINJA_VERSION}"

RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && git -C /opt/vcpkg checkout --detach "${VCPKG_COMMIT}" \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/opt/vcpkg
ENV VCPKG_DISABLE_METRICS=1
ENV PATH="${VCPKG_ROOT}:${PATH}"

WORKDIR /workspace

CMD ["bash"]
