obj-m += proc.o
obj-m += modulethree.o
obj-m += procseq.o
modulethree-objs := stop.o start.o parm.o

PWD := $(CURDIR)
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
