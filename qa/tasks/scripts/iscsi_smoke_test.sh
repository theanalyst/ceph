# iSCSI Gateway smoke test
set -x
rpm -q lrbd
lrbd --output
ls -lR /sys/kernel/config/target/
ss --tcp --numeric state listening
echo "See 3260 there?"
set -e
zypper --non-interactive --no-gpg-checks install \
    --force --no-recommends open-iscsi
systemctl start iscsid.service
sleep 5
systemctl --no-pager --full status iscsid.service
iscsiadm -m discovery -t st -p $(hostname)
iscsiadm -m node -L all
sleep 5
ls -l /dev/disk/by-path
ls -l /dev/disk/by-*id
if ( mkfs -t xfs /dev/disk/by-path/*iscsi* ) ; then
    :
else
    dmesg
    false
fi
test -d /mnt
mount /dev/disk/by-path/*iscsi* /mnt
df -h /mnt
echo hubba > /mnt/bubba
test -s /mnt/bubba
umount /mnt
iscsiadm -m node --logout
echo "OK" >/dev/null
