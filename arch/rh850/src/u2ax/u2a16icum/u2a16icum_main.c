/****************************************************************************
 * arch/rh850/src/u2ax/u2a16icum/u2a16icum_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <rh850_internal.h>
#include <nuttx/init.h>
#include <nuttx/board.h>
#include "chip.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

void main(void)
{
  if (u2a16icum_getcoreid() == 3)
    {
      u2a16icum_earlypllinit();
      u2a16icum_cmdreg_init();
      nx_start();
    }
  while (1);
}
