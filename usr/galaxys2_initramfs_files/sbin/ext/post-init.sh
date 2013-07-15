#!/system/bin/sh
# Logging
#/sbin/busybox cp /data/user.log /data/user.log.bak
#/sbin/busybox rm /data/user.log
#exec >>/data/user.log
#exec 2>&1

#!/sbin/busybox sh

chown root.shell /system/etc/gps.conf
chmod 0664 /system/etc/gps.conf

if [ -f /data/bkp.profile ]; then
chown root.system /data/bkp.profile
chmod 0777 /data/bkp.profile
fi

mkdir /data/.siyah
chmod 0777 /data/.siyah

. /res/customconfig/customconfig-helper

ccxmlsum=`md5sum /res/customconfig/customconfig.xml | awk '{print $1}'`
if [ "a${ccxmlsum}" != "a`cat /data/.siyah/.ccxmlsum`" ];
then
  rm -f /data/.siyah/*.profile
  echo ${ccxmlsum} > /data/.siyah/.ccxmlsum
fi
[ ! -f /data/.siyah/default.profile ] && cp /res/customconfig/default.profile /data/.siyah

read_defaults
read_config

#cpu undervolting
echo "${cpu_undervolting}" > /sys/devices/system/cpu/cpu0/cpufreq/vdd_levels

if [ "$logger" == "on" ];then
insmod /lib/modules/logger.ko
fi

# Enable Haveged Entropy Tweaks
if [ "$frandom" == "on" ];then
su
mount -o remount,rw /system
cat /res/haveged > /system/xbin/haveged
chown root.root /system/xbin/haveged
chmod 0755 /system/xbin/haveged
. /sbin/ext/haveged.sh
fi
# Disable Haveged Entropy Tweaks
if [ "$frandom" == "off" ];then
su
mount -o remount,rw /system
rm /system/xbin/haveged
fi

# disable debugging on some modules
if [ "$logger" == "off" ];then
  rm -rf /dev/log
  echo 0 > /sys/module/ump/parameters/ump_debug_level
  echo 0 > /sys/module/mali/parameters/mali_debug_level
  echo 0 > /sys/module/kernel/parameters/initcall_debug
  echo 0 > /sys//module/lowmemorykiller/parameters/debug_level
  echo 0 > /sys/module/earlysuspend/parameters/debug_mask
  echo 0 > /sys/module/alarm/parameters/debug_mask
  echo 0 > /sys/module/alarm_dev/parameters/debug_mask
  echo 0 > /sys/module/binder/parameters/debug_mask
  echo 0 > /sys/module/xt_qtaguid/parameters/debug_mask
fi

# for ntfs automounting
insmod /lib/modules/fuse.ko
mount -o remount,rw /
mkdir -p /mnt/ntfs
chmod 777 /mnt/ntfs
mount -o mode=0777,gid=1000 -t tmpfs tmpfs /mnt/ntfs
mount -o remount,ro /

/sbin/busybox sh /sbin/ext/properties.sh

/sbin/busybox sh /sbin/ext/install.sh

##### Early-init phase tweaks #####
/sbin/busybox sh /sbin/ext/tweaks.sh

/sbin/busybox mount -t rootfs -o remount,ro rootfs

##### EFS Backup #####
(
# make sure that sdcard is mounted
sleep 30
/sbin/busybox sh /sbin/ext/efs-backup.sh
) &

#apply last soundgasm level on boot
/res/uci.sh soundgasm_hp $soundgasm_hp

# apply STweaks defaults
export CONFIG_BOOTING=1
/res/uci.sh apply
export CONFIG_BOOTING=

#usb mode
/res/customconfig/actions/usb-mode ${usb_mode}

if [ "$Boostpulse" == "on" ];then
#install modded powerhal
su
mount -o remount,rw /system
rm /system/lib/hw/power.default.so
cp /res/power.default.so.boostpulse /system/lib/hw/power.default.so
chown root.root /system/lib/hw/power.default.so
chmod 0664 /system/lib/hw/power.default.so
chown root.system /sys/devices/system/cpu/cpufreq/ondemand/boostpulse
chown root.system /sys/devices/system/cpu/cpufreq/ondemand/boostfreq
chmod 0664 /sys/devices/system/cpu/cpufreq/ondemand/boostpulse
chmod 0664 /sys/devices/system/cpu/cpufreq/ondemand/boostfreq
echo "1" > /sys/devices/system/cpu/cpufreq/ondemand/boostpulse
else
#install default powerhal
mount -o remount,rw /system
rm /system/lib/hw/power.default.so
cp /res/power.default.so /system/lib/hw/power.default.so
chown root.root /system/lib/hw/power.default.so
chmod 0664 /system/lib/hw/power.default.so
fi

# Enables JIT compiler for packet filters (needs kernel support)
echo "1" > /proc/sys/net/core/bpf_jit_enable

# 50k sampling rate for ondemand (default is 100k)
echo "50000" > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate

# battery tweaks (thanks @infected_)
if [ "$battery" == "on" ];then
echo "500" > /proc/sys/vm/dirty_expire_centisecs
echo "1000" > /proc/sys/vm/dirty_writeback_centisecs
fi

### Disables Built In Error Reporting
setprop profiler.force_disable_err_rpt 1
setprop profiler.force_disable_ulog 1

# aplly gps configurations

if [ "$gpscontrol" == "on" ];then

chown root.shell /system/etc/gps.conf
chmod 0664 /system/etc/gps.conf
rm /system/etc/gps.conf

if [ "$default" == "on" ];then
su
cp /res/gps.conf/gps.conf.north-america /system/etc/gps.conf
fi

if [ "$africa" == "on" ];then
su
cp /res/gps.conf/gps.conf.africa /system/etc/gps.conf
fi

if [ "$asia" == "on" ];then
su
cp /res/gps.conf/gps.conf.asia /system/etc/gps.conf
fi

if [ "$europe" == "on" ];then
su
cp /res/gps.conf/gps.conf.europe /system/etc/gps.conf
fi

if [ "$oceania" == "on" ];then
su
cp /res/gps.conf/gps.conf.oceania /system/etc/gps.conf
fi

if [ "$samerica" == "on" ];then
su
cp /res/gps.conf/gps.conf.south-america /system/etc/gps.conf
fi
fi

# end gps configurations

# gpu watch
cp /res/gpuwatch /system/bin/gpuwatch
chown root.system /system/bin/gpuwatch
chmod 0755 /system/bin/gpuwatch

cp /res/gpucat /system/bin/gpucat
chown root.system /system/bin/gpucat
chmod 0755 /system/bin/gpucat

mount -o remount,ro /system

##### init scripts #####
/system/bin/sh sh /sbin/ext/run-init-scripts.sh
