FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04

COPY .bash_aliases /root

RUN apt-get update && \
    apt-get install -y \
        git coreutils \
        gcovr \
        clang clang-format clang-tidy clang-tools clangd \
        libclang-dev libclang1 \
        lld lldb llvm-dev llvm-runtime llvm \
        python3-clang \
        valgrind \
        flex bison

RUN apt-get install -y \
    python3-venv python3-pip

RUN git config --global --add safe.directory /mnt/clox2
