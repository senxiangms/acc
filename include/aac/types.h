#ifndef AAC_TYPES_H
#define AAC_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aac_device aac_device_t;
typedef struct aac_queue aac_queue_t;
typedef struct aac_buffer aac_buffer_t;

typedef enum {
	AAC_OK = 0,
	AAC_ERR_IO = -1,
	AAC_ERR_NOMEM = -2,
	AAC_ERR_INVAL = -3,
	AAC_ERR_DEVICE = -4,
} aac_result_t;

typedef enum {
	AAC_BUFFER_HOST_PINNED = 1 << 0,
	AAC_BUFFER_DEVICE = 1 << 1,
} aac_buffer_flags_t;

typedef enum {
	AAC_ARG_U32 = 1,
	AAC_ARG_U64 = 2,
	AAC_ARG_PTR_DEVICE = 3,
} aac_arg_type_t;

typedef struct {
	aac_arg_type_t type;
	union {
		uint32_t u32;
		uint64_t u64;
	} v;
} aac_kernel_arg_t;

#ifdef __cplusplus
}
#endif

#endif
