#!/usr/bin/env sh

FLASH_IMAGE=flash.img
KERNEL_BIN=kernel.x
KERNEL_IMAGE=kernel.img

mkimage -A arm -O linux -T kernel -a 0xA0000000 -e 0xA0001000 -C none -n "SWEB ARM Gumstix Verdex kernel.x" -d ${KERNEL_BIN} ${KERNEL_IMAGE}
if [ "$?" != "0" ]; then
  echo "Consider installing u-boot-tools"
  exit 1
fi

if [ -n "$(find "${KERNEL_IMAGE}" -prune -size +1000000c)" ]; then
    echo "ERROR: Kernel image is too large for arm verdex flash (max 1Mib)"
    exit 1
fi

dd of=${FLASH_IMAGE} bs=128k count=256 if=/dev/zero
dd of=${FLASH_IMAGE} bs=128k conv=notrunc if=$1
dd of=${FLASH_IMAGE} bs=128k conv=notrunc seek=248 if=${KERNEL_IMAGE}
