# ##############################################################################
# arch/rh850/src/cmake/ghs.cmake
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

# Toolchain

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_ASM_COMPILER ccrh850)
set(CMAKE_C_COMPILER ccrh850)
set(CMAKE_CXX_COMPILER cxrh850)
set(CMAKE_STRIP gstrip)
set(CMAKE_OBJCOPY gsrec)
set(CMAKE_OBJDUMP gdump)
set(CMAKE_LINKER cxrh850)
set(CMAKE_LD cxrh850)
set(CMAKE_AR cxrh850)
set(CMAKE_GMEMFILE gmemfile)
set(CMAKE_NM gnm)
set(CMAKE_RANLIB echo)
set(CMAKE_PREPROCESSOR ccrh850 -E -P)

# override the ARCHIVE command

set(CMAKE_ARCHIVE_COMMAND
    "<CMAKE_AR> <LINK_FLAGS> -archive <OBJECTS> -o <TARGET>")
set(CMAKE_C_ARCHIVE_CREATE ${CMAKE_ARCHIVE_COMMAND})
set(CMAKE_CXX_ARCHIVE_CREATE ${CMAKE_ARCHIVE_COMMAND})
set(CMAKE_ASM_ARCHIVE_CREATE ${CMAKE_ARCHIVE_COMMAND})

set(CMAKE_C_ARCHIVE_APPEND ${CMAKE_ARCHIVE_COMMAND})
set(CMAKE_CXX_ARCHIVE_APPEND ${CMAKE_ARCHIVE_COMMAND})
set(CMAKE_ASM_ARCHIVE_APPEND ${CMAKE_ARCHIVE_COMMAND})

set(NO_LTO "-Onolink")

# Architecture flags

add_link_options(
  -e
  startup_entry
  -map=nuttx.map
  -Manx
  -G
  -dual_debug
  -sda=all
  -large_sda
  -nostdlib
  -Ogeneral)

add_compile_options(
  -needprototype
  -Wundef
  -Onomemclr
  --no_commons
  -G
  -dual_debug
  -noobj
  -pragma_asm_inline
  -inline_prologue
  --long_long
  -sda=all
  -large_sda
  -Ogeneral
  -fsoft
  -gcc
  -gnu99
  -registermode=32)

set(PREPROCESS ${CMAKE_C_COMPILER} ${CMAKE_C_FLAG_ARGS} -E -P)
# override nuttx_find_toolchain_lib

set(NUTTX_FIND_TOOLCHAIN_LIB_DEFINED true)

function(nuttx_find_toolchain_lib)
  execute_process(
    COMMAND bash -c "which ${CMAKE_C_COMPILER} | awk -F '/[^/]*$' '{print $1}'"
    OUTPUT_VARIABLE GHS_ROOT_PATH)
  string(STRIP "${GHS_ROOT_PATH}" GHS_ROOT_PATH)
  if(NOT ARGN)
    nuttx_add_extra_library(${GHS_ROOT_PATH}/lib/rh850/libarch.a)
    nuttx_add_extra_library(${GHS_ROOT_PATH}/lib/rh850/libind_sd.a)
    nuttx_add_extra_library(${GHS_ROOT_PATH}/lib/rh850/libind_fp.a)
    nuttx_add_extra_library(${GHS_ROOT_PATH}/lib/rh850/libwc_s32.a)
    nuttx_add_extra_library(${GHS_ROOT_PATH}/lib/rh850/libmath_fp.a)
  endif()
endfunction()

# override nuttx_generate_preproces_target

set(NUTTX_TOOLCHAIN_PREPROCES_DEFINED true)

function(nuttx_generate_preproces_target)

  # parse arguments into variables

  nuttx_parse_function_args(
    FUNC
    nuttx_generate_preproces_target
    ONE_VALUE
    SOURCE_FILE
    TARGET_FILE
    MULTI_VALUE
    DEPENDS
    REQUIRED
    SOURCE_FILE
    TARGET_FILE
    ARGN
    ${ARGN})

  add_custom_command(
    OUTPUT ${TARGET_FILE}
    COMMAND ${PREPROCESS} -I${CMAKE_BINARY_DIR}/include -filetype.cpp
            ${SOURCE_FILE} -o ${TARGET_FILE}
    DEPENDS ${SOURCE_FILE} ${DEPENDS})

endfunction()

# disable nuttx cmake link group otption
set(DISABLE_LINK_GROUP true)
