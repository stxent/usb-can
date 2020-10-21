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

  struct
  {
    size_t position;
    uint8_t arena[SERIALIZED_FRAME_MTU + 1];
    bool skip;
  } parser;

  bool canEvent;
  bool serialEvent;
};
/*----------------------------------------------------------------------------*/
static void changePortMode(struct CanProxy *, enum CanProxyMode);
static bool deserializeFrame(struct CanProxy *, const uint8_t *, size_t);
static void serializeFrames(struct CanProxy *, const struct CanMessage *,
    size_t);
static void handleCanEvent(void *);
static void handleSerialEvent(void *);
static void mockEventHandler(void *, enum CanProxyMode, enum CanProxyEvent);
static size_t processMessage(struct CanProxy *, const uint8_t *, size_t,
    char *);
static bool sendMessageGroup(struct CanProxy *, uint8_t, size_t, size_t);
static bool sendTestMessages(struct CanProxy *, const uint8_t *, size_t);
static bool setCustomRate(struct CanProxy *, const uint8_t *);
static bool setPredefinedRate(struct CanProxy *, const uint8_t *);
static void onCanEventCallback(void *);
static void onSerialEventCallback(void *);
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
static void changePortMode(struct CanProxy *proxy, enum CanProxyMode mode)
{
  switch (mode)
  {
    case SLCAN_MODE_ACTIVE:
      ifSetParam(proxy->can, IF_CAN_ACTIVE, 0);
      break;

    case SLCAN_MODE_LOOPBACK:
      ifSetParam(proxy->can, IF_CAN_LOOPBACK, 0);
      break;

    default:
      ifSetParam(proxy->can, IF_CAN_LISTENER, 0);
      break;
  }

  proxy->mode = mode;
  proxy->callback(proxy->argument, mode, SLCAN_EVENT_NONE);
}
/*----------------------------------------------------------------------------*/
static bool deserializeFrame(struct CanProxy *proxy, const uint8_t *request,
    size_t length)
{
  struct CanStandardMessage message;
  bool converted;

  if (request[0] == 'T' || request[0] == 'R')
  {
    converted = unpackExtFrame(request, length, &message);
  }
  else
  {
    converted = unpackStdFrame(request, length, &message);
  }

  if (!converted)
  {
    proxy->callback(proxy->argument, proxy->mode, SLCAN_EVENT_SERIAL_ERROR);
    return false;
  }

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
/*----------------------------------------------------------------------------*/
static void serializeFrames(struct CanProxy *proxy,
    const struct CanMessage *frames, size_t count)
{
  uint8_t response[SERIALIZED_QUEUE_SIZE * SERIALIZED_FRAME_MTU];
  size_t index = 0;
  size_t length = 0;

  while (index < count)
  {
    const struct CanMessage * const frame = &frames[index++];
    size_t (*serializer)(void *, const struct CanMessage *) =
        (frame->flags & CAN_EXT_ID) ? packExtFrame : packStdFrame;

    length += serializer(response + length, frame);
  }

  const size_t written = ifWrite(proxy->serial, response, length);

  proxy->callback(proxy->argument, proxy->mode, written == length ?
      SLCAN_EVENT_RX : SLCAN_EVENT_SERIAL_OVERRUN);
}
/*----------------------------------------------------------------------------*/
static void handleCanEvent(void *argument)
{
  struct CanProxy * const proxy = argument;
  proxy->canEvent = false;

  struct CanStandardMessage rxMessages[SERIALIZED_QUEUE_SIZE];
  size_t bytesRead;

  while ((bytesRead = ifRead(proxy->can, rxMessages, sizeof(rxMessages))) > 0)
  {
    serializeFrames(proxy, (const struct CanMessage *)&rxMessages,
        bytesRead / sizeof(struct CanStandardMessage));
  }
}
/*----------------------------------------------------------------------------*/
static void handleSerialEvent(void *argument)
{
  struct CanProxy * const proxy = argument;
  proxy->serialEvent = false;

  size_t rxLength;
  uint8_t rxBuffer[64];

  while ((rxLength = ifRead(proxy->serial, rxBuffer, sizeof(rxBuffer))) > 0)
  {
    for (size_t index = 0; index < rxLength; ++index)
    {
      const uint8_t c = rxBuffer[index];

      if (c == '\r' || c == '\n') /* New line */
      {
        if (!proxy->parser.skip)
        {
          proxy->parser.arena[proxy->parser.position] = '\0';

          char response[RESPONSE_MTU];
          const size_t responseLength = processMessage(proxy,
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
}
/*----------------------------------------------------------------------------*/
static void mockEventHandler(void *argument, enum CanProxyMode mode,
    enum CanProxyEvent event)
{
  (void)argument;
  (void)mode;
  (void)event;
}
/*----------------------------------------------------------------------------*/
static size_t processMessage(struct CanProxy *proxy, const uint8_t *request,
    size_t length, char *response)
{
  if (!length)
    return 0;

  switch (request[0])
  {
    case 's':
      if (length >= 5 && length <= 7 && setCustomRate(proxy, request))
        strcpy(response, "\r");
      else
        strcpy(response, "\a");
      break;

    case 'S':
      if (length == 2 && setPredefinedRate(proxy, request))
        strcpy(response, "\r");
      else
        strcpy(response, "\a");
      break;

    case 'r':
    case 'R':
    case 't':
    case 'T':
      if (deserializeFrame(proxy, request, length))
        strcpy(response, "z\r");
      else
        strcpy(response, "\a");
      break;

    case 'O':
      changePortMode(proxy, SLCAN_MODE_ACTIVE);
      strcpy(response, "\r");
      break;

    case 'I':
      changePortMode(proxy, SLCAN_MODE_LOOPBACK);
      strcpy(response, "\r");
      break;

    case 'L':
      changePortMode(proxy, SLCAN_MODE_LISTENER);
      strcpy(response, "\r");
      break;

    case 'C':
      changePortMode(proxy, SLCAN_MODE_DISABLED);
      strcpy(response, "\r");
      break;

    case 'X':
    {
      if (sendTestMessages(proxy, request, length))
        strcpy(response, "\r");
      else
        strcpy(response, "\a");
    }

    case 'Z':
    {
      /* Enable or disable timestamping */
      break;
    }

    case 'V':
    {
      /* Get hardware version */
      const struct BoardVersion * const ver = getBoardVersion();
      return packNumber16(response, (ver->hw.major << 8) | ver->hw.minor);
    }

    case 'v':
    {
      /* Get software version */
      const struct BoardVersion * const ver = getBoardVersion();
      return packNumber16(response, (ver->sw.major << 8) | ver->sw.minor);
    }

    case 'N':
      /* Get the serial number */
      if (!proxy->storage)
        return packNumber16(response, 0xFFFF);
      else
        return packNumber16(response, (uint16_t)proxy->storage->values.serial);

    case 'n':
      /* Set the serial number */
      if (length == 5 && proxy->storage &&
          !isSerialNumberValid(proxy->storage->values.serial))
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

    case 'B':
      resetToBootloader();
      strcpy(response, "\a");
      break;

    case 'F':
      strcpy(response, "z00\r");
      break;

    case 'W':
      strcpy(response, "\r");
      break;

    default:
      strcpy(response, "\a");
      break;
  }

  return strlen(response);
}
/*----------------------------------------------------------------------------*/
static bool sendMessageGroup(struct CanProxy *proxy, uint8_t flags,
    size_t length, size_t count)
{
  if (!proxy->chrono)
    return false;

  uint32_t rate;
  if (ifGetParam(proxy->can, IF_RATE, &rate) != E_OK)
    return false;

  const uint32_t frameLength = calcFrameLength(flags, length);
  const uint32_t groupTimeout = (frameLength * 1000000 / rate) * count;
  const uint32_t timestamp = timerGetValue(proxy->chrono);

  for (size_t i = 0; i < count; ++i)
  {
    const struct CanStandardMessage message = {
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
static bool sendTestMessages(struct CanProxy *proxy, const uint8_t *request,
    size_t length)
{
  struct GroupSettings
  {
    uint8_t length;
    uint8_t flags;
  };

  static const size_t GROUP_SIZE = 1000;
  static const struct GroupSettings GROUP_SETTINGS[] = {
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

    for (size_t i = 0; completed && (i < ARRAY_SIZE(GROUP_SETTINGS)); ++i)
    {
      completed = sendMessageGroup(proxy, GROUP_SETTINGS[i].length,
          GROUP_SETTINGS[i].flags, GROUP_SIZE);
    }

    return completed;
  }
  else if (length == 2)
  {
    const unsigned int code = hexToBin(request[1]);

    if (code < ARRAY_SIZE(GROUP_SETTINGS))
    {
      return sendMessageGroup(proxy, GROUP_SETTINGS[code].length,
          GROUP_SETTINGS[code].flags, GROUP_SIZE);
    }
    else
      return false;
  }
  else
    return false;
}
/*----------------------------------------------------------------------------*/
static bool setCustomRate(struct CanProxy *proxy, const uint8_t *request)
{
  uint32_t rate = 0;

  while (*(++request))
    rate = (rate << 4) | hexToBin(*request);

  return ifSetParam(proxy->can, IF_RATE, &rate) == E_OK;
}
/*----------------------------------------------------------------------------*/
static bool setPredefinedRate(struct CanProxy *proxy, const uint8_t *request)
{
  static const uint32_t BAUDRATE_TABLE[] = {
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

  if (code < ARRAY_SIZE(BAUDRATE_TABLE))
  {
    return ifSetParam(proxy->can, IF_RATE, &BAUDRATE_TABLE[code]) == E_OK;
  }
  else
  {
    return false;
  }
}
/*----------------------------------------------------------------------------*/
static void onCanEventCallback(void *argument)
{
  struct CanProxy * const proxy = argument;

  if (!proxy->canEvent)
  {
    proxy->canEvent = true;
    wqAdd(WQ_DEFAULT, handleCanEvent, argument);
  }
}
/*----------------------------------------------------------------------------*/
static void onSerialEventCallback(void *argument)
{
  struct CanProxy * const proxy = argument;
  size_t level;

  if (ifGetParam(proxy->serial, IF_AVAILABLE, &level) == E_OK)
  {
    if (level && !proxy->serialEvent)
    {
      proxy->serialEvent = true;
      wqAdd(WQ_DEFAULT, handleSerialEvent, argument);
    }
  }
}
/*----------------------------------------------------------------------------*/
static enum Result proxyInit(void *object, const void *configBase)
{
  const struct CanProxyConfig * const config = configBase;
  struct CanProxy * const proxy = object;

  assert(config);
  assert(config->can);
  assert(config->serial);

  proxy->can = config->can;
  proxy->serial = config->serial;
  proxy->chrono = config->chrono;
  proxy->storage = config->storage;

  proxy->callback = config->callback ? config->callback : mockEventHandler;
  proxy->argument = config->argument;
  proxy->mode = SLCAN_MODE_DISABLED;
  proxy->parser.position = 0;
  proxy->parser.skip = false;
  proxy->canEvent = false;
  proxy->serialEvent = false;

  ifSetCallback(proxy->can, onCanEventCallback, proxy);
  ifSetCallback(proxy->serial, onSerialEventCallback, proxy);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void proxyDeinit(void *object)
{
  struct CanProxy * const proxy = object;

  ifSetCallback(proxy->serial, 0, 0);
  ifSetCallback(proxy->can, 0, 0);
}
