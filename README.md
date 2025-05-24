# Device Mapper Proxy
Module for Linux kernel, that create virtual block device on top of some block device to keep statistics. 


## Build and Setup
Ensure you have gcc, make, linux-headers, dmsetup package installed.
```bash
$ make -j8
$ sudo insmod dmp.ko
```

## Test
```bash
$ sudo dmsetup create zero1 --table "0 4096 zero"
$ sudo dmsetup create dmp1 --table "0 4096 dmp /dev/mapper/zero1"
$ sudo dd if=/dev/random of=/dev/mapper/dmp1 bs=4k count=1
$ sudo dd of=/dev/null if=/dev/mapper/dmp1 bs=4k count=1
```

## Clean
```bash
$ make clean
```
