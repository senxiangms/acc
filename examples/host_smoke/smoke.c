#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aac/runtime.h"

int main(void)
{
	aac_device_t *dev = NULL;
	aac_queue_t *q = NULL;
	uint64_t buf = 0;
	char host_in[] = "hello device";
	char host_out[sizeof(host_in)] = { 0 };

	if (aac_device_open("/dev/aac0", &dev) != AAC_OK) {
		perror("aac_device_open");
		fprintf(stderr, "Load the aac kernel module and create /dev/aac0 first.\n");
		return 1;
	}
	if (aac_queue_create(dev, &q) != AAC_OK) {
		fprintf(stderr, "queue create failed\n");
		return 1;
	}
	if (aac_buffer_alloc(dev, 4096, AAC_BUFFER_DEVICE, &buf) != AAC_OK) {
		fprintf(stderr, "buffer alloc failed\n");
		return 1;
	}

	if (aac_memcpy_h2d(q, host_in, buf, sizeof(host_in)) != AAC_OK) {
		fprintf(stderr, "h2d failed\n");
		return 1;
	}
	if (aac_memcpy_d2h(q, buf, host_out, sizeof(host_out)) != AAC_OK) {
		fprintf(stderr, "d2h failed\n");
		return 1;
	}
	if (aac_sync(q) != AAC_OK) {
		fprintf(stderr, "sync failed\n");
		return 1;
	}

	if (memcmp(host_in, host_out, sizeof(host_in)) != 0) {
		fprintf(stderr, "data mismatch\n");
		return 1;
	}

	aac_launch_desc_t launch = { 0 };
	launch.code_dev_offset = buf;
	launch.entry_offset = 0;
	launch.grid[0] = 1;
	launch.block[0] = 1;
	launch.arg_count = 1;
	launch.args[0].type = AAC_ARG_U64;
	launch.args[0].v.u64 = buf;

	if (aac_launch(q, &launch) != AAC_OK) {
		fprintf(stderr, "launch failed\n");
		return 1;
	}
	if (aac_sync(q) != AAC_OK) {
		fprintf(stderr, "sync after launch failed\n");
		return 1;
	}

	printf("smoke ok: h2d/d2h roundtrip + stub launch\n");

	aac_buffer_free(dev, buf);
	aac_queue_destroy(q);
	aac_device_close(dev);
	return 0;
}
