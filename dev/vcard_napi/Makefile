obj-m := vcard.o
KDIR := /lib/modules/$(shell uname -r)/build/

vcard:
	make -C $(KDIR) M=$(PWD) modules
	gcc -o capture capture.c

clean:
	rm -rf *.o *.ko *.order *.symvers capture .*.cmd *.mod.c .*versions
