# Enable targetcli debug logging
set -ex
zypper --non-interactive --no-gpg-checks install \
	    --force --no-recommends targetcli
targetcli / set global loglevel_file=debug
