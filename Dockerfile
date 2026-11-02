FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        libgtest-dev gcc-avr avr-libc binutils-avr \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["bash"]
