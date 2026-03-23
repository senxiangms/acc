#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "aac/ioctl.h"
#include "aac/runtime.h"

struct aac_device {
	int fd;
};

struct aac_queue {
	aac_device_t *dev;
};

static aac_result_t map_errno(void)
{
	switch (errno) {
	case ENOMEM:
		return AAC_ERR_NOMEM;
	case EINVAL:
		return AAC_ERR_INVAL;
	default:
		return AAC_ERR_IO;
	}
}

aac_result_t aac_device_open(const char *path, aac_device_t **out_dev)
{
	int fd = open(path, O_RDWR);
	if (fd < 0)
		return AAC_ERR_IO;
	aac_device_t *d = calloc(1, sizeof(*d));
	if (!d) {
		close(fd);
		return AAC_ERR_NOMEM;
	}
	d->fd = fd;
	*out_dev = d;
	return AAC_OK;
}

void aac_device_close(aac_device_t *dev)
{
	if (!dev)
		return;
	close(dev->fd);
	free(dev);
}

aac_result_t aac_queue_create(aac_device_t *dev, aac_queue_t **out_q)
{
	if (!dev || !out_q)
		return AAC_ERR_INVAL;
	aac_queue_t *q = calloc(1, sizeof(*q));
	if (!q)
		return AAC_ERR_NOMEM;
	q->dev = dev;
	*out_q = q;
	return AAC_OK;
}

void aac_queue_destroy(aac_queue_t *q)
{
	free(q);
}

aac_result_t aac_buffer_alloc(aac_device_t *dev, size_t size, uint32_t flags,
			      uint64_t *out_dev_offset)
{
	if (!dev || !out_dev_offset || size == 0)
		return AAC_ERR_INVAL;
	struct aac_ioctl_alloc a = {
		.size = size,
		.flags = flags,
	};
	if (ioctl(dev->fd, AAC_IOCTL_ALLOC, &a) < 0)
		return map_errno();
	*out_dev_offset = a.dev_offset_out;
	return AAC_OK;
}

aac_result_t aac_buffer_free(aac_device_t *dev, uint64_t dev_offset)
{
	if (!dev)
		return AAC_ERR_INVAL;
	struct aac_ioctl_free f = { .dev_offset = dev_offset };
	if (ioctl(dev->fd, AAC_IOCTL_FREE, &f) < 0)
		return map_errno();
	return AAC_OK;
}

aac_result_t aac_memcpy_h2d(aac_queue_t *q, const void *host, uint64_t dev_offset,
			    size_t size)
{
	if (!q || (!host && size > 0))
		return AAC_ERR_INVAL;
	struct aac_ioctl_copy_h2d c = {
		.host_user_ptr = (uintptr_t)host,
		.dev_offset = dev_offset,
		.size = size,
	};
	if (ioctl(q->dev->fd, AAC_IOCTL_COPY_H2D, &c) < 0)
		return map_errno();
	return AAC_OK;
}

aac_result_t aac_memcpy_d2h(aac_queue_t *q, uint64_t dev_offset, void *host,
			    size_t size)
{
	if (!q || (!host && size > 0))
		return AAC_ERR_INVAL;
	struct aac_ioctl_copy_d2h c = {
		.dev_offset = dev_offset,
		.host_user_ptr = (uintptr_t)host,
		.size = size,
	};
	if (ioctl(q->dev->fd, AAC_IOCTL_COPY_D2H, &c) < 0)
		return map_errno();
	return AAC_OK;
}

aac_result_t aac_launch(aac_queue_t *q, const aac_launch_desc_t *desc)
{
	if (!q || !desc || desc->arg_count > AAC_MAX_ARGS)
		return AAC_ERR_INVAL;
	struct aac_ioctl_launch l;
	memset(&l, 0, sizeof(l));
	l.code_dev_offset = desc->code_dev_offset;
	l.entry_offset = desc->entry_offset;
	l.arg_count = desc->arg_count;
	l.grid_x = desc->grid[0];
	l.grid_y = desc->grid[1];
	l.grid_z = desc->grid[2];
	l.block_x = desc->block[0];
	l.block_y = desc->block[1];
	l.block_z = desc->block[2];
	for (uint32_t i = 0; i < desc->arg_count; i++) {
		l.args[i].type = (uint32_t)desc->args[i].type;
		switch (desc->args[i].type) {
		case AAC_ARG_U32:
			l.args[i].value = desc->args[i].v.u32;
			break;
		case AAC_ARG_U64:
		case AAC_ARG_PTR_DEVICE:
			l.args[i].value = desc->args[i].v.u64;
			break;
		default:
			return AAC_ERR_INVAL;
		}
	}
	if (ioctl(q->dev->fd, AAC_IOCTL_LAUNCH, &l) < 0)
		return map_errno();
	return AAC_OK;
}

aac_result_t aac_sync(aac_queue_t *q)
{
	if (!q)
		return AAC_ERR_INVAL;
	if (ioctl(q->dev->fd, AAC_IOCTL_QUEUE_SYNC, 0) < 0)
		return map_errno();
	return AAC_OK;
}
