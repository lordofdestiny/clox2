FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04

RUN apt-get update && \
    apt-get install -y \
        git \
        clang-format clang-tidy clang-tools clang clangd \
        libc++-dev libc++1 libc++abi-dev libc++abi1 libclang-dev libclang1 liblldb-dev libllvm-ocaml-dev libomp-dev libomp5 \
        lld lldb llvm-dev llvm-runtime llvm \
        python3-clang

RUN git config --global --add safe.directory /mnt/clox2