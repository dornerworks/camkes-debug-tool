qemu-system-i386 -nographic  -m 512 -kernel images/kernel-ia32-pc99   -initrd images/capdl-loader-experimental-image-ia32-pc99 \
-serial telnet:127.0.0.1:1234,server,nowait
