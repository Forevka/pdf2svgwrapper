FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libcairo2-dev \
    libpoppler-glib-dev \
    libglib2.0-dev \
    curl zip unzip tar

WORKDIR /src
