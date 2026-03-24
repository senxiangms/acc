#ifndef AAC_IOCTL_H
#define AAC_IOCTL_H

/*
 * Shared userspace + kernel ioctl ABI.
 * Kernel builds must not use <stdint.h> (not on kernel's include path).
 */
#ifdef __KERNEL__
#include <linux/types.h>
typedef __u64 aac_u64;
typedef __u32 aac_u32;
#else
#include <stdint.h>
typedef uint64_t aac_u64;
typedef uint32_t aac_u32;
#endif

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define AAC_IOCTL_MAGIC 0xC1

struct aac_ioctl_alloc {
	aac_u64 size;
	aac_u32 flags;
	aac_u32 _pad;
	aac_u64 dev_offset_out;
};

struct aac_ioctl_free {
	aac_u64 dev_offset;
};

struct aac_ioctl_copy_h2d {
	aac_u64 host_user_ptr;
	aac_u64 dev_offset;
	aac_u64 size;
};

struct aac_ioctl_copy_d2h {
	aac_u64 dev_offset;
	aac_u64 host_user_ptr;
	aac_u64 size;
};

struct aac_ioctl_kernel_arg {
	aac_u32 type;
	aac_u32 _pad;
	aac_u64 value;
};

#define AAC_MAX_KERNEL_ARGS 16

struct aac_ioctl_launch {
	aac_u64 code_dev_offset;
	aac_u32 entry_offset;
	aac_u32 arg_count;
	aac_u32 grid_x, grid_y, grid_z;
	aac_u32 block_x, block_y, block_z;
	struct aac_ioctl_kernel_arg args[AAC_MAX_KERNEL_ARGS];
};

#define AAC_IOCTL_ALLOC _IOWR(AAC_IOCTL_MAGIC, 1, struct aac_ioctl_alloc)
#define AAC_IOCTL_FREE _IOW(AAC_IOCTL_MAGIC, 2, struct aac_ioctl_free)
#define AAC_IOCTL_COPY_H2D _IOW(AAC_IOCTL_MAGIC, 3, struct aac_ioctl_copy_h2d)
#define AAC_IOCTL_COPY_D2H _IOW(AAC_IOCTL_MAGIC, 4, struct aac_ioctl_copy_d2h)
#define AAC_IOCTL_LAUNCH _IOW(AAC_IOCTL_MAGIC, 5, struct aac_ioctl_launch)
#define AAC_IOCTL_QUEUE_SYNC _IO(AAC_IOCTL_MAGIC, 6)

#endif
