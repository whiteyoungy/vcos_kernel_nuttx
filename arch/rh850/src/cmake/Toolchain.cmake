# ##############################################################################
# arch/tricore/src/cmake/Toolchain.cmake
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

if(CONFIG_ARCH_RH850G3K)
  set(ARCH_SUBDIR rh850g3k)
  include(${CMAKE_CURRENT_LIST_DIR}/rh850g3k.cmake)
endif()

if(CONFIG_ARCH_TOOLCHAIN_GHS)
  include(${CMAKE_CURRENT_LIST_DIR}/ghs.cmake)
endif()
