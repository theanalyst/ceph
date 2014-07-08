#!/bin/sh -ex

MB=1048576
twoMB=$((2*MB))

rm -r layout_test || true
rm new_layout || true
rm file2_layout || true
rm temp || true

# Assume default data pool called 'data' as it is by teuthology ceph task
data_pool_id=`ceph osd dump | sed -n "s/^pool \([0-9]*\) 'data'.*/\1/p"`

echo "layout.data_pool:     $data_pool_id
layout.object_size:   1048576
layout.stripe_unit:   1048576
layout.stripe_count:  1" > new_layout
echo "layout.data_pool:     $data_pool_id
layout.object_size:   2097152
layout.stripe_unit:   1048576
layout.stripe_count:  2" > file2_layout
echo "layout.data_pool:     $data_pool_id
layout.object_size:   1048576
layout.stripe_unit:   262144
layout.stripe_count:  1" > file4_layout


mkdir layout_test
cephfs layout_test show_layout
cephfs layout_test set_layout -u $MB -c 1 -s $MB -p $data_pool_id
touch layout_test/file1
cephfs layout_test/file1 show_layout > temp
diff new_layout temp || return 1
echo "hello, I'm a file" > layout_test/file1
cephfs layout_test/file1 show_layout > temp
diff new_layout temp || return 1
touch layout_test/file2
cephfs layout_test/file2 show_layout > temp
diff new_layout temp || return 1
cephfs layout_test/file2 set_layout -u $MB -c 2 -s $twoMB -p $data_pool_id
cephfs layout_test/file2 show_layout > temp
diff file2_layout temp || return 1

echo "hello, I'm a file with a custom layout" > layout_test/file2

touch layout_test/file3
cephfs layout_test/file3 show_layout > temp
diff new_layout temp || return 1
sudo ls /sys/kernel/debug/ceph
sudo ls /sys/kernel/debug/ceph/\* || true
sudo bash -c 'ls /sys/kernel/debug/ceph/*' || true
sudo bash -c 'cat /sys/kernel/debug/ceph/*/mdsmap' > temp
ceph osd pool create newpool 2 || true
newpool_id=`ceph osd dump | sed -n "s/^pool \([0-9]*\) 'newpool'.*/\1/p"`

ceph mds add_data_pool $newpool_id || true
sudo bash -c 'cat /sys/kernel/debug/ceph/*/mdsmap' > temp2
while diff -q temp2 temp
do
    echo "waiting for mdsmap to update"
    sleep 1
    sudo bash -c 'cat /sys/kernel/debug/ceph/*/mdsmap' > temp2
done
sudo rm temp temp2

echo "layout.data_pool:     $newpool_id
layout.object_size:   1048576
layout.stripe_unit:   1048576
layout.stripe_count:  1" > file3_layout

cephfs layout_test/file3 set_layout -p $newpool_id
cephfs layout_test/file3 show_layout > temp
diff file3_layout temp || return 1
echo "hello, I'm a file in pool3" > layout_test/file3

touch layout_test/file4
cephfs layout_test/file4 show_layout > temp
diff new_layout temp || return 1
cephfs layout_test/file4 set_layout -p $data_pool_id -u 262144
cephfs layout_test/file4 show_layout > temp
diff file4_layout temp || return 1
echo "hello, I'm a file with a small stripe unit!" > layout_test/file3

sync
echo "OK"
