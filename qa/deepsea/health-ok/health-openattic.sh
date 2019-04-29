#!/bin/bash
#
# wrapper around health-ok.sh for deploying a Ceph cluster with openATTIC
#
set -e
set +x

SCRIPTNAME=$(basename ${0})
BASEDIR=$(readlink -f "$(dirname ${0})")
test -d $BASEDIR

source $BASEDIR/health-ok.sh --client-nodes=1 --mds --igw --min-nodes=2 --nfs-ganesha --openattic --rgw
