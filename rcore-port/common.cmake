cmake_minimum_required(VERSION 3.0)

SET(CMAKE_INSTALL_RPATH "/opt/musl-libc/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

macro(global_set Name Value)
    #  message("set ${Name} to " ${ARGN})
    set(${Name} "${Value}" CACHE STRING "NoDesc" FORCE)
endmacro()

function(JOIN VALUES GLUE OUTPUT)
    string(REGEX REPLACE "([^\\]|^);" "\\1${GLUE}" _TMP_STR "${VALUES}")
    string(REGEX REPLACE "[\\](.)" "\\1" _TMP_STR "${_TMP_STR}") #fixes escaping
    set(${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

macro(add_compile_flags WHERE)
    JOIN("${ARGN}" " " STRING_ARGS)
    if (${WHERE} STREQUAL C)
        global_set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STRING_ARGS}")

    elseif (${WHERE} STREQUAL CXX)
        global_set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STRING_ARGS}")

    elseif (${WHERE} STREQUAL LD)
        global_set(LDFLAGS "${LDFLAGS} ${STRING_ARGS}")
        global_set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${STRING_ARGS}")
        global_set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${STRING_ARGS}")
        global_set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${STRING_ARGS}")

    elseif (${WHERE} STREQUAL BOTH)
        add_compile_flags(C ${ARGN})
        add_compile_flags(CXX ${ARGN})

    elseif (${WHERE} STREQUAL ALL)
        add_compile_flags(C ${ARGN})
        add_compile_flags(CXX ${ARGN})
        add_compile_flags(LD ${ARGN})

    else ()
        message(FATAL_ERROR "add_compile_flags - only support: C, CXX, BOTH, LD, ALL")
    endif ()
endmacro()

if(NOT TOOLCHAIN)   # 如果没有设定工具链
    #find_path(_TOOLCHAIN riscv64-unknown-elf-gcc${EXT}) # 寻找riscv64工具链，将其引入为我们使用的工具链
    global_set(TOOLCHAIN "/usr/bin")
elseif(NOT "${TOOLCHAIN}" MATCHES "/$") # TOKNOW
	global_set(TOOLCHAIN "${TOOLCHAIN}")
endif()

if (NOT TOOLCHAIN)  # 检测工具链是否被正确设置，没有则报错
    message(FATAL_ERROR "TOOLCHAIN must be set, to absolute path of kendryte-toolchain dist/bin folder.")
endif ()

message(STATUS "Using ${TOOLCHAIN} as toolchain")

set(EXT,"")

global_set(CMAKE_C_COMPILER "${TOOLCHAIN}/gcc${EXT}")
global_set(CMAKE_CXX_COMPILER "${TOOLCHAIN}/g++${EXT}")
global_set(CMAKE_LINKER "${TOOLCHAIN}/ld${EXT}")
global_set(CMAKE_AR "${TOOLCHAIN}/ar${EXT}")
global_set(CMAKE_OBJCOPY "${TOOLCHAIN}/objcopy${EXT}")
global_set(CMAKE_SIZE "${TOOLCHAIN}/size${EXT}")
global_set(CMAKE_OBJDUMP "${TOOLCHAIN}/objdump${EXT}")

#global_set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIC -static -Os")
include(${CMAKE_CURRENT_LIST_DIR}/compile-flags.cmake)
