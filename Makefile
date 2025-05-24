CURRENT:=$(shell uname -r)
KDIR:=/lib/modules/$(CURRENT)/build
PWD:=$(shell pwd)
DEST:=/lib/modules/$(CURRENT)/misc

TARGET:=dmp
obj-m:=$(TARGET).o

.PHONY: default clean
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	@rm -f *.o *.symvers *.ko *.mod *.order *.mod.*
