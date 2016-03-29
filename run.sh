qemu-system-i386 -nographic  -m 512 -kernel images/kernel-ia32-pc99   -initrd images/capdl-loader-experimental-image-ia32-pc99 \
#-serial telnet:127.0.0.1:1234,server,nowait \

#-net nic \
#-net tap,ifname=tap1,script=qemu_br0_ifup.sh,downscript=qemu_br0_ifdown.sh \
#-chardev socket,host=192.168.2.1,port=4555,id=gnc0 \
#-device isa-serial,chardev=gnc0 \
#-monitor stdio [-nographic]
#-chardev tty,id=pts4,path=/dev/pts/4 -device isa-serial,chardev=pts4
#-net nic -net,tapmifname=tap1,script=qemu_br0_ifup.sh,downscript=qemu.br0.ifdown.sh \
#-chardev socket,host=192.168.1.1,port=4555,id=gnc0
#qemu-system-arm -M kzm -nographic -kernel images/capdl-loader-experimental-image-arm-imx31
