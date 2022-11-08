cmake_minimum_required(VERSION 3.16)

project(AIFM)

set(SHENANGO_PATH ../shenango)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -D_GNU_SOURCE -std=gnu++2a -fconcepts -Wno-unused-function -Wno-subobject-linkage -march=native")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${SHENANGO_PATH}/base/base.ld")

set(RUNTIME_LIBS
        runtime
        net
        base
        pthread
        mlx5
        ibverbs
        nl-3
        nl-route-3
        )

link_directories(
        ${SHENANGO_PATH}
        ${SHENANGO_PATH}/bindings/cc
        $(SHENANGO_PATH)/rdma-core/build/lib/statics/
)

aux_source_directory(src AIFM_SRCS)
list(REMOVE_ITEM AIFM_SRCS "src/tcp_device_server.cpp")

add_library(aifm STATIC ${AIFM_SRCS})
target_include_directories(aifm
        PUBLIC
        ${SHENANGO_PATH}/inc
        $(SHENANGO_PATH)/spdk/include
        $(SHENANGO_PATH)/rdma-core/build/include
        ${SHENANGO_PATH}/bindings/cc
        ${SHENANGO_PATH}/ksched
        inc
        DataFrame/AIFM/include
        )
target_compile_definitions(aifm
        PUBLIC
        MLX_CX4
        MLX5
        DIRECTPATH
        )

add_executable(test_pointer_noswap test/test_pointer_noswap.cpp)
target_link_libraries(test_pointer_noswap
        aifm
        numa
        rt++
        ${RUNTIME_LIBS}
        )

