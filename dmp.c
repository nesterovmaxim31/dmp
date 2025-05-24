#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/device-mapper.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>

#define DM_MSG_PREFIX "dmp"

typedef unsigned long long ull;

static struct device_mapper_proxy {
    struct dm_dev* device_mapper_proxy_dev; /* Underlying device */
    sector_t device_mapper_proxy_sector;      /* Starting sector number of
                                               the device */
    ull write_requests; /* Amount of write bio*/
    ull read_requests;  /* Amount of read bio */ 
    ull writen_size;    /* Sum of all written blocks's size */
    ull read_size;      /* Sum of all read blocks's size */

    spinlock_t spinlock; /* Spinlock to synchronize changing in struct */

    struct kobject *kobj_dmp; /* Sysfs kobject */
} dmp;

/* Sysfs function to fill volumes file */
static ssize_t  sysfs_show(struct kobject *kobj, 
                           struct kobj_attribute *attr, char *buf) {
    ull dmp_read_requests, dmp_write_requests, dmp_avg_size_writen, \
        dmp_avg_size_read, dmp_rw_requests, dmp_avg_size_rw;
    
    spin_lock(&dmp.spinlock);
    dmp_read_requests = dmp.read_requests;
    dmp_write_requests = dmp.write_requests;
    dmp_avg_size_read = dmp.read_size / (dmp_read_requests != 0 ? \
                                         dmp_read_requests : 1);
    dmp_avg_size_writen = dmp.writen_size / (dmp_write_requests != 0 ? \
                                             dmp_write_requests : 1);
    dmp_rw_requests = dmp_write_requests + dmp_read_requests;
    dmp_avg_size_rw = (dmp.writen_size + dmp.read_size) / \
        ((dmp_write_requests + dmp_read_requests) != 0 ? \
         (dmp_write_requests + dmp_read_requests) : 1);
    spin_unlock(&dmp.spinlock);
    
    return sprintf(buf, "read:\n\treqs: %llu\n\tavg size: %llu\n \
write:\n\treqs: %llu\n\tavg size: %llu\ntotal:\n\treqs: %llu\n\tavg size: \
%llu\n", dmp_read_requests, dmp_avg_size_read, dmp_write_requests, \
                   dmp_avg_size_writen, dmp_rw_requests, dmp_avg_size_rw);
}
        
struct kobj_attribute dmp_attr = __ATTR(volumes, 0660, sysfs_show, \
                                        NULL);

/* Constructor */
static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv) {
    int _result;

    /* One argument is required - underlying block device name */
    if (argc != 1) {
        ti->error = "No underlying block device name is given";
		return -EINVAL;
	}

    _result = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), \
                            &dmp.device_mapper_proxy_dev);
    if(_result) { /* If dm_get_device return not zero, then handle error */
        ti->error = "Attempt to get device is failed";

    }

    /* Set default values */
    dmp.write_requests = 0;
    dmp.read_requests = 0;
    dmp.writen_size = 0;
    dmp.read_size = 0;

    /* Init spinlock */
    spin_lock_init(&dmp.spinlock);

    return 0;
}

/* Destructor*/
static int dmp_map(struct dm_target *ti, struct bio *bio) {
    switch(bio_op(bio)) {
    case REQ_OP_READ: /* In case of writing */
        spin_lock(&dmp.spinlock);
        dmp.read_requests++;
        dmp.read_size += bio->bi_iter.bi_size;
        spin_unlock(&dmp.spinlock);
        break;
        
    case REQ_OP_WRITE: /* In case of reading */
        spin_lock(&dmp.spinlock);
        dmp.write_requests++;
        dmp.writen_size += bio->bi_iter.bi_size;
        spin_unlock(&dmp.spinlock);
        break;
    default:
		return DM_MAPIO_KILL;
    }

    
    /* Change block device target for bio and redirect */
    bio_set_dev(bio, dmp.device_mapper_proxy_dev->bdev);
    submit_bio(bio);
    
    return DM_MAPIO_SUBMITTED;
}

/* Map */
static void dmp_dtr(struct dm_target *ti) {
    printk("\n Destructor is started \n");
    kfree(ti->private);
}

static struct target_type dmp_target = {
	.name   = "dmp",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.ctr    = dmp_ctr,
    .dtr    = dmp_dtr,
	.map    = dmp_map, 
};

static int __init init_dmp(void) {
    int result;
    result = dm_register_target(&dmp_target);
  
    if(result < 0)
        printk(KERN_CRIT "Error in registering target\n");
    else
        printk("Device is registered\n");

    /* Sysfs */
    /* Create direcotry stat */
    dmp.kobj_dmp = kobject_create_and_add("stat", &THIS_MODULE->mkobj.kobj);
    if (dmp.kobj_dmp == NULL) {
        return -ENOMEM;
    }

    /* Create file volumes */
    result = sysfs_create_file(dmp.kobj_dmp, &dmp_attr.attr);
    if (result) {
        printk(KERN_CRIT "Error in sysfs initialization\n");
    }
  
    return result;
}

static void __exit exit_dmp(void) {
    /* Unregister device */
    dm_unregister_target(&dmp_target);    
}

module_init(init_dmp);
module_exit(exit_dmp);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maksim Nesterov <maxgoonfuture@gmail.com>");
MODULE_DESCRIPTION("Device mapper proxy");
MODULE_VERSION("1.0");
