# Groupe
- antoine.sole
- thomas.crambert

Welcome to GISTRE Linux !

# Organisation du code

TODO

# Questions

TODO

# Remarques

TODO

# Compiling

# Misc

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- versatile_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-

qemu-system-arm   -M versatilepb   -m 128   -kernel arch/arm/boot/zImage   -dtb arch/arm/boot/dts/versatile-pb.dtb   -initrd rootfs.cpio.gz   -serial stdio   -append "console=ttyAMA0,115200 initrd=/bin/bash"


https://elinux.org/Debugging_by_printing
