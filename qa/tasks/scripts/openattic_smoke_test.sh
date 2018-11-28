# openATTIC smoke test
set -ex
systemctl status --full --lines=0 apache2.service
ss --tcp --numeric state listening
echo "OK" >/dev/null
