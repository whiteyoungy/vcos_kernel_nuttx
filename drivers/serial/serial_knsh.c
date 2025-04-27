/****************************************************************************
 * drivers/serial/serial_knsh.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <nuttx/serial/serial.h>
#include <nuttx/panic_notifier.h>
#include <nuttx/syslog/syslog.h>
#include <nuttx/kmalloc.h>
#include <nuttx/kernel_shell.h>
#include <nuttx/nuttx.h>

#include <string.h>
#include <debug.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct uart_knsh_s
{
  FAR struct uart_dev_s *dev;
  FAR struct knsh_state_s *state;
  FAR const struct uart_ops_s *ops;
  struct notifier_block nb;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct uart_knsh_s *g_uart_knsh;

/****************************************************************************
 * Private Functions prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void uart_knsh_attach(FAR struct uart_knsh_s *uart_knsh)
{
  FAR uart_dev_t *dev = uart_knsh->dev;
  uart_knsh->ops = dev->ops;
  uart_setup(dev);
  uart_attach(dev);
  uart_disablerxint(dev);
}

/****************************************************************************
 * Name: uart_knsh_panic_callback
 *
 * Description:
 *   This is panic callback for knsh, If a crash occurs,
 *   users can continue to interact with KNSH
 *
 ****************************************************************************/

static int uart_knsh_panic_callback(FAR struct notifier_block *nb,
                                       unsigned long action, FAR void *data)
{
  FAR struct uart_knsh_s *uart_knsh =
    container_of(nb, struct uart_knsh_s, nb);

  if (action != PANIC_KERNEL_FINAL)
    {
      return 0;
    }

#ifndef CONFIG_SERIAL_KNSH_AUTO_ATTACH
  uart_knsh_attach(uart_knsh);
#endif

  _alert("Enter panic shell mode.\n");

  syslog_flush();
  knsh_process(uart_knsh->state);
  return 0;
}

/****************************************************************************
 * Name: uart_knsh_receive
 *
 * Description:
 *   This is knsh receive char function.
 *
 ****************************************************************************/

static ssize_t uart_knsh_receive(FAR void *priv, FAR void *buf,
                                    size_t len)
{
  FAR struct uart_knsh_s *uart_knsh = priv;
  FAR uart_dev_t *dev = uart_knsh->dev;
  FAR char *ptr = buf;
  unsigned int state;
  size_t i = 0;

  while (i < len)
    {
      if (uart_knsh->ops->rxavailable(dev))
        {
          if (uart_knsh->ops->recvbuf)
            {
              i += uart_knsh->ops->recvbuf(dev, ptr + i, len - i);
            }
          else
            {
              ptr[i++] = uart_knsh->ops->receive(dev, &state);
            }
        }
    }

  return len;
}

/****************************************************************************
 * Name: uart_knsh_send
 *
 * Description:
 *   This is knsh send char function.
 *
 ****************************************************************************/

static ssize_t uart_knsh_send(FAR void *priv, FAR void *buf, size_t len)
{
  FAR struct uart_knsh_s *uart_knsh = priv;
  FAR uart_dev_t *dev = uart_knsh->dev;
  size_t i = 0;

  while (i < len)
    {
      if (uart_knsh->ops->sendbuf)
        {
          i += uart_knsh->ops->sendbuf(dev, buf + i, len - i);
        }
      else
        {
          uart_knsh->ops->send(dev, ((FAR char *)buf)[i++]);
        }
    }

  while (!uart_knsh->ops->txempty(dev));

  return len;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: uart_knsh_register
 *
 * Description:
 *   Use the uart device to register knsh.
 *   knsh run with serial interrupt.
 *
 ****************************************************************************/

int uart_knsh_register(FAR uart_dev_t *dev, FAR const char *path)
{
  FAR struct uart_knsh_s *uart_knsh;

  if (g_uart_knsh == NULL)
    {
      uart_knsh = kmm_zalloc(sizeof(struct uart_knsh_s));
      if (uart_knsh == NULL)
        {
          return -ENOMEM;
        }

      g_uart_knsh = uart_knsh;
    }
  else
    {
      uart_knsh = g_uart_knsh;
    }

  if (strcmp(path, CONFIG_SERIAL_KNSH_PATH) != 0)
    {
      return -EINVAL;
    }

  uart_knsh->state = knsh_state_init(uart_knsh_send,
                                       uart_knsh_receive,
                                       uart_knsh);
  if (uart_knsh->state == NULL)
    {
      kmm_free(uart_knsh);
      return -ENOMEM;
    }

  uart_knsh->dev = dev;
  uart_knsh->nb.notifier_call = uart_knsh_panic_callback;
  panic_notifier_chain_register(&uart_knsh->nb);

#ifdef CONFIG_SERIAL_KNSH_AUTO_ATTACH
  uart_knsh_attach(uart_knsh);
  return 0;
#else
  return 1;
#endif
}
