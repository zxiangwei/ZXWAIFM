cmake_minimum_required(VERSION 3.16)

project(AIFM)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++2a -fconcepts -Wno-unused-function")

set(SHENANGO_PATH ../shenango)

include_directories(
        ${SHENANGO_PATH}/bindings/cc
        ${SHENANGO_PATH}/ksched
        inc
        DataFrame/AIFM/include
)

link_directories(${SHENANGO_PATH}/bindings/cc)

link_libraries(numa rt)

aux_source_directory(src AIFM_SRCS)

add_library(aifm STATIC ${AIFM_SRCS})
target_include_directories(aifm
        PUBLIC
        ${SHENANGO_PATH}/bindings/cc
        ${SHENANGO_PATH}/ksched
        inc
        DataFrame/AIFM/include
        )
#target_link_libraries(aifm
#        numa
#        rt
#        )

