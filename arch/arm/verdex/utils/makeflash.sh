#!/bin/bash
mkimage -A arm -O linux -T kernel -a 0xA0000000 -e 0xA0001000 -C none -n "SWEB ARM Gumstix Verdex kernel.x" -d kernel.x kernel.img
dd of=flash.img bs=128k count=256 if=/dev/zero
dd of=flash.img bs=128k conv=notrunc if=$1
dd of=flash.img bs=128k conv=notrunc seek=248 if=kernel.img

