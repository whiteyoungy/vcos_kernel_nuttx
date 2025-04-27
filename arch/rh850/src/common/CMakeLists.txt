# ##############################################################################
# arch/rh850/src/common/CMakeLists.txt
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

set(SRCS
    rh850_allocateheap.c
    rh850_checkstack.c
    rh850_createstack.c
    rh850_doirq.c
    rh850_exit.c
    rh850_getintstack.c
    rh850_idle.c
    rh850_initialize.c
    rh850_initialstate.c
    rh850_irq.c
    rh850_mdelay.c
    rh850_nputs.c
    rh850_registerdump.c
    rh850_releasestack.c
    rh850_saveusercontext.S
    rh850_schedulesigaction.c
    rh850_sigdeliver.c
    rh850_stackframe.c
    rh850_stub.c
    rh850_svcall.c
    rh850_switchcontext.c
    rh850_tcbinfo.c
    rh850_testset.c
    rh850_usestack.c)

target_sources(arch PRIVATE ${SRCS})
