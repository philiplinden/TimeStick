savedcmd_ax_usb_nic.mod := printf '%s\n'   ax_main.o ax88179_178a.o ax88179a_772d.o ax_ptp.o | awk '!x[$$0]++ { print("./"$$0) }' > ax_usb_nic.mod
