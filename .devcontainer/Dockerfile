FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04

RUN apt-get update && \
    apt-get install -y \
        git coreutils \
        clang-20 clangd-20 clang-format clang-tidy clang-tools \
        libclang-dev libclang1 \
        lld lldb llvm-dev llvm-runtime llvm \
        python3-clang \
        valgrind \
        flex bison

RUN apt-get autoremove -y clang

RUN ln -s /usr/bin/clang-20 /usr/bin/clang && \
    ln -s /usr/bin/clangd-20 /usr/bin/clangd

RUN apt-get install -y \
    python3-venv python3-pip

RUN git config --global --add safe.directory /mnt/clox2

COPY .bash_aliases /root
