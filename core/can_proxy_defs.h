/*
 * core/can_proxy_defs.h
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

 #ifndef CORE_CAN_PROXY_DEFS_H_
 #define CORE_CAN_PROXY_DEFS_H_
/*----------------------------------------------------------------------------*/
#include <xcore/helpers.h>
#include <stddef.h>
#include <stdint.h>
/*----------------------------------------------------------------------------*/
struct CANStandardMessage;

/* Type (1) + ID (4 * 2) + Length (1) */
#define EXT_DATA_OFFSET (1 + 4 * 2 + 1)
/* Data Offset + Data (2 * 8) + EOF (1) */
#define EXT_MAX_LENGTH  (EXT_DATA_OFFSET + 2 * 8 + 1)
/* Type (1) + ID (3) + Length (1) */
#define STD_DATA_OFFSET (1 + 3 + 1)

struct [[gnu::packed]] PackedNumber16
{
  uint8_t prefix;
  uint32_t number;
  uint8_t eof;
};

#define RESPONSE_MTU          sizeof(struct PackedNumber16)
#define SERIALIZED_FRAME_MTU  EXT_MAX_LENGTH

#ifdef CONFIG_SERIAL_HS
/* Serial over High-Speed USB */
#  define SERIAL_MTU            512
#  define SERIALIZED_QUEUE_SIZE 16
#else
#  define SERIAL_MTU            64
#  define SERIALIZED_QUEUE_SIZE 2
#endif
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

uint32_t calcFrameLength(uint8_t, size_t);
size_t packFrame(void *, const struct CANStandardMessage *);
size_t packNumber16(void *, char, uint16_t);
bool unpackFrame(const void *, size_t, struct CANStandardMessage *);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* CORE_CAN_PROXY_DEFS_H_ */
