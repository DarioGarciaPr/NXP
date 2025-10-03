ifneq ($(KERNELRELEASE),)
    # Esta parte se ejecuta cuando make es invocado por el kernel
    obj-m := nxp_simtemp.o
else
    # Directorio de headers del kernel
    KDIR ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
endif

