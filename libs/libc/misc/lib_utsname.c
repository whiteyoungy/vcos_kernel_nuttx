/****************************************************************************
 * libs/libc/misc/lib_utsname.c
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2015 Stavros Polymenis. All rights reserved.
 * SPDX-FileContributor: Stavros Polymenis <sp@orbitalfox.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/utsname.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/version.h>
#include <unistd.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if defined(__DATE__) && defined(__TIME__) && \
    !defined(CONFIG_LIBC_UNAME_DISABLE_TIMESTAMP)
static const char g_version[] = CONFIG_VERSION_BUILD " " __DATE__ " " __TIME__;
#else
static const char g_version[] = CONFIG_VERSION_BUILD;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: uname
 *
 * Description:
 *   The uname() function will store information identifying the current
 *   system in the structure pointed to by name.
 *
 *   The uname() function uses the utsname structure defined in
 *   <sys/utsname.h>.
 *
 *   The uname() function will return a string naming the current system in
 *   the character array sysname. Similarly, nodename will contain the name
 *   of this node within an implementation-defined communications network.
 *   The arrays release and version will further identify the operating
 *   system. The array machine will contain a name that identifies the
 *   hardware that the system is running on.
 *
 *   The format of each member is implementation-defined.
 *
 * Input Parameters:
 *   name - The user-provided buffer to receive the system information.
 *
 * Returned Value:
 *   Upon successful completion, a non-negative value will be returned.
 *   Otherwise, -1 will be returned and errno set to indicate the error.
 *
 ****************************************************************************/

int uname(FAR struct utsname *name)
{
  int ret = 0;

  /* Copy the strings.  Assure that each is NUL terminated. */

  strlcpy(name->sysname, "NuttX", sizeof(name->sysname));

  /* Get the hostname */

  ret = gethostname(name->nodename, HOST_NAME_MAX);
  name->nodename[HOST_NAME_MAX - 1] = '\0';

  strlcpy(name->release,  CONFIG_VERSION_STRING, sizeof(name->release));

  strlcpy(name->version,  g_version, sizeof(name->version));

  strlcpy(name->machine,  CONFIG_ARCH, sizeof(name->machine));

  return ret;
}
