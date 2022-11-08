cmake_minimum_required(VERSION 3.16)

project(AIFM)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -D_GNU_SOURCE -std=gnu++2a -fconcepts -Wno-unused-function -Wno-subobject-linkage")

set(SHENANGO_PATH ../shenango)

link_directories(${SHENANGO_PATH}/bindings/cc)

link_libraries(numa rt)

aux_source_directory(src AIFM_SRCS)
message("${AIFM_SRCS}")
string(REPLACE "src/tcp_device_server.cpp" "" AIFM_SRCS ${AIFM_SRCS})
message("${AIFM_SRCS}")

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
#target_link_libraries(aifm
#        numa
#        rt
#        )

