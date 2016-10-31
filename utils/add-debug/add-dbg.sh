#!/bin/bash
OBJCPY=$1
shift
for var in "$@"
do
    if [ -f "$var" ]; then
        filename=$(basename "$var")
        extension="${filename##*.}"
        if [[ "$extension" == "sweb" ]]; then
            ./add-dbg "$var" "$var.dbg"
            if [ -f "$var.dbg" ]; then
                $OBJCPY --remove-section .swebdbg "$var"
                $OBJCPY --add-section .swebdbg="$var.dbg" --set-section-flags .swebdbg=noload,readonly "$var"
            fi
            rm -f "$var.dbg"
        fi
    fi
done

