#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/device-mapper.h>
#include <linux/spinlock.h>


#define DM_MSG_PREFIX "dmp"
DEFINE_SPINLOCK(lock1);

typedef unsigned long long ull;

struct device_mapper_proxy {
    struct dm_dev* device_mapper_proxy_dev; /* Underlying device */
    sector_t device_mapper_proxy_sector;      /* Starting sector number of
                                               the device */
    ull write_requests; /* Amount of write bio*/
    ull read_requests;  /* Amount of read bio */ 
    ull writen_size;    /* Sum of all written blocks's size */
    ull read_size;      /* Sum of all read blocks's size */

    spinlock_t spinlock; /* Spinlock to synchronize changing in struct */ 
};

/* Constructor */
static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv) {
    printk("\n Constructor is started \n");
    int _result;

    ti->private = NULL;

    /* One argument is required - underlying block device name */
    if (argc != 2) {
        ti->error = "No underlying block device name is given";
		return -EINVAL;
	}

    struct device_mapper_proxy* dmp = NULL;
    dmp = kmalloc(sizeof(struct device_mapper_proxy), GFP_KERNEL);
    if(dmp == NULL) {
        printk(KERN_CRIT "\n Unable to allocate memory \n");
        ti->error = "Unable to allocate memory";
        return -ENOMEM;
    }

    _result = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), \
                            &dmp->device_mapper_proxy_dev);
    if(_result) { /* If dm_get_device return not zero, then handle error */
        ti->error = "Attempt to get device is failed";
        kfree(dmp);
        return _result;
    }

    dmp->write_requests = 0;
    dmp->read_requests = 0;
    dmp->writen_size = 0;
    dmp->read_size = 0;
    
    spin_lock_init(&dmp->spinlock);
    
    ti->private = dmp;
    
    return 0;
}

/* Destructor*/
static int dmp_map(struct dm_target *ti, struct bio *bio) {
    printk("\n Map is started \n");
    struct device_mapper_proxy *dmp = \
        (struct device_mapper_proxy *) ti->private;

    switch(bio_op(bio)) {
    case REQ_OP_READ: /* In case of writing */
        spin_lock(&dmp->spinlock);
        dmp->read_requests++;
        dmp->read_size += bio->bi_iter.bi_size;
        spin_unlock(&dmp->spinlock);
        break;
        
    case REQ_OP_WRITE: /* In case of reading */
        spin_lock(&dmp->spinlock);
        dmp->write_requests++;
        dmp->writen_size += bio->bi_iter.bi_size;        
        spin_unlock(&dmp->spinlock);        
        break;
    default:
        break;
    }

    /* Change block device target for bio and redirect */
    bio->bi_bdev = dmp->device_mapper_proxy_dev->bdev;
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
      printk(KERN_CRIT "\n Error in registering target \n");
  else
      printk("\n Device is registered \n");
  
  return 0;
}

static void __exit exit_dmp(void) {
  dm_unregister_target(&dmp_target);
}

module_init(init_dmp);
module_exit(exit_dmp);

MODULE_LICENSE("LGPL");
MODULE_AUTHOR("Maksim Nesterov <maxgoonfuture@gmail.com>");
MODULE_DESCRIPTION("Device mapper proxy");
MODULE_VERSION("1.0");
