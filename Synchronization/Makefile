obj-m += mutex.o
obj-m += atomic.o
obj-m += rwLocks.o
obj-m += spinLocks.o

PWD := $(CURDIR)
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
