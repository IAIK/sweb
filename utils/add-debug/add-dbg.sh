#!/bin/bash
for var in "$@"
do
    if [ -f "$var" ]; then
        filename=$(basename "$var")
        extension="${filename##*.}"
        if [[ "$extension" == "sweb" ]]; then
            ./add-dbg "$var" "$var.dbg"  &> /dev/null 
            if [ -f "$var.dbg" ]; then
                objcopy --remove-section .swebdbg "$var"
                objcopy --add-section .swebdbg="$var.dbg" --set-section-flags .swebdbg=noload,readonly "$var"
            fi
            rm -f "$var.dbg"
        fi
    fi
done

