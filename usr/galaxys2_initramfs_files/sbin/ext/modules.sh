#!/system/bin/sh

mount -t tmpfs tmpfs /system/lib/modules
ln -s /lib/modules/* /system/lib/modules
mount -o remount,ro /system
