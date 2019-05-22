/*
 * core/flash_loader.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <halm/generic/flash.h>
#include <halm/generic/work_queue.h>
#include <halm/irq.h>
#include <halm/usb/dfu.h>
#include "flash_loader.h"
/*----------------------------------------------------------------------------*/
static const struct FlashGeometry *findFlashRegion(const struct FlashLoader *,
    size_t);
static void flashLoaderReset(struct FlashLoader *);
static void flashProgramTask(void *);
static uint32_t getSectorEraseTime(const struct FlashLoader *, size_t);
static bool isSectorAddress(const struct FlashLoader *, size_t);
static void onDetachRequest(void *, uint16_t);
static size_t onDownloadRequest(void *, size_t, const void *, size_t,
    uint16_t *);
static size_t onUploadRequest(void *, size_t, void *, size_t);
/*----------------------------------------------------------------------------*/
static enum Result flashLoaderInit(void *, const void *);
static void flashLoaderDeinit(void *);
/*----------------------------------------------------------------------------*/
const struct EntityClass * const FlashLoader = &(const struct EntityClass){
    .size = sizeof(struct FlashLoader),
    .init = flashLoaderInit,
    .deinit = flashLoaderDeinit
};
/*----------------------------------------------------------------------------*/
static const struct FlashGeometry *findFlashRegion(
    const struct FlashLoader *loader, size_t address)
{
  if (address >= loader->flashSize)
    return 0;

  const struct FlashGeometry *region = loader->geometry;
  size_t offset = 0;

  while (region->count) {
    /* Sector size should be a power of 2 */
    assert((region->size & (region->size - 1)) == 0);

    if (((address - offset) & (region->size - 1)) == 0)
      return region;

    offset += region->count * region->size;
    ++region;
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
static void flashLoaderReset(struct FlashLoader *loader)
{
  loader->bufferSize = 0;
  loader->currentPosition = loader->flashOffset;
  loader->erasingPosition = 0;
  loader->eraseQueued = false;
  memset(loader->chunk, 0xFF, loader->pageSize);
}
/*----------------------------------------------------------------------------*/
static void flashProgramTask(void *argument)
{
  struct FlashLoader * const loader = argument;

  loader->eraseQueued = false;

  const IrqState irqState = irqSave();
  ifSetParam(loader->flash, IF_FLASH_ERASE_SECTOR, &loader->erasingPosition);
  dfuOnDownloadCompleted(loader->device, true);
  irqRestore(irqState);
}
/*----------------------------------------------------------------------------*/
static uint32_t getSectorEraseTime(const struct FlashLoader *loader,
    size_t address)
{
  const struct FlashGeometry * const region = findFlashRegion(loader, address);
  return region ? region->time : 0;
}
/*----------------------------------------------------------------------------*/
static bool isSectorAddress(const struct FlashLoader *loader, size_t address)
{
  const struct FlashGeometry * const region = findFlashRegion(loader, address);
  return region != 0;
}
/*----------------------------------------------------------------------------*/
static void onDetachRequest(void *object,
    uint16_t timeout __attribute__((unused)))
{
  struct FlashLoader * const loader = object;
  loader->reset();
}
/*----------------------------------------------------------------------------*/
static size_t onDownloadRequest(void *object, size_t position,
    const void *buffer, size_t length, uint16_t *timeout)
{
  struct FlashLoader * const loader = object;

  if (!position)
  {
    /* Reset position and erase first sector */
    flashLoaderReset(loader);
    loader->erasingPosition = loader->currentPosition;
    loader->eraseQueued = true;
    workQueueAdd(flashProgramTask, loader);
  }

  if (loader->currentPosition + length > loader->flashSize)
    return 0;

  size_t processed = 0;

  do
  {
    if (!length || loader->bufferSize == loader->pageSize)
    {
      const enum Result res = ifSetParam(loader->flash, IF_POSITION,
          &loader->currentPosition);

      if (res != E_OK)
        return 0;

      const size_t written = ifWrite(loader->flash, loader->chunk,
          loader->pageSize);

      if (written != loader->pageSize)
        return 0;

      loader->currentPosition += loader->bufferSize;
      loader->bufferSize = 0;
      memset(loader->chunk, 0xFF, loader->pageSize);

      if (isSectorAddress(loader, loader->currentPosition))
      {
        /* Enqueue sector erasure */
        loader->erasingPosition = loader->currentPosition;
        loader->eraseQueued = true;
        workQueueAdd(flashProgramTask, loader);
      }
    }

    const size_t chunkSize = length <= loader->pageSize - loader->bufferSize ?
        length : loader->pageSize - loader->bufferSize;

    memcpy(loader->chunk + loader->bufferSize,
        (const uint8_t *)buffer + processed, chunkSize);

    loader->bufferSize += chunkSize;
    processed += chunkSize;
  }
  while (processed < length);

  *timeout = loader->eraseQueued ?
      getSectorEraseTime(loader, loader->erasingPosition) : 0;

  return length;
}
/*----------------------------------------------------------------------------*/
static size_t onUploadRequest(void *object, size_t position, void *buffer,
    size_t length)
{
  struct FlashLoader * const loader = object;
  const size_t offset = position + loader->flashOffset;

  if (offset + length > loader->flashSize)
    return 0;
  if (ifSetParam(loader->flash, IF_POSITION, &offset) != E_OK)
    return 0;

  return ifRead(loader->flash, buffer, length);
}
/*----------------------------------------------------------------------------*/
static enum Result flashLoaderInit(void *object, const void *configBase)
{
  const struct FlashLoaderConfig * const config = configBase;
  struct FlashLoader * const loader = object;
  enum Result res;

  assert(config);

  loader->flash = config->flash;
  loader->device = config->device;
  loader->reset = config->reset;
  loader->geometry = config->geometry;
  loader->flashOffset = config->offset;

  res = ifGetParam(loader->flash, IF_SIZE, &loader->flashSize);
  if (res != E_OK)
    return res;
  if (loader->flashOffset >= loader->flashSize)
    return E_VALUE;

  res = ifGetParam(loader->flash, IF_FLASH_PAGE_SIZE, &loader->pageSize);
  if (res != E_OK)
    return res;

  loader->chunk = malloc(loader->pageSize);
  if (!loader->chunk)
    return E_MEMORY;

  if (loader->reset)
    dfuSetDetachRequestCallback(loader->device, onDetachRequest);

  dfuSetCallbackArgument(loader->device, loader);
  dfuSetDownloadRequestCallback(loader->device, onDownloadRequest);
  dfuSetUploadRequestCallback(loader->device, onUploadRequest);

  flashLoaderReset(loader);
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void flashLoaderDeinit(void *object)
{
  struct FlashLoader * const loader = object;

  dfuSetUploadRequestCallback(loader->device, 0);
  dfuSetDownloadRequestCallback(loader->device, 0);
  dfuSetDetachRequestCallback(loader->device, 0);
  dfuSetCallbackArgument(loader->device, 0);

  free(loader->chunk);
}
