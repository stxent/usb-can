/*
 * core/can_proxy.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "can_proxy.h"
#include "can_proxy_defs.h"
#include "helpers.h"
#include "indicator.h"
#include "system.h"
#include "version.h"
#include <halm/generic/can.h>
#include <halm/generic/serial.h>
#include <halm/generic/work_queue.h>
#include <halm/timer.h>
#include <xcore/interface.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
struct CanProxy
{
  struct Entity base;

  struct Interface *can;
  struct Interface *serial;
  struct Timer *chrono;
  struct ParamStorage *storage;

  ProxyCallback callback;
  void *argument;

  enum CanProxyMode mode;
  bool blocking;

  struct
  {
    size_t position;
    char arena[SERIALIZED_FRAME_MTU + 1];
    bool skip;
  } parser;

  struct
  {
    bool can;
    bool serial;
  } events;
};
/*----------------------------------------------------------------------------*/
static void canToSerial(struct CanProxy *);
static void changePortMode(struct CanProxy *, enum CanProxyMode);
static bool deserializeFrame(struct CanProxy *, const char *, size_t);
static void handleCanEvent(void *);
static void handleSerialEvent(void *);
static void mockEventHandler(void *, enum CanProxyMode, enum CanProxyEvent);
static void onCanEventCallback(void *);
static void onSerialEventCallback(void *);
static size_t processCommand(struct CanProxy *, const char *, size_t,
    char *);
static void readSerialInput(struct CanProxy *);
static bool sendMessageGroup(struct CanProxy *, uint8_t, size_t, size_t);
static bool sendTestMessages(struct CanProxy *, const char *, size_t);
static void serializeFrames(struct CanProxy *,
    const struct CANStandardMessage *, size_t);
static bool setBlockingMode(struct CanProxy *, const char *);
static bool setCustomRate(struct CanProxy *, const char *);
static bool setPredefinedRate(struct CanProxy *, const char *);
/*----------------------------------------------------------------------------*/
static enum Result proxyInit(void *, const void *);
static void proxyDeinit(void *);
/*----------------------------------------------------------------------------*/
const struct EntityClass * const CanProxy = &(const struct EntityClass){
    .size = sizeof(struct CanProxy),
    .init = proxyInit,
    .deinit = proxyDeinit
};
/*----------------------------------------------------------------------------*/
static void canToSerial(struct CanProxy *proxy)
{
  struct CANStandardMessage frames[SERIALIZED_QUEUE_SIZE];
  size_t capacity;

  ifGetParam(proxy->serial, IF_TX_AVAILABLE, &capacity);
  capacity /= SERIALIZED_FRAME_MTU;
  capacity = MIN(capacity, ARRAY_SIZE(frames));

  if (capacity > 0)
  {
    const size_t count = ifRead(proxy->can, frames,
        capacity * sizeof(struct CANStandardMessage));

    if (count > 0)
    {
      serializeFrames(proxy, frames, count / sizeof(struct CANStandardMessage));
    }
  }
}
/*----------------------------------------------------------------------------*/
static void changePortMode(struct CanProxy *proxy, enum CanProxyMode mode)
{
  switch (mode)
  {
    case SLCAN_MODE_ACTIVE:
      ifSetParam(proxy->can, IF_CAN_ACTIVE, NULL);
      break;

    case SLCAN_MODE_LOOPBACK:
      ifSetParam(proxy->can, IF_CAN_LOOPBACK, NULL);
      break;

    default:
      ifSetParam(proxy->can, IF_CAN_LISTENER, NULL);
      break;
  }

  proxy->mode = mode;
  proxy->callback(proxy->argument, mode, SLCAN_EVENT_NONE);
}
/*----------------------------------------------------------------------------*/
static bool deserializeFrame(struct CanProxy *proxy, const char *request,
    size_t length)
{
  struct CANStandardMessage message;

  if (unpackFrame(request, length, &message))
  {
    if (ifWrite(proxy->can, &message, sizeof(message)) == sizeof(message))
    {
      proxy->callback(proxy->argument, proxy->mode, SLCAN_EVENT_TX);
      return true;
    }
    else
    {
      proxy->callback(proxy->argument, proxy->mode, SLCAN_EVENT_CAN_OVERRUN);
      return false;
    }
  }
  else
  {
    proxy->callback(proxy->argument, proxy->mode, SLCAN_EVENT_SERIAL_ERROR);
    return false;
  }
}
/*----------------------------------------------------------------------------*/
static void handleCanEvent(void *argument)
{
  struct CanProxy * const proxy = argument;
  size_t rxAvailable;
  size_t txAvailable;

  proxy->events.can = false;
  ifGetParam(proxy->can, IF_RX_AVAILABLE, &rxAvailable);
  ifGetParam(proxy->can, IF_TX_AVAILABLE, &txAvailable);

  if (rxAvailable > 0)
  {
    canToSerial(proxy);
  }

  if (txAvailable >= SERIALIZED_QUEUE_SIZE)
  {
    ifSetParam(proxy->serial, IF_SERIAL_RTS, &((uint8_t){1}));
    readSerialInput(proxy);
  }
}
/*----------------------------------------------------------------------------*/
static void handleSerialEvent(void *argument)
{
  struct CanProxy * const proxy = argument;
  size_t rxAvailable;
  size_t txAvailable;

  proxy->events.serial = false;
  ifGetParam(proxy->serial, IF_RX_AVAILABLE, &rxAvailable);
  ifGetParam(proxy->serial, IF_TX_AVAILABLE, &txAvailable);

  if (rxAvailable > 0)
  {
    readSerialInput(proxy);
  }

  if (txAvailable > 0)
  {
    canToSerial(proxy);
  }
}
/*----------------------------------------------------------------------------*/
static void mockEventHandler(void *, enum CanProxyMode, enum CanProxyEvent)
{
}
/*----------------------------------------------------------------------------*/
static void onCanEventCallback(void *argument)
{
  struct CanProxy * const proxy = argument;

  if (!proxy->events.can)
  {
    if (wqAdd(WQ_DEFAULT, handleCanEvent, argument) == E_OK)
      proxy->events.can = true;
  }
}
/*----------------------------------------------------------------------------*/
static void onSerialEventCallback(void *argument)
{
  struct CanProxy * const proxy = argument;

  if (!proxy->events.serial)
  {
    if (wqAdd(WQ_DEFAULT, handleSerialEvent, argument) == E_OK)
      proxy->events.serial = true;
  }
}
/*----------------------------------------------------------------------------*/
static size_t processCommand(struct CanProxy *proxy, const char *request,
    size_t length, char *response)
{
  if (!length)
    return 0;

  switch (request[0])
  {
    case 's':
    {
      /* Custom command: set bit rate */
      if (length >= 5 && length <= 7 && setCustomRate(proxy, request))
        strcpy(response, "\r");
      else
        strcpy(response, "\a");
      break;
    }

    case 'S':
    {
      /* Set standard bit rate */
      if (length == 2 && setPredefinedRate(proxy, request))
        strcpy(response, "\r");
      else
        strcpy(response, "\a");
      break;
    }

    case 'r':
    case 'R':
    case 't':
    case 'T':
    {
      if (deserializeFrame(proxy, request, length))
        strcpy(response, "z\r");
      else
        strcpy(response, "\a");
      break;
    }

    case 'C':
    {
      /* Close the channel */
      changePortMode(proxy, SLCAN_MODE_DISABLED);
      strcpy(response, "\r");
      break;
    }

    case 'l':
    {
      /* Custom command: enable loopback mode */
      changePortMode(proxy, SLCAN_MODE_LOOPBACK);
      strcpy(response, "\r");
      break;
    }

    case 'L':
    {
      /* Open the channel in listen only mode */
      changePortMode(proxy, SLCAN_MODE_LISTENER);
      strcpy(response, "\r");
      break;
    }

    case 'O':
    {
      /* Open the channel in normal mode */
      changePortMode(proxy, SLCAN_MODE_ACTIVE);
      strcpy(response, "\r");
      break;
    }

    case 'F':
    {
      /* Read status flags */
      strcpy(response, "z00\r");
      break;
    }

    case 'W':
    {
      /* Filter mode settings */
      strcpy(response, "\r");
      break;
    }

    case 'Z':
    {
      /* Enable or disable timestamping */
      break;
    }

    case 'N':
    {
      /* Read the serial number */
      const uint16_t number = proxy->storage != NULL ?
          (uint16_t)proxy->storage->values.serial : 0xFFFF;

      return packNumber16(response, 'N', number);
    }

    case 'V':
    {
      /* Read hardware version */
      const struct BoardVersion * const ver = getBoardVersion();
      return packNumber16(response, 'V', (ver->hw.major << 8) | ver->hw.minor);
    }

    case 'b':
    {
      /* Custom command: enable or disable blocking mode */
      if (length == 2 && setBlockingMode(proxy, request))
        strcpy(response, "\r");
      else
        strcpy(response, "\a");
      break;
    }

    case 'B':
    {
      /* Custom command: reset to bootloader */
      resetToBootloader();
      strcpy(response, "\a");
      break;
    }

    case 'n':
    {
      /* Custom command: set the serial number */
      if (length == 5 && proxy->storage != NULL
          && !isSerialNumberValid(proxy->storage->values.serial))
      {
        proxy->storage->values.serial = inPlaceHexToBin4(&request[1]);

        if (storageSave(proxy->storage))
          strcpy(response, "\r");
        else
          strcpy(response, "\a");
      }
      else
        strcpy(response, "\a");
      break;
    }

    case 'v':
    {
      /* Custom command: read software version */
      const struct BoardVersion * const ver = getBoardVersion();
      return packNumber16(response, 'v', (ver->sw.major << 8) | ver->sw.minor);
    }

    case 'x':
    {
      /* Custom command: send test sequence */
      if (sendTestMessages(proxy, request, length))
        strcpy(response, "\r");
      else
        strcpy(response, "\a");
      break;
    }

    default:
    {
      strcpy(response, "\a");
      break;
    }
  }

  return strlen(response);
}
/*----------------------------------------------------------------------------*/
static void readSerialInput(struct CanProxy *proxy)
{
  size_t count;

  do
  {
    char buffer[SERIAL_MTU];
    size_t available;
    uint8_t ready;

    ifGetParam(proxy->can, IF_TX_AVAILABLE, &available);
    ready = available >= SERIALIZED_QUEUE_SIZE;
    ifSetParam(proxy->serial, IF_SERIAL_RTS, &ready);

    if (proxy->blocking && available < SERIALIZED_QUEUE_SIZE)
      break;

    count = ifRead(proxy->serial, buffer, sizeof(buffer));

    for (size_t index = 0; index < count; ++index)
    {
      const char c = buffer[index];

      if (c == '\r' || c == '\n') /* New line */
      {
        if (!proxy->parser.skip)
        {
          proxy->parser.arena[proxy->parser.position] = '\0';

          char response[RESPONSE_MTU];
          const size_t responseLength = processCommand(proxy,
              proxy->parser.arena, proxy->parser.position, response);
          const size_t writtenLength = ifWrite(proxy->serial,
              response, responseLength);

          if (writtenLength != responseLength)
          {
            proxy->callback(proxy->argument, proxy->mode,
                SLCAN_EVENT_SERIAL_OVERRUN);
          }
        }

        proxy->parser.position = 0;
        proxy->parser.skip = false;
      }
      else if (proxy->parser.position < sizeof(proxy->parser.arena) - 1)
      {
        proxy->parser.arena[proxy->parser.position++] = c;
      }
      else
      {
        proxy->parser.skip = true;
      }
    }
  }
  while (count > 0);
}
/*----------------------------------------------------------------------------*/
static bool sendMessageGroup(struct CanProxy *proxy, uint8_t flags,
    size_t length, size_t count)
{
  uint32_t rate;

  if (proxy->chrono == NULL)
    return false;
  if (ifGetParam(proxy->can, IF_RATE, &rate) != E_OK || rate == 0)
    return false;

  const uint32_t frameLength = calcFrameLength(flags, length);
  const uint32_t groupTimeout = (frameLength * 1000000 / rate) * count;
  const uint32_t timestamp = timerGetValue(proxy->chrono);

  for (size_t i = 0; i < count; ++i)
  {
    const struct CANStandardMessage message = {
        .timestamp = 0,
        .id = (uint32_t)i,
        .flags = flags,
        .length = length,
        .data = {1, 2, 3, 4, 5, 6, 7, 8}
    };

    while (ifWrite(proxy->can, &message, sizeof(message)) != sizeof(message))
    {
      if (timerGetValue(proxy->chrono) - timestamp > groupTimeout)
        return false;
    }
  }

  return true;
}
/*----------------------------------------------------------------------------*/
static bool sendTestMessages(struct CanProxy *proxy, const char *request,
    size_t length)
{
  struct GroupSettings
  {
    uint8_t flags;
    uint8_t length;
  };

  static const size_t testGroupSize = 1000;
  static const struct GroupSettings testGroupSettings[] = {
      /* Standard frames with empty data field */
      {0, 0},
      /* Standard frames with 64-bit data field */
      {0, 8},
      /* Extended frames with empty data field */
      {CAN_EXT_ID, 0},
      /* Extended frames with 64-bit data field */
      {CAN_EXT_ID, 8},
      /* Standard RTR frames */
      {CAN_RTR, 0},
      /* Extended RTR frames */
      {CAN_RTR | CAN_EXT_ID, 0}
  };

  if (length == 1)
  {
    bool completed = true;

    for (size_t i = 0; completed && (i < ARRAY_SIZE(testGroupSettings)); ++i)
    {
      completed = sendMessageGroup(proxy, testGroupSettings[i].flags,
          testGroupSettings[i].length, testGroupSize);
    }

    return completed;
  }
  else if (length == 2)
  {
    const unsigned int code = hexToBin(request[1]);

    if (code < ARRAY_SIZE(testGroupSettings))
    {
      return sendMessageGroup(proxy, testGroupSettings[code].flags,
          testGroupSettings[code].length, testGroupSize);
    }
    else
      return false;
  }
  else
    return false;
}
/*----------------------------------------------------------------------------*/
static void serializeFrames(struct CanProxy *proxy,
    const struct CANStandardMessage *frames, size_t count)
{
  char response[SERIALIZED_FRAME_MTU * SERIALIZED_QUEUE_SIZE];
  size_t length = 0;

  for (size_t i = 0; i < count; ++i)
    length += packFrame(response + length, frames + i);

  const size_t written = ifWrite(proxy->serial, response, length);

  proxy->callback(proxy->argument, proxy->mode, written == length ?
      SLCAN_EVENT_RX : SLCAN_EVENT_SERIAL_OVERRUN);
}
/*----------------------------------------------------------------------------*/
static bool setBlockingMode(struct CanProxy *proxy, const char *request)
{
  if (request[1] == '0')
  {
    proxy->blocking = false;
    return true;
  }
  else if (request[1] == '1')
  {
    proxy->blocking = true;
    return true;
  }
  else
    return false;
}
/*----------------------------------------------------------------------------*/
static bool setCustomRate(struct CanProxy *proxy, const char *request)
{
  uint32_t rate = 0;

  while (*(++request))
    rate = (rate << 4) | hexToBin(*request);

  return ifSetParam(proxy->can, IF_RATE, &rate) == E_OK;
}
/*----------------------------------------------------------------------------*/
static bool setPredefinedRate(struct CanProxy *proxy, const char *request)
{
  static const uint32_t baudrateMap[] = {
      10000,
      20000,
      50000,
      100000,
      125000,
      250000,
      500000,
      800000,
      1000000
  };

  const unsigned int code = hexToBin(request[1]);

  if (code < ARRAY_SIZE(baudrateMap))
  {
    return ifSetParam(proxy->can, IF_RATE, &baudrateMap[code]) == E_OK;
  }
  else
  {
    return false;
  }
}
/*----------------------------------------------------------------------------*/
static enum Result proxyInit(void *object, const void *configBase)
{
  const struct CanProxyConfig * const config = configBase;
  assert(config != NULL);
  assert(config->can != NULL);
  assert(config->serial != NULL);

  struct CanProxy * const proxy = object;

  proxy->can = config->can;
  proxy->serial = config->serial;
  proxy->chrono = config->chrono;
  proxy->storage = config->storage;

  proxy->callback = config->callback ? config->callback : mockEventHandler;
  proxy->argument = config->argument;
  proxy->mode = SLCAN_MODE_DISABLED;
  proxy->blocking = false;
  proxy->parser.position = 0;
  proxy->parser.skip = false;
  proxy->events.can = false;
  proxy->events.serial = false;

  ifSetCallback(proxy->can, onCanEventCallback, proxy);
  ifSetCallback(proxy->serial, onSerialEventCallback, proxy);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void proxyDeinit(void *object)
{
  struct CanProxy * const proxy = object;

  ifSetCallback(proxy->serial, NULL, NULL);
  ifSetCallback(proxy->can, NULL, NULL);
}
