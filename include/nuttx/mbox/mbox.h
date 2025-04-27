/****************************************************************************
 * include/nuttx/mbox/mbox.h
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

/* For the purposes of this driver, a hardware mbox is any controller that
 * provide the ability of inter-processor communication in a multi-core
 * system-on-chip architecture, for data communication, event control and
 * resource sharing which are necessary for efficient performance of SoC.
 *
 * The Mbox driver is split into two parts:
 *
 * 1) An "Upper half", generic driver that provides the common Mbox
 *    interface to application level code, and
 * 2) A "lower half", platform-specific driver that implements the
 *    low-level controls to configure and control the mbox controller(s)
 *    for inter-processor communcation.
 *
 * The lower-half mbox ops provided a communication semantics
 * as the following state machine:
 * [s]: state    [a]: action
 *
 * ---------------------------------------------------------
 * |     sender          |     receiver                    |
 * ---------------------------------------------------------
 * |     (1) txready [s] ->                                |
 * |                     \                                 |
 * |                       \                               |
 * |                       -> (2) send [a]                 |
 * |                        \                              |
 * |                          \                            |
 * |                          -> (3) rxavailable [s]       |
 * |                           |                           |
 * |                          -> (4) recv [a]              |
 * |                        /                              |
 * |                     /                                 |
 * |       (5) ack [a] <-                                  |
 * |                 /                                     |
 * |               /                                       |
 * |            <-                                         |
 * |           |                                           |
 * |    (6) txacked [s]                                    |
 * |          |                                            |
 * |    (7) txfinish [a]                                   |
 * |          |                                            |
 * |    (8) txready [s] ->                                 |
 * |         ......                                        |
 * ---------------------------------------------------------
 *
 * Please note that the communication pattern above is only a common
 * description, for example, if platform-specific hardware does not require
 * an ACK, you can only implement the send and recv methods and ignore
 * other ops.
 *
 * The upper-half part provided blocking and non-blocking sending methods
 * and non-blocking receiving methods. It is designed based on the state
 * transfer of lower-half ops as follows.
 *
 * -----------------------------------------------------------------
 * |            TX                    |            RX              |
 * -----------------------------------------------------------------
 * |                / IRQ             |                  / IRQ     |
 * |       txready /                  |     rxavailable /          |
 * |          |    \                  |          |      \          |
 * |          |     \ Polling         |          |       \ Polling |
 * |        send                      |        recv                |
 * |          |                       |          |                 |
 * |          |            / IRQ      |          |                 |
 * |     wait for txacked /           |      acknowledge           |
 * |          |           \           |          |                 |
 * |          |            \ Polling  |          |                 |
 * |        txfinish                  |   next rxavailable         |
 * |          |                       |                            |
 * |          |                       |                            |
 * |    next txready                  |                            |
 * -----------------------------------------------------------------
 */

#ifndef __INCLUDE_NUTTX_MBOX_MBOX_H
#define __INCLUDE_NUTTX_MBOX_MBOX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <nuttx/circbuf.h>
#include <semaphore.h>
#include <nuttx/compiler.h>
#include <nuttx/wdog.h>
#include <nuttx/wqueue.h>
#include <nuttx/spinlock.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MBOX_MAX_MSG_SIZE  (128)

/* Access macros ************************************************************/

/****************************************************************************
 * Name: MBOX_GET_CHAN
 *
 * Description:
 *   Send a 32bits message to remote core from specific channel
 *
 * Input Parameters:
 *   dev   - Device-specific state data
 *   arg   - Argument used to index mbox channel
 *
 * Returned Value:
 *   A pointer of specific channel
 *
 ****************************************************************************/

#define MBOX_GET_CHAN(d,a) ((d)->ops->getchan(d,a))

/****************************************************************************
 * Name: MBOX_SETUP
 *
 * Description:
 *   Setup specific channel before using for transmiting data
 *
 * Input Parameters:
 *   chan  - Mbox specific channel
 *   arg   - Argument
 *
 * Returned Value:
 *   OK unless an error occurs.  Then a negated errno value is returned
 *
 ****************************************************************************/

#define MBOX_SETUP(c,a) ((c)->dev->ops->setup(c,a))

/****************************************************************************
 * Name: MBOX_SHUTDOWN
 *
 * Description:
 *   Shutdown specific channel after transmitting messages
 *
 * Input Parameters:
 *   chan  - Mbox specific channel
 *
 * Returned Value:
 *   OK unless an error occurs.  Then a negated errno value is returned
 *
 ****************************************************************************/

#define MBOX_SHUTDOWN(c) ((c)->dev->ops->shutdown(c))

/****************************************************************************
 * Name: MBOX_SEND
 *
 * Description:
 *   Send a message to remote core from specific channel
 *
 * Input Parameters:
 *   chan  - Mbox specific channel
 *   data  - Data buffer to send
 *   size  - Size of data in bytes
 *
 * Returned Value:
 *   OK unless an error occurs.  Then a negated errno value is returned
 *
 ****************************************************************************/

#define MBOX_SEND(c,d,s) ((c)->dev->ops->send(c,d,s))

/****************************************************************************
 * Name: MBOX_RECV
 *
 * Description:
 *   Recv a message from remote core by specific channel
 *
 * Input Parameters:
 *   chan  - Mbox specific channel
 *   data  - Data buffer to receive message
 *
 * Returned Value:
 *   Size of message data unless an error occurs.  Then a negated errno value
 *   is returned
 *
 ****************************************************************************/

#define MBOX_RECV(c,d) ((c)->dev->ops->recv(c,d))

/****************************************************************************
 * Name: MBOX_ACKNOWLEDGE
 *
 * Description:
 *   Send acknowledge to remote core after receiving the message
 *
 * Input Parameters:
 *   chan  - Mbox specific channel
 *   data  - Data buffer of Acknowledge message
 *   size  - Size of acknowledge message in bytes
 *
 * Returned Value:
 *   OK unless an error occurs.  Then a negated errno value is returned
 *
 ****************************************************************************/

#define MBOX_ACKNOWLEDGE(c,d,s) ((c)->dev->ops->acknowledge(c,d,s))

/****************************************************************************
 * Name: MBOX_TX_FINISH
 *
 * Description:
 *   Finish a transmission process and prepare for the next transmission
 *
 * Input Parameters:
 *   chan  - Mbox specific channel
 *
 * Returned Value:
 *   OK unless an error occurs.  Then a negated errno value is returned
 *
 ****************************************************************************/

#define MBOX_TX_FINISH(c) ((c)->dev->ops->txfinish(c))

/****************************************************************************
 * Name: MBOX_REGISTER_CALLBACK
 *
 * Description:
 *   Attach a callback which will be called when received a message or
 *   finished a transmission on MBOX
 *
 * Input Parameters:
 *   chan     - Mbox specific channel
 *   callback - The function to be called when something has been received
 *   arg      - A caller provided value to return with the callback
 *
 * Returned Value:
 *   OK unless an error occurs.  Then a negated errno value is returned
 *
 ****************************************************************************/

#define MBOX_REGISTER_CALLBACK(c,cb,a) ((c)->dev->ops->setcallback(c,cb,a))

/****************************************************************************
 * Name: MBOX_RX_AVAILABLE
 *
 * Description:
 *   Check if the message is ready to be received on specific channel
 *
 * Input Parameters:
 *   chan     - Mbox specific channel
 *
 * Returned Value:
 *   OK unless an error occurs.  Then a negated errno value is returned
 *
 ****************************************************************************/

#define MBOX_RX_AVAILABLE(c) ((c)->dev->ops->rxavailable(c))

/****************************************************************************
 * Name: MBOX_TX_READY
 *
 * Description:
 *   Check if the channel is ready for next message transfer
 *
 * Input Parameters:
 *   chan     - Mbox specific channel
 *
 * Returned Value:
 *   OK unless an error occurs.  Then a negated errno value is returned
 *
 ****************************************************************************/

#define MBOX_TX_READY(c) ((c)->dev->ops->txready(c))

/****************************************************************************
 * Name: MBOX_TX_ACKED
 *
 * Description:
 *   Check if the channel is acked by remote receiver
 *
 * Input Parameters:
 *   chan     - Mbox specific channel
 *
 * Returned Value:
 *   OK unless an error occurs.  Then a negated errno value is returned
 *
 ****************************************************************************/

#define MBOX_TX_ACKED(c) ((c)->dev->ops->txacked(c))

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Transmission direction of mbox channel */

enum mbox_direction_e
{
  MBOX_RX = 0,
  MBOX_TX = 1
};

/* Forward declaration */

struct mbox_chan_s;

/* Mbox callback type for notify that data is ready */

typedef CODE int (*mbox_callback_t)(int ret,
                                    FAR struct mbox_chan_s *chan,
                                    FAR void *data, size_t size);

#ifdef CONFIG_MBOX_TRACE
/* Statistics of mbox channel */

struct mbox_stats_s
{
  size_t timeout;  /* Count of messages TX timeout */
  size_t sent;     /* Count of message sent */
  size_t received; /* Count of message received */
  size_t acked;    /* Count of message acknowledged */
  size_t buffered; /* Count of message in buffer */
};
#endif

/* Forward declaration */

struct mbox_dev_s;

/* Mbox channel structure */

struct mbox_chan_s
{
  int                   id;          /* ID used to identify a channel */
  FAR struct mbox_dev_s *dev;        /* Associated lower-half device */
  mbox_callback_t       callback;    /* User-defined callback */
  struct circbuf_s      txbuf;       /* Buffer of messages to be sent */
  sem_t                 txsem;       /* TX semaphore */
  struct wdog_s         timer;       /* Timer for detecting TX timeout */
  clock_t               timeout;     /* Max time from sending to be acked */
  bool                  expired;     /* Expiry flag */
#if CONFIG_MBOX_TRACE
  struct mbox_stats_s   stats;       /* Stats of channel */
#endif
  FAR void              *priv;       /* Private data */
};

/* This structure defines a wrapper when use a mbox channel for tx */

struct mbox_sender_s
{
  FAR struct mbox_chan_s *chan;      /* Associated mbox channel */
  bool                   blocking;   /* Blocking or No-blocking */
  uint32_t               timeout;    /* Max blocking ticks */
  FAR void               *priv;      /* Private data */
};

/* This structure defines a wrapper when use a mbox channel for rx */

struct mbox_receiver_s
{
  FAR struct mbox_chan_s *chan;      /* Associated mbox channel */
  FAR void               *priv;      /* Private data */
};

/* This structure is a set of operation functions used to call from the
 * upper-half, generic Mbox driver into lower-half, platfrom-specific logic
 * that supports the low-level functionality.
 */

struct mbox_ops_s
{
  /* Get handle of specific mbox channel by a channel index */

  CODE struct mbox_chan_s *(*getchan)(FAR struct mbox_dev_s *dev,
                                      FAR void *arg);

  /* Request and configure the mbox channel. This method is called before
   * using the mbox channel to transmit data.
   */

  CODE int (*setup)(FAR struct mbox_chan_s *chan, FAR void *arg);

  /* Release the mbox channel. This method is called when the mbox channel
   * is closed. This method reverses the operations the setup method.
   */

  CODE int (*shutdown)(FAR struct mbox_chan_s *chan);

  /* Send message through the mbox channel */

  CODE int (*send)(FAR struct mbox_chan_s *chan,
                   FAR const void *data, size_t size);

  /* Receive message through the mbox channel */

  CODE int (*recv)(FAR struct mbox_chan_s *chan, FAR void *data);

  /* Send acknowledge message from receiver to the sender */

  CODE int (*acknowledge)(FAR struct mbox_chan_s *chan,
                          FAR const void *data, size_t size);

  /* Finish a tranmission process and prepare for the next tranmission */

  CODE int (*txfinish)(FAR struct mbox_chan_s *chan);

  /* Set user-defined callback for the mbox channel, which will be called
   * when TX or RX done and executed in work queue.
   */

  CODE int (*setcallback)(FAR struct mbox_chan_s *chan,
                          mbox_callback_t callback, FAR void *arg);

  /* This method is used to check if the message is ready to be received */

  CODE bool (*rxavailable)(FAR struct mbox_chan_s *chan);

  /* This method is used to check if this channel is ready for next
   * message transfer.
   */

  CODE bool (*txready)(FAR struct mbox_chan_s *chan);

  /* This method is used to check if this channel is acked by
   * remote receiver.
   */

  CODE bool (*txacked)(FAR struct mbox_chan_s *chan);
};

/* This structure is the generic form of state structure used by lower half
 * mbox driver.
 */

struct mbox_dev_s
{
  FAR const struct mbox_ops_s *ops;
  FAR struct mbox_chan_s      *chans;
  size_t                      num_chans;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

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
                                      FAR void *arg);

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
 *   msgbuf  - The internal buffer allocated for the msgbuf of mbox_chan_s
 *   bufsize - The size of the msgbuf in mbox_chan_s
 *
 * Returned Value:
 *   Pointer to mbox channel on success; NULL on failure
 *
 ****************************************************************************/

int mbox_chan_init(FAR struct mbox_chan_s *chan, FAR struct mbox_dev_s *dev,
                   enum mbox_direction_e dir, void *buffer, size_t size);

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
              FAR const void *data, size_t size);

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
                  FAR const void *data, size_t size, clock_t timeout);

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
                   FAR const struct timespec *abstime);

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
                             mbox_callback_t callback, FAR void *arg);

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

void mbox_chan_rx_data(FAR struct mbox_chan_s *chan);

/****************************************************************************
 * Name: mbox_chan_tx_done
 *
 * Description:
 *   This function should be called upon receiving an ACK (when trigger by an
 *   interrupt or when polling detects the txacked status)
 *   This function will first cancel tx timer because ack has reached. Then
 *   lower-half ops->txfinish will be called to finish current transmission,
 *   clean related state and prepare for next transmission.
 *   If there are any remaining message in circbuf, it will be submit to
 *   mailbox controller to send to remote. Finally, the user-defined callback
 *   function will be put into work queue to notify user txdone.
 *
 *
 * Input Parameters:
 *   chan    - A pointer to the mbox channel which received ack
 *
 ****************************************************************************/

void mbox_chan_tx_done(FAR struct mbox_chan_s *chan);

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
                         FAR struct mbox_stats_s *stats);
#endif

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_MBOX_MBOX_H */
