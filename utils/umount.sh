#
# umount.sh
# unmounts the SWEB-flat.vmdk image file from linux
#
umount /mnt/sweb/part1
umount /mnt/sweb/part2
losetup -d /dev/loop6
losetup -d /dev/loop5
losetup -d /dev/loop4
