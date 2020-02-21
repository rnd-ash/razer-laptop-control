DESTDIR?=/
KERNELDIR?=/lib/modules/$(shell uname -r)/build
DRIVERDIR?=$(shell pwd)/src
MODULEDIR?=/lib/modules/$(shell uname -r)/kernel/hid

DKMS_NAME?=razercontrol
DKMS_VER?=1.0.1

MY_CFLAGS += -g -DDEBUG
ccflags-y += ${MY_CFLAGS}
CC += ${MY_CFLAGS}


all: driver

driver:
	@echo "Compiling kernel modules"
	$(MAKE) -C $(KERNELDIR) M=$(DRIVERDIR) modules
debug:
	@echo "Compiling kernel modules - debug"
	$(MAKE) -C $(KERNELDIR) M=$(DRIVERDIR) modules
	EXTRA_CFLAGS="$(MY_CFLAGS)"
driver_clean:
	@echo "Cleaning kernel modules"
	$(MAKE) -C $(KERNELDIR) M=$(DRIVERDIR) clean

driver_dkms:
	@echo "Installing kernel modules for DKMS"
	install -m 644 -v -D Makefile $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/Makefile
	install -m 644 -v -D dkms.conf $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/dkms.conf
	install -m 755 -v -d src $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/src
	install -m 644 -v -D src/*.c $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/src/
	install -m 644 -v -D src/*.h $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/src/
	install -m 644 -v -D src/Makefile $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/src/
	rm -rf $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/src/*.mod.c


remove_dkms:
	@echo "Removing dkms module"
	rm -rf $(DESTDIR)/usr/src/$(DKMS_NAME)_$(DKMS_VER)

clean: driver_clean

.PHONY: driver
