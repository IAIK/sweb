#
# mount.sh
# mounts the SWEB-flat.vmdk image file to linux
#
mkdir -p /mnt/sweb
mkdir -p /mnt/sweb/part0
mkdir -p /mnt/sweb/part1
mkdir -p /mnt/sweb/part2
mkdir -p /mnt/sweb/part3
losetup /dev/loop4 /tmp/sweb/SWEB-flat.vmdk
losetup -o 32256 /dev/loop5 /dev/loop4
mount /dev/loop5 /mnt/sweb/part1
losetup -o 10321920 /dev/loop6 /dev/loop4
mount /dev/loop6 /mnt/sweb/part2
