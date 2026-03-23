#ifndef AAC_RUNTIME_H
#define AAC_RUNTIME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Open accelerator device `path` (e.g. /dev/aac0). */
aac_result_t aac_device_open(const char *path, aac_device_t **out_dev);
void aac_device_close(aac_device_t *dev);

aac_result_t aac_queue_create(aac_device_t *dev, aac_queue_t **out_q);
void aac_queue_destroy(aac_queue_t *q);

/**
 * Allocate device-visible memory. Returns device offset usable in copy/launch.
 * Host pinned buffers can be registered for H2D/D2H without extra mmap (driver copies from user pointer).
 */
aac_result_t aac_buffer_alloc(aac_device_t *dev, size_t size, uint32_t flags,
			      uint64_t *out_dev_offset);
aac_result_t aac_buffer_free(aac_device_t *dev, uint64_t dev_offset);

aac_result_t aac_memcpy_h2d(aac_queue_t *q, const void *host, uint64_t dev_offset,
			    size_t size);
aac_result_t aac_memcpy_d2h(aac_queue_t *q, uint64_t dev_offset, void *host,
			    size_t size);

#define AAC_MAX_ARGS 16

typedef struct {
	uint64_t code_dev_offset;
	uint32_t entry_offset;
	uint32_t grid[3];
	uint32_t block[3];
	uint32_t arg_count;
	aac_kernel_arg_t args[AAC_MAX_ARGS];
} aac_launch_desc_t;

aac_result_t aac_launch(aac_queue_t *q, const aac_launch_desc_t *desc);
aac_result_t aac_sync(aac_queue_t *q);

#ifdef __cplusplus
}
#endif

#endif
