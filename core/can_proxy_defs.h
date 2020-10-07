/*
 * core/can_proxy_defs.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

 #ifndef CORE_CAN_PROXY_DEFS_H_
 #define CORE_CAN_PROXY_DEFS_H_
/*----------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <xcore/helpers.h>
/*----------------------------------------------------------------------------*/
struct CanMessage;
struct CanStandardMessage;

struct PackedExtFrame
{
  uint8_t type;
  uint32_t id[2];
  uint8_t length;
  uint32_t data[4];
  uint8_t eof;
} __attribute__((packed));

#define EXT_DATA_OFFSET offsetof(struct PackedExtFrame, data)

struct PackedStdFrame
{
  uint8_t type;
  uint32_t joinedIdLength;
  uint32_t data[4];
  uint8_t eof;
} __attribute__((packed));

#define STD_DATA_OFFSET offsetof(struct PackedStdFrame, data)

struct PackedNumber16
{
  uint8_t prefix;
  uint32_t number;
  uint8_t eof;
} __attribute__((packed));

#define RESPONSE_MTU          sizeof(struct PackedNumber16)
#define SERIALIZED_FRAME_MTU  sizeof(struct PackedExtFrame)
#define SERIALIZED_QUEUE_SIZE 2
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

uint32_t calcFrameLength(uint8_t, size_t);
size_t packExtFrame(void *, const struct CanMessage *);
size_t packStdFrame(void *, const struct CanMessage *);
size_t packNumber16(void *, uint16_t);
bool unpackExtFrame(const void *, size_t, struct CanStandardMessage *);
bool unpackStdFrame(const void *, size_t, struct CanStandardMessage *);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* CORE_CAN_PROXY_DEFS_H_ */
