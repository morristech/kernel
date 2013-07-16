#!/system/bin/sh
# root installation helper by GM
mount -o remount,rw /data
rm /data/.siyah/install-root > /dev/null 2>&1
exit
(
while : ; do
	# keep this running until we have root
	if [ -e /data/.siyah/install-root ] ; then
		rm /data/.siyah/install-root
		system/bin/sh /sbin/ext/install.sh
		exit 0
	fi
	if [ -e /system/xbin/su ] ; then
		exit 0
	fi
	sleep 5
done
) &

