obj-m := vcard.o
KDIR := /lib/modules/$(shell uname -r)/build/

vcard:
	make -C $(KDIR) M=$(PWD) modules
	gcc -o sender sender.c

clean:
	rm -rf *.o *.ko *.order *.symvers sender .*.cmd *.mod.c .*versions
