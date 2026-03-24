/*
 * SPDX-License-Identifier: MIT OR GPL-2.0
 * (Kernel module must declare a GPL-compatible MODULE_LICENSE to use
 *  class_create/device_create et al.; they are EXPORT_SYMBOL_GPL.)
 */
/*
 * Minimal AAC character driver: simulated device RAM + ioctl ABI.
 * Replace dma_alloc_coherent / MMIO / doorbells with real hardware hooks.
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include <aac/ioctl.h>

#define AAC_DEVICE_NAME "aac"
#define AAC_CLASS_NAME "aac"
#define AAC_MINOR_COUNT 1

MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("AAC Ascend-like accelerator stub driver");
MODULE_AUTHOR("AAC");

struct aac_mem_block {
	struct list_head list;
	u64 id;
	size_t size;
	u8 *ptr;
};

struct aac_device_state {
	struct mutex lock;
	struct list_head blocks;
	u64 next_id;
};

static struct aac_device_state g_state;
static dev_t g_dev;
static struct cdev g_cdev;
static struct class *g_class;
static struct device *g_device;

static struct aac_mem_block *aac_find_block(u64 dev_offset)
{
	struct aac_mem_block *b;
	list_for_each_entry(b, &g_state.blocks, list) {
		if (b->id == dev_offset)
			return b;
	}
	return NULL;
}

static long aac_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;

	mutex_lock(&g_state.lock);

	switch (cmd) {
	case AAC_IOCTL_ALLOC: {
		struct aac_ioctl_alloc a;
		struct aac_mem_block *b;

		if (copy_from_user(&a, uarg, sizeof(a))) {
			mutex_unlock(&g_state.lock);
			return -EFAULT;
		}
		if (a.size == 0 || a.size > (SIZE_MAX / 2)) {
			mutex_unlock(&g_state.lock);
			return -EINVAL;
		}
		b = kzalloc(sizeof(*b), GFP_KERNEL);
		if (!b) {
			mutex_unlock(&g_state.lock);
			return -ENOMEM;
		}
		b->ptr = kvmalloc(a.size, GFP_KERNEL);
		if (!b->ptr) {
			kfree(b);
			mutex_unlock(&g_state.lock);
			return -ENOMEM;
		}
		b->size = a.size;
		b->id = ++g_state.next_id;
		list_add_tail(&b->list, &g_state.blocks);
		a.dev_offset_out = b->id;
		mutex_unlock(&g_state.lock);
		if (copy_to_user(uarg, &a, sizeof(a)))
			return -EFAULT;
		return 0;
	}
	case AAC_IOCTL_FREE: {
		struct aac_ioctl_free f;
		struct aac_mem_block *b;

		if (copy_from_user(&f, uarg, sizeof(f))) {
			mutex_unlock(&g_state.lock);
			return -EFAULT;
		}
		b = aac_find_block(f.dev_offset);
		if (!b) {
			mutex_unlock(&g_state.lock);
			return -EINVAL;
		}
		list_del(&b->list);
		mutex_unlock(&g_state.lock);
		kvfree(b->ptr);
		kfree(b);
		return 0;
	}
	case AAC_IOCTL_COPY_H2D: {
		struct aac_ioctl_copy_h2d c;
		struct aac_mem_block *b;

		if (copy_from_user(&c, uarg, sizeof(c))) {
			mutex_unlock(&g_state.lock);
			return -EFAULT;
		}
		b = aac_find_block(c.dev_offset);
		if (!b || c.size > b->size) {
			mutex_unlock(&g_state.lock);
			return -EINVAL;
		}
		mutex_unlock(&g_state.lock);
		if (copy_from_user(b->ptr, (void __user *)(uintptr_t)c.host_user_ptr,
				   c.size))
			return -EFAULT;
		return 0;
	}
	case AAC_IOCTL_COPY_D2H: {
		struct aac_ioctl_copy_d2h c;
		struct aac_mem_block *b;

		if (copy_from_user(&c, uarg, sizeof(c))) {
			mutex_unlock(&g_state.lock);
			return -EFAULT;
		}
		b = aac_find_block(c.dev_offset);
		if (!b || c.size > b->size) {
			mutex_unlock(&g_state.lock);
			return -EINVAL;
		}
		mutex_unlock(&g_state.lock);
		if (copy_to_user((void __user *)(uintptr_t)c.host_user_ptr, b->ptr,
				 c.size))
			return -EFAULT;
		return 0;
	}
	case AAC_IOCTL_LAUNCH: {
		struct aac_ioctl_launch l;

		if (copy_from_user(&l, uarg, sizeof(l))) {
			mutex_unlock(&g_state.lock);
			return -EFAULT;
		}
		if (l.arg_count > AAC_MAX_KERNEL_ARGS) {
			mutex_unlock(&g_state.lock);
			return -EINVAL;
		}
		/* Stub: validate code blob exists if offset non-zero */
		if (l.code_dev_offset) {
			if (!aac_find_block(l.code_dev_offset)) {
				mutex_unlock(&g_state.lock);
				return -EINVAL;
			}
		}
		mutex_unlock(&g_state.lock);
		pr_debug("aac: launch entry=%u grid=(%u,%u,%u) block=(%u,%u,%u) args=%u\n",
			 l.entry_offset, l.grid_x, l.grid_y, l.grid_z, l.block_x,
			 l.block_y, l.block_z, l.arg_count);
		return 0;
	}
	case AAC_IOCTL_QUEUE_SYNC:
		mutex_unlock(&g_state.lock);
		return 0;
	default:
		mutex_unlock(&g_state.lock);
		return -ENOTTY;
	}
}

static int aac_open(struct inode *ino, struct file *f)
{
	return 0;
}

static int aac_release(struct inode *ino, struct file *f)
{
	return 0;
}

static const struct file_operations aac_fops = {
	.owner = THIS_MODULE,
	.open = aac_open,
	.release = aac_release,
	.unlocked_ioctl = aac_ioctl,
	.llseek = no_llseek,
};

static int __init aac_init(void)
{
	int ret;

	INIT_LIST_HEAD(&g_state.blocks);
	mutex_init(&g_state.lock);
	g_state.next_id = 0;

	ret = alloc_chrdev_region(&g_dev, 0, AAC_MINOR_COUNT, AAC_DEVICE_NAME);
	if (ret)
		return ret;

	cdev_init(&g_cdev, &aac_fops);
	g_cdev.owner = THIS_MODULE;
	ret = cdev_add(&g_cdev, g_dev, AAC_MINOR_COUNT);
	if (ret)
		goto err_cdev;

	/* Linux 6.4+: class_create(name) only; older kernels used class_create(THIS_MODULE, name). */
	g_class = class_create(AAC_CLASS_NAME);
	if (IS_ERR(g_class)) {
		ret = PTR_ERR(g_class);
		goto err_class;
	}

	g_device = device_create(g_class, NULL, g_dev, NULL, AAC_DEVICE_NAME "%d",
				 MINOR(g_dev));
	if (IS_ERR(g_device)) {
		ret = PTR_ERR(g_device);
		goto err_dev;
	}

	pr_info("aac: loaded major=%u minor=%u (device /dev/%s0)\n", MAJOR(g_dev),
		MINOR(g_dev), AAC_DEVICE_NAME);
	return 0;

err_dev:
	class_destroy(g_class);
err_class:
	cdev_del(&g_cdev);
err_cdev:
	unregister_chrdev_region(g_dev, AAC_MINOR_COUNT);
	return ret;
}

static void __exit aac_exit(void)
{
	struct aac_mem_block *b, *tmp;

	mutex_lock(&g_state.lock);
	list_for_each_entry_safe(b, tmp, &g_state.blocks, list) {
		list_del(&b->list);
		kvfree(b->ptr);
		kfree(b);
	}
	mutex_unlock(&g_state.lock);

	device_destroy(g_class, g_dev);
	class_destroy(g_class);
	cdev_del(&g_cdev);
	unregister_chrdev_region(g_dev, AAC_MINOR_COUNT);
	pr_info("aac: unloaded\n");
}

module_init(aac_init);
module_exit(aac_exit);
