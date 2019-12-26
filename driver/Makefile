DESTDIR?=/
KERNELDIR?=/lib/modules/$(shell uname -r)/build
DRIVERDIR?=$(shell pwd)/src
MODULEDIR?=/lib/modules/$(shell uname -r)/kernel/hid

DKMS_NAME?=razer-laptop-driver
DKMS_VER?=0.0.1

MY_CFLAGS += -g -DDEBUG
ccflags-y += ${MY_CFLAGS}
CC += ${MY_CFLAGS}


all: driver

driver:
	@echo "Compiling kernel modules"
	$(MAKE) -C $(KERNELDIR) M=$(DRIVERDIR) modules
debug:
	@echo "Compiling kernel modules (Debug)"
	$(MAKE) -C $(KERNELDIR) M=$(DRIVERDIR) modules
	EXTRA_CFLAGS="$(MY_CFLAGS)"
driver_clean:
	@echo "Cleaning kernel modules"
	$(MAKE) -C $(KERNELDIR) M=$(DRIVERDIR) clean

clean: driver_clean

.PHONY: driver
