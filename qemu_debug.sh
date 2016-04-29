qemu-system-i386 -nographic -m 512 -kernel images/kernel-ia32-pc99   -initrd images/capdl-loader-experimental-image-ia32-pc99 \
-chardev socket,host=127.0.0.1,port=1234,id=gnc0,server,nowait \
-device isa-serial,chardev=gnc0 \
