obj-m := vcard.o vcard_pkt_receiver.o
KDIR := /lib/modules/$(shell uname -r)/build/

vcard:
	make -C $(KDIR) M=$(PWD) modules

clean:
	rm -f *.o *.ko *.order *.symvers