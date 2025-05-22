#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/device-mapper.h>


/* Constructor */
static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv) {
    printk("\n Constructor is started \n");

    
    return 0;
}

/* Destructor*/
static int dmp_map(struct dm_target *ti, struct bio *bio) {
    printk("\n Destructor is started \n");
    return 0;
}

/* Map */
static void dmp_dtr(struct dm_target *ti) {
    printk("\n Map is started \n");
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
