obj-m = aaa_kernel.o

all:
	${MAKE} -C ${KERNEL_SRC} M=$(PWD) modules
	# make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	${MAKE} -C ${KERNEL_SRC} M=$(PWD) clean

modules_install:
	${MAKE} -C ${KERNEL_SRC} M=$(PWD) modules_install

test:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
