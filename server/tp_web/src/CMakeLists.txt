cmake_minimum_required(VERSION 3.5)

MESSAGE(STATUS "=======================================================")
MESSAGE(STATUS "               tp_web")
MESSAGE(STATUS "=======================================================")
#MESSAGE(STATUS "operation system is ${CMAKE_SYSTEM}")
#MESSAGE(STATUS "current source directory is ${CMAKE_CURRENT_SOURCE_DIR}")

include(../../../CMakeCfg.txt)

set(CMAKE_EXE_LINKER_FLAGS "-export-dynamic")

aux_source_directory(. DIR_SRCS)
aux_source_directory(../../../common/libex/src DIR_SRCS)
aux_source_directory(../../../common/pyshell/src DIR_SRCS)

include_directories(
        ../../../common/libex/include
        ../../../common/pyshell/include
)

if (OS_LINUX)
    include_directories(
            ${TP_EXTERNAL_RELEASE_DIR}/include
            ${TP_EXTERNAL_RELEASE_DIR}/include/python3.7m
    )
    link_directories(${TP_EXTERNAL_RELEASE_DIR}/lib)
    add_executable(tp_web ${DIR_SRCS})
    target_link_libraries(tp_web python3.7m ssl crypto dl pthread rt util)
elseif (OS_MACOS)
    include_directories(
            /usr/local/Cellar/python@3.9/3.9.0_1/Frameworks/Python.framework/Versions/3.9/include/python3.9
    )
    link_directories(
#            /usr/local/Cellar/python@3.9/3.9.0_1/Frameworks/Python.framework/Versions/3.9/lib
            /usr/local/opt/python@3.9/Frameworks/Python.framework/Versions/3.9/lib/python3.9/config-3.9-darwin
    )
    add_executable(tp_web ${DIR_SRCS})
    target_link_libraries(tp_web python3.9 intl dl)
endif ()

