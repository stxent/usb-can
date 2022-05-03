/*
 * core/can_proxy_defs.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "can_proxy_defs.h"
#include "helpers.h"
#include <halm/generic/can.h>
/*----------------------------------------------------------------------------*/
static size_t packExtFrame(void *, const struct CANMessage *);
static size_t packStdFrame(void *, const struct CANMessage *);
static bool unpackExtFrame(const void *, size_t, struct CANStandardMessage *);
static bool unpackStdFrame(const void *, size_t, struct CANStandardMessage *);
/*----------------------------------------------------------------------------*/
static size_t packExtFrame(void *buffer, const struct CANMessage *message)
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
static size_t packStdFrame(void *buffer, const struct CANMessage *message)
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
static bool unpackExtFrame(const void *request, size_t length,
    struct CANStandardMessage *message)
{
  if (length < EXT_DATA_OFFSET)
    return false;

  const struct PackedExtFrame * const frame = request;

  message->id = (hexToBin4(frame->id[0]) << 16) | hexToBin4(frame->id[1]);
  message->flags = frame->type == 'R' ? (CAN_EXT_ID | CAN_RTR) : CAN_EXT_ID;
  message->length = hexToBin(frame->length);

  if (frame->type == 'T' && message->length > (length - EXT_DATA_OFFSET) >> 1)
    return false;

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
    struct CANStandardMessage *message)
{
  if (length < STD_DATA_OFFSET)
    return false;

  const struct PackedStdFrame * const frame = request;
  const uint16_t joinedIdLength = hexToBin4(frame->joinedIdLength);

  message->id = joinedIdLength >> 4;
  message->flags = frame->type == 'r' ? CAN_RTR : 0;
  message->length = joinedIdLength & 0x000F;

  if (frame->type == 't' && message->length > (length - STD_DATA_OFFSET) >> 1)
    return false;

  for (size_t i = 0; i < message->length; i += 2)
  {
    const uint16_t pair = hexToBin4(frame->data[i >> 1]);

    message->data[i] = pair >> 8;
    message->data[i + 1] = pair;
  }

  return true;
}
/*----------------------------------------------------------------------------*/
uint32_t calcFrameLength(uint8_t flags, size_t dlc)
{
  uint32_t length = 0;

  /* System fields */
  length += (flags & CAN_EXT_ID) ? 64 : 44;
  /* Data field */
  length += (flags & CAN_RTR) ? 0 : (dlc << 3);
  /* Bit stuffing */
  length += (length - 11) >> 2;
  /* Interframe spacing */
  length += 3;

  return length;
}
/*----------------------------------------------------------------------------*/
size_t packFrame(void *buffer, const struct CANMessage *message)
{
  if (message->flags & CAN_EXT_ID)
  {
    return packExtFrame(buffer, message);
  }
  else
  {
    return packStdFrame(buffer, message);
  }
}
/*----------------------------------------------------------------------------*/
size_t packNumber16(void *buffer, uint16_t value)
{
  struct PackedNumber16 * const response = buffer;

  response->prefix = 'z';
  response->number = binToHex4(value);
  response->eof = '\r';

  return sizeof(struct PackedNumber16);
}
/*----------------------------------------------------------------------------*/
bool unpackFrame(const void *buffer, size_t length,
    struct CANStandardMessage *message)
{
  const char * const request = buffer;

  if (request[0] == 'T' || request[0] == 'R')
  {
    return unpackExtFrame(request, length, message);
  }
  else
  {
    return unpackStdFrame(request, length, message);
  }
}
