
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/export.h>
#include <linux/module.h>

#include <linux/miscdevice.h>
#include <linux/of_address.h>
#include <linux/dma-buf.h>
#include <linux/dma-resv.h>

#include "puwrap.h"

#ifdef CONFIG_RTK_RESERVE_MEMORY
#define USE_ION_ALLOCATOR
#ifdef USE_ION_ALLOCATOR
#define NUM_ION_STRUCT 256

#if defined(CONFIG_ION_REALTEK)
#include "mem_allocator/ion.h"
#include <soc/realtek/uapi/ion_rtk.h>
#else
#include "../../../../staging/android/ion/ion.h"
#include "../../../../staging/android/uapi/ion_rtk.h"
#endif

#include <ion_rtk_alloc.h>
#define ion_alloc ext_rtk_ion_alloc

struct rtkdrv_ion_buffer_t {
    unsigned int size;
    unsigned int inUse;
    unsigned long phys_addr;
    unsigned long dmabuf;
    unsigned long pIonHandle;
};

struct rtkdrv_ion_buffer_t pu_ion_buffer[NUM_ION_STRUCT];
static DEFINE_MUTEX(s_pu_mutex);
#else /* else USE_ION_ALLOCATOR */
typedef struct rtkdrv_buffer_t {
    unsigned int size;
    unsigned long phys_addr;
    unsigned long base;							/* kernel logical address in use kernel */
    unsigned long virt_addr;				/* virtual user space address */
} rtkdrv_buffer_t;

#define RTK_INIT_VIDEO_MEMORY_SIZE_IN_BYTE (16*1024*1024)
#define RTK_DRAM_PHYSICAL_BASE 0x06000000
#include "rtk_mm.h"
static video_mm_t s_mem;
static rtkdrv_buffer_t s_memory = {0};
#endif /* endif USE_ION_ALLOCATOR */
#define VE_SECURE_NORMAL 1
#define VE_SECURE_PROTECTION 2
#endif /* endif CONFIG_RTK_RESERVE_MEMORY */

typedef struct rtkdrv_dovi_flag_t{
	int nCoreIdx;
	int nInstIdx;
	unsigned int bEnable;
} rtkdrv_dovi_flag_t;

rtkdrv_dovi_flag_t dovi_flag = {-1, -1, 0};
static DEFINE_MUTEX(s_dovi_mutex);

#define PU_PLATFORM_DEVICE_NAME "rtk_puwrap"

#ifdef CONFIG_RTK_RESERVE_MEMORY

phys_addr_t get_mem_pa(struct ion_buffer *buffer)
{
	unsigned long ret = -1UL;
	struct sg_table *table;
	struct page *page;
	phys_addr_t paddr;

	mutex_lock(&buffer->lock);
	table = buffer->sg_table;
	page = sg_page(table->sgl);
	paddr = PFN_PHYS(page_to_pfn(page));
	mutex_unlock(&buffer->lock);

	ret = paddr;

	return ret;
}

int pu_alloc_dma_buffer(unsigned int size, unsigned long *phys_addr, unsigned long *base, unsigned int mem_type)
{

#ifdef USE_ION_ALLOCATOR
    int ret = -1;
    if (!size)
        return -1;

    mutex_lock(&s_pu_mutex);
    do {
        int i;
        struct dma_buf *dmabuf;
        phys_addr_t dat;
        size_t len;
        int heap = RTK_ION_HEAP_MEDIA_MASK;
        int flags = (ION_FLAG_SCPUACC | ION_FLAG_HWIPACC | ION_FLAG_NONCACHED);

        for (i = 0; i < NUM_ION_STRUCT; i++)
        {
            if (pu_ion_buffer[i].inUse == 0)
                break;
        }
        if (i == NUM_ION_STRUCT)
        {
            printk(KERN_ERR "[PUWRAP] NUM_ION_STRUCT is too small!!!");
            break;
        }

        switch(mem_type)
        {
            case VE_SECURE_NORMAL:
                {
                    heap = RTK_ION_HEAP_MEDIA_MASK;
                    flags = (ION_FLAG_SCPUACC | ION_FLAG_HWIPACC | ION_FLAG_NONCACHED | ION_FLAG_VE_SPEC);
                    break;
                }
            case VE_SECURE_PROTECTION:
                {
                    heap = RTK_ION_HEAP_SECURE_MASK | RTK_ION_HEAP_MEDIA_MASK;
                    flags = (ION_FLAG_HWIPACC | ION_FLAG_PROTECTED_V2_VIDEO_POOL);
                    break;
                }
            default:
                {
                    heap = RTK_ION_HEAP_MEDIA_MASK;
                    flags = (ION_FLAG_SCPUACC | ION_FLAG_HWIPACC | ION_FLAG_NONCACHED);
                    break;
                }
        }

        int handle = ion_alloc(size, heap, flags);
        if (IS_ERR(handle))
        {
            printk(KERN_ERR "[PUWRAP] ion_alloc allocation error size=%d\n", size);
            break;
        }

	dmabuf = dma_buf_get(handle);
	if (IS_ERR(dmabuf)) {
		break;
	}

	dat = get_mem_pa(dmabuf->priv);

        *base = (unsigned long) dmabuf; // we didn't need base address on user space, but we also need it has value to do detection
        *phys_addr = (unsigned long)dat;
        pu_ion_buffer[i].size = size;
        pu_ion_buffer[i].inUse = 1;
        pu_ion_buffer[i].phys_addr = (unsigned long)*phys_addr;
        pu_ion_buffer[i].dmabuf = (unsigned long)dmabuf;
        pu_ion_buffer[i].pIonHandle = (unsigned long)handle;

	__close_fd(current->files, handle);

        ret = 0;
        break;
    } while (0);
    mutex_unlock(&s_pu_mutex);
    return ret;

#else /* else USE_ION_ALLOCATOR */
    if (!size)
        return -1;

    *phys_addr = (unsigned long)vmem_alloc(&s_mem, size, 0);
    if (*phys_addr  == (unsigned long)-1)
    {
        printk(KERN_ERR "[PUWRAP] Physical memory allocation error size=%d\n", size);
        return -1;
    }

    *base = (unsigned long)(s_memory.base + (*phys_addr - s_memory.phys_addr));
    //printk("[PUWARP] base:0x%lx, phy_addr:0x%lx, size:%d\n", *base, (unsigned long)*phys_addr, size);

    return 0;
#endif /* endif USE_ION_ALLOCATOR */
}

void pu_free_dma_buffer(unsigned long base, unsigned long phys_addr)
{
    if (!base)
        return;

#ifdef USE_ION_ALLOCATOR
    mutex_lock(&s_pu_mutex);
    do {
        int i;
        struct ion_handle *handle;
        struct dma_buf *dmabuf;

        for (i = 0; i < NUM_ION_STRUCT; i++)
        {
            if (pu_ion_buffer[i].phys_addr == phys_addr)
                break;
        }
        if (i == NUM_ION_STRUCT)
        {
            printk(KERN_ERR "[PUWRAP] can't find phys_addr:0x%lx !!!\n", (unsigned long)phys_addr);
            break;
        }

        dmabuf = (struct dma_buf *)pu_ion_buffer[i].dmabuf;
        if (dmabuf != NULL)
        {
            dma_buf_put(dmabuf);
            pu_ion_buffer[i].dmabuf = (unsigned long) NULL;
        }

        handle = (struct ion_handle *)pu_ion_buffer[i].pIonHandle;
        if(handle != NULL)
        {
            //ion_free(pu_client, handle);
        }
        memset(&pu_ion_buffer[i], 0, sizeof(struct rtkdrv_ion_buffer_t));
    } while (0);
    mutex_unlock(&s_pu_mutex);
#else
    vmem_free(&s_mem, (unsigned long)phys_addr, 0);
#endif
}

int pu_mmap_dma_buffer(struct vm_area_struct *vm)
{
#ifdef USE_ION_ALLOCATOR
    u32 i;
    int ret = 0;
    struct dma_buf *dmabuf;
    unsigned long phys_addr = (unsigned long)(vm->vm_pgoff << PAGE_SHIFT);
    mutex_lock(&s_pu_mutex);

    for (i = 0; i < NUM_ION_STRUCT; i++)
    {
        if (pu_ion_buffer[i].phys_addr == phys_addr)
            break;
    }
    if (i == NUM_ION_STRUCT)
    {
        printk(KERN_ERR "[PUWRAP] can't find phys_addr:0x%lx !!!\n", (unsigned long)phys_addr);
        mutex_unlock(&s_pu_mutex);
        return -EINVAL;
    }

    dmabuf = (struct dma_buf *)pu_ion_buffer[i].dmabuf;
    ret = dma_buf_mmap(dmabuf, vm, 0);

    mutex_unlock(&s_pu_mutex);

    return ret;
#else
    return -EINVAL;
#endif
}

unsigned long pu_mmap_kernel_buffer(unsigned long phys_addr, unsigned int size)
{
#ifdef USE_ION_ALLOCATOR
    unsigned long ret = 0;

    if (!size)
        return 0;

    mutex_lock(&s_pu_mutex);
    do {
        int i;
        struct dma_buf *dmabuf;
        void * virt_addr;

        for (i = 0; i < NUM_ION_STRUCT; i++)
        {
            if (pu_ion_buffer[i].phys_addr == phys_addr)
                break;
        }
        if (i == NUM_ION_STRUCT)
        {
            printk(KERN_ERR "[PUWRAP] can't find phys_addr:0x%lx !!!\n", (unsigned long)phys_addr);
            break;
        }

        dmabuf = (struct dma_buf *)pu_ion_buffer[i].dmabuf;
        if(dmabuf != NULL)
        {
            ret = dma_buf_begin_cpu_access(dmabuf, 0);
            if (ret) {
                printk(KERN_ERR "[PUWRAP] dma_buf_begin_cpu_access error ret=%d (size=%d) \n", ret, size);
                break;
            }
            virt_addr = dma_buf_kmap(dmabuf, 0);
            if (IS_ERR_OR_NULL(virt_addr)) {
                printk(KERN_ERR "[PUWRAP] dma_buf_kmap error %p (size=%d) \n", virt_addr, size);
                break;
            }

            ret = (unsigned long)virt_addr;
        }
    } while(0);
    mutex_unlock(&s_pu_mutex);

    return ret;
#endif
}

void pu_unmap_kernel_buffer(unsigned long base, unsigned long phys_addr)
{
    if (!base)
        return;

#ifdef USE_ION_ALLOCATOR
    mutex_lock(&s_pu_mutex);
    do {
        int i;
        struct dma_buf *dmabuf;

        for (i = 0; i < NUM_ION_STRUCT; i++)
        {
            if (pu_ion_buffer[i].phys_addr == phys_addr)
                break;
        }
        if (i == NUM_ION_STRUCT)
        {
            printk(KERN_ERR "[PUWRAP] can't find phys_addr:0x%lx !!!\n", (unsigned long)phys_addr);
            break;
        }

        dmabuf = (struct dma_buf *)pu_ion_buffer[i].dmabuf;
        if(dmabuf != NULL)
        {
            dma_buf_kunmap(dmabuf, 0, (void *)base);
            dma_buf_end_cpu_access(dmabuf, 0);
        }
    } while (0);
    mutex_unlock(&s_pu_mutex);
#endif
}
#endif //#ifdef CONFIG_RTK_RESERVE_MEMORY

int pu_set_dovi_flag(int nCoreIdx, int nInstIdx, unsigned int bEnable)
{
	int ret = 0;
	//pr_info("VE1: dovi_flag[%p, %d, %d, %d], input[%d, %d, %d]\n", &dovi_flag, dovi_flag.nCoreIdx, dovi_flag.nInstIdx, dovi_flag.bEnable, nCoreIdx, nInstIdx, bEnable);

	mutex_lock(&s_dovi_mutex);
	if (bEnable == 1)
	{
		if (dovi_flag.bEnable == 0)
		{
			dovi_flag.bEnable = 1;
			dovi_flag.nCoreIdx = nCoreIdx;
			dovi_flag.nInstIdx = nInstIdx;
			ret = 1;
		}
	}
	else
	{
		if (dovi_flag.bEnable == 1 && dovi_flag.nCoreIdx == nCoreIdx && dovi_flag.nInstIdx == nInstIdx)
		{
			dovi_flag.bEnable = 0;
			dovi_flag.nCoreIdx = -1;
			dovi_flag.nInstIdx = -1;
		}
	}
	mutex_unlock(&s_dovi_mutex);

	return ret;
}

static int pu_probe(struct platform_device *pdev)
{
    struct resource res;
    struct device_node *node;

    printk(KERN_INFO "[PUWRAP] pu_probe\n");

    node = pdev->dev.of_node;

#ifdef CONFIG_RTK_RESERVE_MEMORY
#ifdef USE_ION_ALLOCATOR
    memset(pu_ion_buffer, 0, sizeof(struct rtkdrv_ion_buffer_t)*NUM_ION_STRUCT);
    printk(KERN_INFO "[PUWRAP] success to probe pu wrapper device with USE_ION_ALLOCATOR\n");
#else /* else USE_ION_ALLOCATOR */
    if(pdev)
        res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if(res)
    {
        s_memory.size = res->end - res->start;
        s_memory.phys_addr = res->start;
    }
    else
    {
        s_memory.size = RTK_INIT_VIDEO_MEMORY_SIZE_IN_BYTE;
        s_memory.phys_addr = RTK_DRAM_PHYSICAL_BASE;
    }

    s_memory.base = (unsigned long)ioremap(s_memory.phys_addr, PAGE_ALIGN(s_memory.size));
    if (!s_memory.base)
    {
        printk(KERN_ERR "[PUWRAP] :  fail to remap video memory physical phys_addr=0x%x, base=0x%x, size=%d\n", (int)s_memory.phys_addr, (int)s_memory.base, (int)s_memory.size);
        goto ERROR_PROVE_DEVICE;
    }
    if (vmem_init(&s_mem, s_memory.phys_addr, s_memory.size) < 0)
    {
        printk(KERN_ERR "[PUWRAP] :  fail to init vmem system\n");
        goto ERROR_PROVE_DEVICE;
    }
    printk(KERN_INFO "[PUWRAP] success to probe pu wrapper device with reserved video memory phys_addr=0x%x, base = 0x%x\n", (int) s_memory.phys_addr, (int)s_memory.base);
#endif /* endif USE_ION_ALLOCATOR */
#else /* else CONFIG_RTK_RESERVE_MEMORY */
    printk(KERN_INFO "[PUWRAP] success to probe pu wrapper device\n");
#endif /* endif CONFIG_RTK_RESERVE_MEMORY */


    return 0;


#ifdef CONFIG_RTK_RESERVE_MEMORY
#ifndef USE_ION_ALLOCATOR
ERROR_PROVE_DEVICE:

    return 0;
#endif
#endif
}

static int pu_remove(struct platform_device *pdev)
{
    printk(KERN_INFO "[PUWRAP] pu_remove\n");

#ifdef CONFIG_RTK_RESERVE_MEMORY
#ifdef USE_ION_ALLOCATOR

	/* Todo release ion buffer */

#else /* else USE_ION_ALLOCATOR */
    if (s_memory.base)
    {
        iounmap((void *)s_memory.base);
        s_memory.base = 0;
        vmem_exit(&s_mem);
    }
#endif /* endif USE_ION_ALLOCATOR */
#endif /* endif CONFIG_RTK_RESERVE_MEMORY */

    return 0;
}

#if 0
static struct platform_driver pu_driver = {
    .driver = {
        .name = PU_PLATFORM_DEVICE_NAME,
    },
    .probe = pu_probe,
    .remove = pu_remove,
};
#endif

static ssize_t pu_memsize_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", (int)(get_num_physpages() << PAGE_SHIFT));
}

static struct kobj_attribute pu_memsize_attr =
    __ATTR(pu_memsize, 0644, pu_memsize_show, NULL);

static struct attribute *pu_attrs[] = {
    &pu_memsize_attr.attr,
    NULL,
};

static struct attribute_group rtk_pu_attr_group = {
    .attrs = pu_attrs,
};

static struct kobject *pu_kobj;

static int __init pu_init(void)
{
    int res = 0;

    pu_kobj = kobject_create_and_add("rtk_pu", kernel_kobj);
    if (pu_kobj)
    {
        res = sysfs_create_group(pu_kobj, &rtk_pu_attr_group);
        if (res)
            printk(KERN_WARNING "[PUWRAP] %d: unable to create sysfs entry\n", __LINE__);
    }
    else
        printk(KERN_WARNING "[PUWRAP] %d: unable to create sysfs entry\n", __LINE__);

    printk(KERN_INFO "[PUWRAP] begin pu_init\n");
    //res = platform_driver_register(&pu_driver);
    printk(KERN_INFO "[PUWRAP] end pu_init result=0x%x\n", res);
    return res;
}

static void __exit pu_exit(void)
{
    printk(KERN_INFO "[PUWRAP] pu_exit\n");
    //platform_driver_unregister(&pu_driver);
    return;
}


static const struct of_device_id rtk_pu_dt_match[] = {
    { .compatible = "Realtek,rtd13xx-pu_pll" },
    {}
};
MODULE_DEVICE_TABLE(of, rtk_pu_dt_match);

const struct dev_pm_ops rtk_pu_pmops = {
    .suspend    = NULL,
    .resume     = NULL,
};

static struct platform_driver rtk_pu_driver = {
    .driver		= {
        .name	= "rtk-pu",
        .owner	= THIS_MODULE,
        .of_match_table = rtk_pu_dt_match,
        //.pm	= SDHCI_PLTFM_PMOPS,
        .pm	= &rtk_pu_pmops,
    },
    .probe		= pu_probe,
    .remove		= pu_remove,
};

module_platform_driver(rtk_pu_driver);

MODULE_AUTHOR("RTK Wrapper for JPU & VPU, Inc.");
MODULE_DESCRIPTION("PU WRAPPER linux driver");
MODULE_LICENSE("GPL");

module_init(pu_init);
module_exit(pu_exit);
