/****************************************************************************
 * drivers/mbox/mbox.c
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

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdbool.h>
#include <debug.h>

#include <nuttx/mbox/mbox.h>
#include <nuttx/irq.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: mbox_enqueue_data
 *
 * Description:
 *   Enqueue the message data waiting to be sent to circbuf
 *   This is a buffering operation for non-blocking send.
 ****************************************************************************/

static int mbox_enqueue_data(FAR struct mbox_chan_s *chan,
                             FAR const void *data, size_t size);

/****************************************************************************
 * Name: mbox_dequeue_data
 *
 * Description:
 *   Dequeue the message data from circbuf to do actual send
 *   This is a buffering operation for non-blocking send.
 ****************************************************************************/

static int mbox_dequeue_data(FAR struct mbox_chan_s *chan,
                             FAR void *data, FAR size_t *size);

/****************************************************************************
 * Name: submit_msg
 *
 * Description:
 *   Drain a message from circbuf of specific mbox channel, then call
 *   lower half ops->send() to perform actual message tranmission.
 ****************************************************************************/

static int submit_msg(FAR struct mbox_chan_s *chan);

/****************************************************************************
 * Name: tx_expiry
 *
 * Description:
 *   Called when tx timer expires
 ****************************************************************************/

static void tx_expiry(wdparm_t arg);

/****************************************************************************
 * Name: notify_txdone
 *
 * Description:
 *   Notify that the mbox channel has received an ACK.
 ****************************************************************************/

static void notify_txdone(FAR struct mbox_chan_s *chan);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int mbox_enqueue_data(FAR struct mbox_chan_s *chan,
                             FAR const void *data, size_t size)
{
  if (circbuf_space(&chan->txbuf) < size + sizeof(size))
    {
      return -ENOBUFS;
    }

  circbuf_write(&chan->txbuf, (void *)&size, sizeof(size_t));
  circbuf_write(&chan->txbuf, (void *)data, size);

#ifdef CONFIG_MBOX_TRACE
  ++chan->stats.buffered;
#endif
  return 0;
}

static int mbox_dequeue_data(FAR struct mbox_chan_s *chan,
                             FAR void *data, FAR size_t *size)
{
  if (circbuf_is_empty(&chan->txbuf))
    {
      return -ENODATA;
    }

  circbuf_read(&chan->txbuf, (void *)size, sizeof(size_t));
  circbuf_read(&chan->txbuf, (void *)data, *size);

#ifdef CONFIG_MBOX_TRACE
  --chan->stats.buffered;
#endif
  return 0;
}

static int submit_msg(FAR struct mbox_chan_s *chan)
{
  int ret = 0;
  uint8_t data[MBOX_MAX_MSG_SIZE];
  size_t size = 0;

  ret = mbox_dequeue_data(chan, (void *)data, &size);
  if (ret != 0)
    {
      return ret;
    }

  ret = chan->dev->ops->send(chan, data, size);
  if (ret != 0)
    {
      return ret;
    }

#ifdef CONFIG_MBOX_TRACE
  ++chan->stats.sent;
#endif

  wd_start(&chan->timer, chan->timeout, tx_expiry, (wdparm_t)chan);
  return ret;
}

static void notify_txdone(FAR struct mbox_chan_s *chan)
{
  int semcount;
  nxsem_get_value(&chan->txsem, &semcount);
  if (semcount < 1)
    {
      nxsem_post(&chan->txsem);
    }
}

static void tx_expiry(wdparm_t arg)
{
  FAR struct mbox_chan_s *chan = (FAR struct mbox_chan_s *)arg;
  irqstate_t flags;

#ifdef CONFIG_MBOX_TRACE
  chan->stats.timeout++;
#endif

  /* If chan status is not txready, just reset the unfinished transmission */

  if (!chan->dev->ops->txready(chan))
    {
      chan->dev->ops->txfinish(chan);
    }

  /* Submit next message to mbox if ringbuf not empty */

  if (!circbuf_is_empty(&chan->txbuf))
    {
      flags = enter_critical_section();
      submit_msg(chan);
      leave_critical_section(flags);
    }

  /* Notify that it can continue sending */

  chan->expired = true;
  notify_txdone(chan);

  return;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mbox_chan_tx_done
 *
 * Description:
 *   This function should be called upon receiving an ACK (when trigger by an
 *   interrupt or when polling detects the txacked status)
 *   This function will first cancel tx timer because ack has reached. Then
 *   lower-half ops->txfinish() will be called to finish current
 *   transmission, clean related state and prepare for next transmission.
 *   If there are any remaining message in circbuf, it will be submit to
 *   mailbox controller to send to remote. Finally, the user-defined callback
 *   function will be put into work queue to notify user txdone.
 *
 *
 * Input Parameters:
 *   chan    - A pointer to the mbox channel which received ack
 *
 ****************************************************************************/

void mbox_chan_tx_done(FAR struct mbox_chan_s *chan)
{
  irqstate_t flags;

  /* Cancel timer because ACK has reached */

  if (WDOG_ISACTIVE(&chan->timer))
    {
      wd_cancel(&chan->timer);
    }

  /* Finish this transmission, clean related state for next transmit */

  if (chan->dev->ops->txfinish)
    {
      chan->dev->ops->txfinish(chan);
    }

#ifdef CONFIG_MBOX_TRACE
  ++chan->stats.acked;
#endif

  /* notify txdone to blocking API */

  notify_txdone(chan);

  /* Send next data if ringbuf not empty */

  if (!circbuf_is_empty(&chan->txbuf))
    {
      flags = enter_critical_section();
      submit_msg(chan);
      leave_critical_section(flags);
    }

  return;
}

/****************************************************************************
 * Name: mbox_chan_rx_data
 *
 * Description:
 *   This function should be called when RX happened (when triggered by an
 *   interrupt or when polling detects the rxavailable status)
 *   This function will call lower-half ops->recv to receive the data from
 *   remote and send acknowledge to remote sender. Finally, the user-defined
 *   callback function will be put into work queue to push data to
 *   upper-layer.
 *
 * Input Parameters:
 *   chan    - A pointer to the mbox channel which rx happened
 *
 ****************************************************************************/

void mbox_chan_rx_data(FAR struct mbox_chan_s *chan)
{
  /* Receive data from lower-half ops */

  uint8_t data[MBOX_MAX_MSG_SIZE];
  size_t  size;
  size = chan->dev->ops->recv(chan, data);

#ifdef CONFIG_MBOX_TRACE
  ++chan->stats.received;
#endif

  /* Send acknowledge to remote sender */

  if (chan->dev->ops->acknowledge)
    {
      chan->dev->ops->acknowledge(chan, NULL, 0);
    }

#ifdef CONFIG_MBOX_TRACE
  ++chan->stats.acked;
#endif

  /* Post work, call user-defined callback */

  if (chan->callback)
    {
      chan->callback(0, chan, data, size);
    }
}

/****************************************************************************
 * Name: mbox_get_chan
 *
 * Description:
 *   Get a mbox channel hanlde by platform-specific argument.
 *
 * Input Parameters:
 *   dev  - A pointer to mbox dev
 *   arg  - Platform-specific argument to index a channel
 *
 * Returned Value:
 *   Pointer to mbox channel on success; NULL on failure
 *
 ****************************************************************************/

FAR struct mbox_chan_s *mbox_get_chan(FAR struct mbox_dev_s *dev,
                                      FAR void *arg)
{
  if (!dev)
    {
      return NULL;
    }

  return MBOX_GET_CHAN(dev, arg);
}

/****************************************************************************
 * Name: mbox_chan_init
 *
 * Description:
 *   Initialize the members of the specific mbox channel structure.
 *   This function could be called to initialize the base part of
 *   a platform-specific channel in lower-half initialization.
 *
 * Input Parameters:
 *   chan    - A pointer to mbox channel to be initialized
 *   dev     - A pointer to mbox dev
 *   dir     - Mbox xfer direction TX or RX
 *   buffer  - The internal buffer allocated for the txbuf of mbox_chan_s
 *   size    - The size of the txbuf in mbox_chan_s
 *
 * Returned Value:
 *   Pointer to mbox channel on success; NULL on failure
 *
 ****************************************************************************/

int mbox_chan_init(FAR struct mbox_chan_s *chan, FAR struct mbox_dev_s *dev,
                   enum mbox_direction_e dir, void *buffer, size_t size)
{
  int ret = 0;

  if (!chan || !dev)
    {
      return -EINVAL;
    }

  chan->dev = dev;

  if (dir == MBOX_TX)
    {
      ret = nxsem_init(&chan->txsem, 0, 0);
      if (ret != 0)
        {
          goto fail;
        }

      memset(buffer, 0, size);
      ret = circbuf_init(&chan->txbuf, buffer, size);
      if (ret != 0)
        {
          goto fail;
        }
    }

#ifdef CONFIG_MBOX_TRACE
  memset(&chan->stats, 0, sizeof(chan->stats));
#endif
  return 0;

fail:
  return ret;
}

/****************************************************************************
 * Name: mbox_ticksend
 *
 * Description:
 *   Send a message through a specific send channel.
 *
 * Input Parameters:
 *   sender  - A Pointer to a mbox sender
 *   data    - Message Buffer to send
 *   size    - Size of data in bytes
 *   timeout - The ticks to wait until the message is acked
 *
 * Returned Value:
 *   0: success, <0: A negated errno
 *   -EINVAL:    Invalid parameter
 *   -EIO:       IO Error happened when send data
 *   -EAGAIN:    Not txready now, please try again
 *   -ETIMEDOUT: Waiting for ACK timeout
 *
 ****************************************************************************/

int mbox_ticksend(FAR struct mbox_sender_s *sender,
                  FAR const void *data, size_t size, clock_t timeout)
{
  FAR struct mbox_chan_s *chan;
  irqstate_t flags;
  int ret = 0;

  if (!sender || !sender->chan || !data)
    {
      return -EINVAL;
    }

  chan = sender->chan;
  if (timeout == 0)
    {
      /* Try send directly if txready */

      if (chan->dev->ops->txready(chan) && !WDOG_ISACTIVE(&chan->timer))
        {
          ret = chan->dev->ops->send(chan, data, size);
          if (ret != 0)
            {
              return -EIO;
            }

          /* Start timer to monitor ACK timeout */

          wd_start(&chan->timer, timeout, tx_expiry, (wdparm_t)chan);

#ifdef CONFIG_MBOX_TRACE
          ++chan->stats.sent;
#endif
        }
      else
        {
          /* Put data into buffer util triggered to send by txdone or timer */

          flags = enter_critical_section();
          ret = mbox_enqueue_data(chan, data, size);
          leave_critical_section(flags);

          if (ret != 0)
            {
              return -EAGAIN;
            }
        }
    }
  else
    {
      /* Blocking send util timeout, If not ready for xfer, return -EAGAIN */

      if (!chan->dev->ops->txready(chan) || WDOG_ISACTIVE(&chan->timer))
        {
          return -EAGAIN;
        }

      ret = chan->dev->ops->send(chan, data, size);
      if (ret != 0)
        {
          return -EIO;
        }

      wd_start(&chan->timer, timeout, tx_expiry, (wdparm_t)chan);

#ifdef CONFIG_MBOX_TRACE
      ++chan->stats.sent;
#endif

      /* Blocking wait for nxsem_post from tx_expiry or mbox_chan_tx_done */

      ret = nxsem_wait(&chan->txsem);
      if (chan->expired)
        {
          ret = -ETIMEDOUT;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: mbox_send
 *
 * Description:
 *   Send a message through a specific send channel. It will blocking until
 *   the message is acknowledged.
 *
 * Input Parameters:
 *   sender  - A Pointer to a mbox sender
 *   data    - Message to send
 *   size    - Bytes of data to send
 *
 * Returned Value:
 *   0: success, <0: A negated errno
 *   -EINVAL:    Invalid parameter
 *   -EIO:       IO Error happened when send data
 *   -EAGAIN:    Not txready now, please try again
 *   -ETIMEDOUT: Waiting for ACK timeout
 *
 ****************************************************************************/

int mbox_send(FAR struct mbox_sender_s *sender,
              FAR const void *data, size_t size)
{
  return mbox_ticksend(sender, data, size, (clock_t)-1);
}

/****************************************************************************
 * Name: mbox_timedsend
 *
 * Description:
 *   Send a message through a specific send channel.
 *
 * Input Parameters:
 *   sender  - A Pointer to a mbox sender
 *   data    - Message to send
 *   size    - Bytes of data to send
 *   abstime - The absolute time to wait until a timeout is declared
 *
 * Returned Value:
 *   0: success, <0: A negated errno
 *   -EINVAL:    Invalid parameter
 *   -EIO:       IO Error happened when send data
 *   -EAGAIN:    Not txready now, please try again
 *   -ETIMEDOUT: Waiting for ACK timeout
 *
 ****************************************************************************/

int mbox_timedsend(FAR struct mbox_sender_s *sender,
                   FAR void *data, size_t size,
                   FAR const struct timespec *abstime)
{
  if (!abstime ||
      (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000))
    {
      return -EINVAL;
    }

  clock_t ticks = clock_time2ticks(abstime);
  return mbox_ticksend(sender, data, size, ticks);
}

/****************************************************************************
 * Name: mbox_register_rxcallback
 *
 * Description:
 *   Register a user-defined callback function to receiver channel. This
 *   callback function will be called when rxavailable.
 *
 * Input Parameters:
 *   receiver - Mbox receiver which binding to a rx channel
 *   callback - User-defined callback
 *
 * Returned Value:
 *   0: success, <0: A negated errno
 *
 ****************************************************************************/

int mbox_register_rxcallback(FAR struct mbox_receiver_s *receiver,
                             mbox_callback_t callback, FAR void *arg)
{
  if (!receiver || !receiver->chan)
    {
      return -EINVAL;
    }

  return MBOX_REGISTER_CALLBACK(receiver->chan, callback, arg);
}

#ifdef CONFIG_MBOX_TRACE
/****************************************************************************
 * Name: mbox_chan_get_stats
 *
 * Description:
 *   This function is only visible when configure CONFIG_MBOX_TRACE.
 *   This function will retrieves the message sending and receiving
 *   statistics data of specific mbox channel and returns it to
 *   the caller.
 *
 * Input Parameters:
 *   chan    - A pointer to the mbox channel which received ack
 *   stats   - A pointer to the mbox_stats_s passed by caller
 *
 ****************************************************************************/

void mbox_chan_get_stats(FAR struct mbox_chan_s *chan,
                         FAR struct mbox_stats_s *stats)
{
  if (!chan || !stats)
    {
      return;
    }

  *stats = chan->stats;
}
#endif
