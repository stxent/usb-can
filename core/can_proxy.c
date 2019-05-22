/*
 * core/can_proxy.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <halm/generic/can.h>
#include <halm/generic/work_queue.h>
#include <xcore/memory.h>
#include "can_proxy.h"
#include "version.h"
/*----------------------------------------------------------------------------*/
struct PackedExtFrame
{
  uint8_t type;
  uint32_t id[2];
  uint8_t length;
  uint32_t data[4];
  uint8_t eof;
} __attribute__((packed));

struct PackedStdFrame
{
  uint8_t type;
  uint32_t joinedIdLength;
  uint32_t data[4];
  uint8_t eof;
} __attribute__((packed));

#define SERIALIZED_FRAME_MTU  sizeof(struct PackedExtFrame)
#define SERIALIZED_QUEUE_SIZE 4
/*----------------------------------------------------------------------------*/
struct CanProxy
{
  struct Entity base;

  struct Interface *can;
  struct Interface *serial;
  struct Timer *timer;

  ProxyCallback callback;
  void *argument;

  enum ProxyPortMode mode;

  struct
  {
    size_t position;
    uint8_t arena[SERIALIZED_FRAME_MTU + 1];
  } parser;

  bool canEvent;
  bool serialEvent;
};
/*----------------------------------------------------------------------------*/
static uint8_t binToHex(uint8_t);
static uint32_t binToHex4(uint16_t);
static uint8_t hexToBin(uint8_t);
static uint16_t hexToBin4(uint32_t);
static void changePortMode(struct CanProxy *, enum ProxyPortMode);
static void packFrames(struct CanProxy *, const struct CanMessage *, size_t);
static void handleCanEvent(void *);
static void handleSerialEvent(void *);
static void mockEventHandler(void *, enum ProxyPortMode, enum ProxyEvent);
static size_t processMessage(struct CanProxy *, const uint8_t *, size_t,
    char *);
static bool unpackFrame(struct CanProxy *, const uint8_t *, size_t);
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
static uint8_t binToHex(uint8_t value)
{
  return value + (value < 10 ? 0x30 : 0x37);
}
/*----------------------------------------------------------------------------*/
static uint32_t binToHex4(uint16_t value)
{
  const uint32_t t0 = (value | (value << 8)) & 0x00FF00FFUL;
  const uint32_t t1 = (t0 | (t0 << 4)) & 0x0F0F0F0FUL;
  const uint32_t t2 = (((t1 + 0x06060606UL) & 0x10101010UL) >> 4) * 0x07;

  return toBigEndian32(t1 + t2 + 0x30303030UL);
}
/*----------------------------------------------------------------------------*/
//static uint8_t codeToFlags(uint8_t c)
//{
//  uint8_t flags = 0;
//
//  /* Is an upper case character */
//  if (c < 'Z')
//    flags |= CAN_EXT_ID;
//
//  /* Convert to upper case and compare */
//  if ((c & 0xCF) == 'R')
//    flags |= CAN_RTR;
//
//  return flags;
//}
/*----------------------------------------------------------------------------*/
static uint8_t hexToBin(uint8_t code)
{
  code &= 0xCF;
  return code < 10 ? code : (code - 0x37);
}
/*----------------------------------------------------------------------------*/
static uint16_t hexToBin4(uint32_t value)
{
  const uint32_t t0 = fromBigEndian32(value) & 0xCFCFCFCFUL;
  const uint32_t t1 = t0 - ((t0 & 0x40404040UL) >> 6) * 0x37;
  const uint32_t t2 = (t1 | (t1 >> 4)) & 0x00FF00FFUL;

  return t2 | (t2 >> 8);
}
/*----------------------------------------------------------------------------*/
static void changePortMode(struct CanProxy *proxy, enum ProxyPortMode mode)
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
  proxy->callback(proxy->argument, proxy->mode, SLCAN_EVENT_NONE);
}
/*----------------------------------------------------------------------------*/
static size_t packExtFrame(void *buffer, const struct CanMessage *message)
{
  struct PackedExtFrame * const frame = buffer;
  const size_t length = offsetof(struct PackedExtFrame, data)
      + (message->length << 1);

  frame->type = (message->flags & CAN_RTR) ? 'R' : 'T';
  frame->id[0] = binToHex4(message->id >> 16);
  frame->id[1] = binToHex4(message->id);
  frame->length = binToHex(message->length);

  for (size_t i = 0; i < message->length; i += 2)
  {
    const uint16_t pair = message->data[i + 1] | (message->data[i] << 8);
    frame->data[i >> 1] = binToHex4(pair);
  }

  *((uint8_t *)buffer + length) = '\r';
  return length + 1;
}
/*----------------------------------------------------------------------------*/
static size_t packStdFrame(void *buffer, const struct CanMessage *message)
{
  struct PackedStdFrame * const frame = buffer;
  const size_t length = offsetof(struct PackedStdFrame, data)
      + (message->length << 1);

  frame->type = (message->flags & CAN_RTR) ? 'r' : 't';
  frame->joinedIdLength = binToHex4((message->id << 4) | message->length);

  for (size_t i = 0; i < message->length; i += 2)
  {
    const uint16_t pair = message->data[i + 1] | (message->data[i] << 8);
    frame->data[i >> 1] = binToHex4(pair);
  }

  *((uint8_t *)buffer + length) = '\r';
  return length + 1;
}
/*----------------------------------------------------------------------------*/
static void packFrames(struct CanProxy *proxy,
    const struct CanMessage *message, size_t count)
{
  uint8_t response[SERIALIZED_QUEUE_SIZE * SERIALIZED_FRAME_MTU];
  size_t length = 0;

  while (count--)
  {
    size_t (*serializer)(void *, const struct CanMessage *) =
        (message->flags & CAN_EXT_ID) ? packExtFrame : packStdFrame;

    length += serializer(response + length, message);
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
    packFrames(proxy, (const struct CanMessage *)&rxMessages,
        bytesRead / sizeof(struct CanStandardMessage));
  }
}
/*----------------------------------------------------------------------------*/
static void handleSerialEvent(void *argument)
{
  struct CanProxy * const proxy = argument;
  proxy->serialEvent = false;

  size_t rxCount;
  uint8_t rxBuffer[64];

  while ((rxCount = ifRead(proxy->serial, rxBuffer, sizeof(rxBuffer))) > 0)
  {
    for (size_t index = 0; index < rxCount; ++index)
    {
      const uint8_t c = rxBuffer[index];

      if (c == '\r' || c == '\n') /* New line */
      {
        proxy->parser.arena[proxy->parser.position] = '\0';

        char response[32] = {0}; //TODO Select size
        const size_t responseLength = processMessage(proxy,
            proxy->parser.arena, proxy->parser.position, response);

        if (responseLength)
        {
          ifWrite(proxy->serial, response, responseLength);
          // TODO Buffer overflow handling
        }

        proxy->parser.position = 0;
      }
      else if (proxy->parser.position < sizeof(proxy->parser.arena) - 1)
      {
        proxy->parser.arena[proxy->parser.position++] = c;
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
static void mockEventHandler(void *argument, enum ProxyPortMode mode,
    enum ProxyEvent event)
{
  (void)argument;
  (void)mode;
  (void)event;
}
/*----------------------------------------------------------------------------*/
static bool sendMessageGroup(struct CanProxy *proxy, uint8_t flags,
    size_t length, size_t count, uint32_t *passed)
{
  if (!proxy->timer)
    return false;

  uint32_t rate;
  uint32_t timeout = 0;

  if (ifGetParam(proxy->can, IF_RATE, &rate) != E_OK)
    return false;

  /* System fields */
  timeout += (flags & CAN_EXT_ID) ? 64 : 44;
  /* Data field */
  timeout += (flags & CAN_RTR) ? 0 : (length << 3);
  /* Bit stuffing */
  timeout += (timeout - 11) >> 2;
  /* Interframe spacing */
  timeout += 3;
  /* Convert to microseconds */
  timeout = (timeout * 1000000 / rate) * count;

  timerSetValue(proxy->timer, 0);

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
      if (timerGetValue(proxy->timer) > timeout)
        return false;
    }
  }

  if (passed)
    *passed = timerGetValue(proxy->timer);

  return true;
}
/*----------------------------------------------------------------------------*/
static bool sendTestMessages(struct CanProxy *proxy, const uint8_t *request,
    uint32_t *passed)
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

  const unsigned int code = hexToBin(request[1]);

  if (code < ARRAY_SIZE(GROUP_SETTINGS))
  {
    return sendMessageGroup(proxy, GROUP_SETTINGS[code].length,
        GROUP_SETTINGS[code].flags, GROUP_SIZE, passed);
  }
  else
  {
    return false;
  }
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
static bool setCustomRate(struct CanProxy *proxy, const uint8_t *request)
{
  const uint32_t rate = hexToBin(request[5])
      | (hexToBin(request[4]) << 4)
      | (hexToBin(request[3]) << 8)
      | (hexToBin(request[2]) << 12)
      | (hexToBin(request[1]) << 16);

  return ifSetParam(proxy->can, IF_RATE, &rate) == E_OK;
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
      if (setCustomRate(proxy, request))
        strcpy(response, "\r");
      else
        strcpy(response, "\a");
      break;

    case 'S':
      if (setPredefinedRate(proxy, request))
        strcpy(response, "\r");
      else
        strcpy(response, "\a");
      break;

    case 'r':
    case 'R':
    case 't':
    case 'T':
      if (unpackFrame(proxy, request, length))
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
      uint32_t passed;

      if (sendTestMessages(proxy, request, &passed))
      {
        response[0] = 'z';
        response[1] = binToHex((passed >> 12) & 0x0F);
        response[2] = binToHex((passed >> 8) & 0x0F);
        response[3] = binToHex((passed >> 4) & 0x0F);
        response[4] = binToHex((passed >> 0) & 0x0F);
        response[5] = '\r';
        response[6] = '\0';
      }
      else
        strcpy(response, "\a");
    }

    case 'Z':
    {
      /* Enable or disable timestamping */
      break;
    }

    case 'V':
      /* Get hardware version */
      response[0] = 'z';
      response[1] = binToHex((VERSION_HW >> 12) & 0x0F);
      response[2] = binToHex((VERSION_HW >> 8) & 0x0F);
      response[3] = binToHex((VERSION_HW >> 4) & 0x0F);
      response[4] = binToHex((VERSION_HW >> 0) & 0x0F);
      response[5] = '\r';
      response[6] = '\0';
      break;

    case 'v':
      /* Get software version */
      response[0] = 'z';
      response[1] = binToHex((VERSION_SW >> 12) & 0x0F);
      response[2] = binToHex((VERSION_SW >> 8) & 0x0F);
      response[3] = binToHex((VERSION_SW >> 4) & 0x0F);
      response[4] = binToHex((VERSION_SW >> 0) & 0x0F);
      response[5] = '\r';
      response[6] = '\0';
      break;

    case 'N':
      strcpy(response, "zFFFF\r");
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
static bool unpackExtFrame(const void *request, size_t length,
    struct CanStandardMessage *message)
{
  if (length < offsetof(struct PackedExtFrame, data))
    return false;

  const struct PackedExtFrame * const frame = request;

  message->id = (hexToBin4(frame->id[0]) << 16) | hexToBin4(frame->id[1]);
  message->flags = frame->type == 'R' ? (CAN_EXT_ID | CAN_RTR) : CAN_EXT_ID;
  message->length = hexToBin(frame->length);

  if (message->length > (length - offsetof(struct PackedExtFrame, data)) >> 1)
  {
    return false;
  }

  for (size_t i = 0; i < message->length; i += 2)
  {
    const uint16_t pair = hexToBin4(frame->data[i >> 1]);

    message->data[i] = pair >> 8;
    message->data[i + 1] = pair;
  }

  return true;
}
/*----------------------------------------------------------------------------*/
static bool unpackStdFrame(const void *request, size_t length,
    struct CanStandardMessage *message)
{
  if (length < offsetof(struct PackedStdFrame, data))
    return false;

  const struct PackedStdFrame * const frame = request;
  const uint16_t joinedIdLength = hexToBin4(frame->joinedIdLength);

  message->id = joinedIdLength >> 4;
  message->flags = frame->type == 'r' ? CAN_RTR : 0;
  message->length = joinedIdLength & 0x000F;

  if (message->length > (length - offsetof(struct PackedStdFrame, data)) >> 1)
  {
    return false;
  }

  for (size_t i = 0; i < message->length; i += 2)
  {
    const uint16_t pair = hexToBin4(frame->data[i >> 1]);

    message->data[i] = pair >> 8;
    message->data[i + 1] = pair;
  }

  return true;
}
/*----------------------------------------------------------------------------*/
static bool unpackFrame(struct CanProxy *proxy, const uint8_t *request,
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
    // TODO Notify about parser errors
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
static void onCanEventCallback(void *argument)
{
  struct CanProxy * const proxy = argument;

  if (!proxy->canEvent)
  {
    proxy->canEvent = true;
    workQueueAdd(handleCanEvent, argument);
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
      workQueueAdd(handleSerialEvent, argument);
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
  proxy->timer = config->timer;
  proxy->callback = mockEventHandler;
  proxy->argument = 0;
  proxy->mode = SLCAN_MODE_DISABLED;
  proxy->parser.position = 0;
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
/*----------------------------------------------------------------------------*/
void proxySetCallback(struct CanProxy *proxy, ProxyCallback callback,
    void *argument)
{
  proxy->callback = callback ? callback : mockEventHandler;
  proxy->argument = argument;
}
