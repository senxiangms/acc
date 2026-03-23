#ifndef AAC_IOCTL_H
#define AAC_IOCTL_H

#include <stdint.h>

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define AAC_IOCTL_MAGIC 0xC1

struct aac_ioctl_alloc {
	uint64_t size;
	uint32_t flags;
	uint32_t _pad;
	uint64_t dev_offset_out;
};

struct aac_ioctl_free {
	uint64_t dev_offset;
};

struct aac_ioctl_copy_h2d {
	uint64_t host_user_ptr;
	uint64_t dev_offset;
	uint64_t size;
};

struct aac_ioctl_copy_d2h {
	uint64_t dev_offset;
	uint64_t host_user_ptr;
	uint64_t size;
};

struct aac_ioctl_kernel_arg {
	uint32_t type;
	uint32_t _pad;
	uint64_t value;
};

#define AAC_MAX_KERNEL_ARGS 16

struct aac_ioctl_launch {
	uint64_t code_dev_offset;
	uint32_t entry_offset;
	uint32_t arg_count;
	uint32_t grid_x, grid_y, grid_z;
	uint32_t block_x, block_y, block_z;
	struct aac_ioctl_kernel_arg args[AAC_MAX_KERNEL_ARGS];
};

#define AAC_IOCTL_ALLOC _IOWR(AAC_IOCTL_MAGIC, 1, struct aac_ioctl_alloc)
#define AAC_IOCTL_FREE _IOW(AAC_IOCTL_MAGIC, 2, struct aac_ioctl_free)
#define AAC_IOCTL_COPY_H2D _IOW(AAC_IOCTL_MAGIC, 3, struct aac_ioctl_copy_h2d)
#define AAC_IOCTL_COPY_D2H _IOW(AAC_IOCTL_MAGIC, 4, struct aac_ioctl_copy_d2h)
#define AAC_IOCTL_LAUNCH _IOW(AAC_IOCTL_MAGIC, 5, struct aac_ioctl_launch)
#define AAC_IOCTL_QUEUE_SYNC _IO(AAC_IOCTL_MAGIC, 6)

#endif
