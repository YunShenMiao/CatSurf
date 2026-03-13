# CatSurf Docker Image
# 
# This Dockerfile creates a container with various HTTP testing and network utilities
# including siege, curl, wget, ab (Apache Benchmark), httpie, wrk, and network tools.
#
# Build the image:
#   docker build -t catsurf .
#
# Run the container with a bind mount for live changes:
#   docker run -it -v /path/to/local/dir:/app catsurf
#
# Example with current directory:
#   docker run -it -p 8080:8080 -v "$(pwd)":/app -w /app catsurf
#
# This bind mount (-v) syncs your local directory with /app in the container.
# Any changes made locally will immediately reflect inside the container and vice versa.
#
# For temporary testing with auto-removal:
#   docker run -it --rm -v $(pwd):/app catsurf

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    siege \
    curl \
    wget \
    apache2-utils \
    httpie \
    wrk \
    make \
    cmake \
    gcc \
    g++ \
    net-tools \
    iputils-ping \
    dnsutils \
    && rm -rf /var/lib/apt/lists/*

EXPOSE 8080