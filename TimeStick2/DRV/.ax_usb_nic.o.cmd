savedcmd_ax_usb_nic.o := ld -m elf_x86_64 -z noexecstack --no-warn-rwx-segments   -r -o ax_usb_nic.o @ax_usb_nic.mod  ; /usr/lib/modules/6.16.8-arch3-1/build/tools/objtool/objtool --hacks=jump_label --hacks=noinstr --hacks=skylake --ibt --orc --retpoline --rethunk --sls --static-call --uaccess --prefix=16  --link  --module ax_usb_nic.o

ax_usb_nic.o: $(wildcard /usr/lib/modules/6.16.8-arch3-1/build/tools/objtool/objtool)
