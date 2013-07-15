#!/system/bin/sh

insmod -f /lib/modules/frandom.ko 2>/dev/null

rm /dev/random && ln /dev/frandom /dev/random

chmod 444 /dev/urandom
chmod 444 /dev/frandom
chmod 444 /dev/erandom

if pgrep haveged > /dev/null; then
echo Haveged already running
fi

haveged -w 4096
