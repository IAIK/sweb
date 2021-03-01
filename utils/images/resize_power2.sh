#!/usr/bin/env sh

set -e

IMAGE_FILE=${1:?"Usage: $0 <image_file>"}
# IMAGE_FILE=$1
# IMAGE_FILE=${$1:-"SWEB.qcow2"}

SIZE_PRE=$(qemu-img info ${IMAGE_FILE} | sed -n -e 's|virtual size: .* (\(.*\) bytes)|\1|p')
SIZE_POST=$(echo "x=l(${SIZE_PRE})/l(2); scale=0; 2^((x+0.5)/1)" | bc -l)

echo "Resizing $IMAGE_FILE from $SIZE_PRE to $SIZE_POST bytes"

qemu-img resize "${IMAGE_FILE}" "${SIZE_POST}"
