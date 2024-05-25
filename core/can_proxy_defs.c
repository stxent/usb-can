/*
 * core/can_proxy_defs.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "can_proxy_defs.h"
#include "helpers.h"
#include <halm/generic/can.h>
/*----------------------------------------------------------------------------*/
static size_t packExtFrame(void *, const struct CANStandardMessage *);
static size_t packStdFrame(void *, const struct CANStandardMessage *);
static bool unpackExtFrame(const void *, size_t, struct CANStandardMessage *);
static bool unpackStdFrame(const void *, size_t, struct CANStandardMessage *);
/*----------------------------------------------------------------------------*/
static size_t packExtFrame(void *buffer,
    const struct CANStandardMessage *message)
{
  uint8_t *frame = buffer;
  uint32_t word;

  *frame++ = (message->flags & CAN_RTR) ? 'R' : 'T';

  word = binToHex4(message->id >> 16);
  memcpy(frame, &word, sizeof(word));
  frame += sizeof(word);

  word = binToHex4(message->id);
  memcpy(frame, &word, sizeof(word));
  frame += sizeof(word);

  *frame++ = binToHex(message->length);

  uint8_t * const eof = frame + message->length * 2;

  for (size_t i = 0; i < message->length; i += 2)
  {
    const uint16_t pair = message->data[i + 1] | (message->data[i] << 8);

    word = binToHex4(pair);
    memcpy(frame, &word, sizeof(word));
    frame += sizeof(word);
  }

  *eof = '\r';
  return (eof - (uint8_t *)buffer) + 1;
}
/*----------------------------------------------------------------------------*/
static size_t packStdFrame(void *buffer,
    const struct CANStandardMessage *message)
{
  uint8_t *frame = buffer;
  uint32_t word;

  *frame++ = (message->flags & CAN_RTR) ? 'r' : 't';

  word = binToHex4((message->id << 4) | message->length);
  memcpy(frame, &word, sizeof(word));
  frame += sizeof(word);

  uint8_t * const eof = frame + message->length * 2;

  for (size_t i = 0; i < message->length; i += 2)
  {
    const uint16_t pair = message->data[i + 1] | (message->data[i] << 8);

    word = binToHex4(pair);
    memcpy(frame, &word, sizeof(word));
    frame += sizeof(word);
  }

  *eof = '\r';
  return (eof - (uint8_t *)buffer) + 1;
}
/*----------------------------------------------------------------------------*/
static bool unpackExtFrame(const void *request, size_t length,
    struct CANStandardMessage *message)
{
  if (length < EXT_DATA_OFFSET)
    return false;

  const uint8_t *frame = request;
  const char type = (char)*frame;

  message->flags = type == 'R' ? (CAN_EXT_ID | CAN_RTR) : CAN_EXT_ID;
  frame += sizeof(uint8_t);
  message->id = (inPlaceHexToBin4(frame) << 16) | inPlaceHexToBin4(frame + 4);
  frame += sizeof(uint32_t) * 2;
  message->length = hexToBin(*frame);
  frame += sizeof(uint8_t);

  if (message->length > 8)
    return false;
  if (type == 'T' && message->length > (length - EXT_DATA_OFFSET) >> 1)
    return false;

  for (size_t i = 0; i < message->length; i += 2)
  {
    const uint16_t pair = inPlaceHexToBin4(frame);
    frame += sizeof(uint32_t);

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

  const uint8_t *frame = request;
  const char type = (char)*frame;
  const uint16_t joinedIdLength = inPlaceHexToBin4(frame + 1);

  message->id = joinedIdLength >> 4;
  message->flags = type == 'r' ? CAN_RTR : 0;
  message->length = joinedIdLength & 0x000F;

  if (message->length > 8)
    return false;
  if (type == 't' && message->length > (length - STD_DATA_OFFSET) >> 1)
    return false;

  frame += STD_DATA_OFFSET;
  for (size_t i = 0; i < message->length; i += 2)
  {
    const uint16_t pair = inPlaceHexToBin4(frame);
    frame += sizeof(uint32_t);

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
size_t packFrame(void *buffer, const struct CANStandardMessage *message)
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
size_t packNumber16(void *buffer, char prefix, uint16_t value)
{
  const struct PackedNumber16 response = {
      .prefix = prefix,
      .number = binToHex4(value),
      .eof = '\r'
  };

  memcpy(buffer, &response, sizeof(response));
  return sizeof(response);
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
