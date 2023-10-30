cmake_minimum_required(VERSION 3.24)
project(clox2 C)

set(CMAKE_C_STANDARD 17)

add_executable(clox2 main.c
        h/common.h
        h/chunk.h
        src/chunk.c
        h/memory.h
        src/memory.c
        src/debug.c
        h/debug.h
        src/value.c
        h/value.h
        src/vm.c
        h/vm.h
        src/compiler.c
        h/compiler.h
        src/scanner.c
        h/scanner.h
        src/object.c
        h/object.h
        src/table.c
        h/table.h)
