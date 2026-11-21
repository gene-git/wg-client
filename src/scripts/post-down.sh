#!/usr/bin/bash
#
# /etc/wg-client/post-down.sh
# 
# Example :
#   PostDown = /etc/wg-client/post-down.sh
#
# wg-quick removes the interface before running PostDown, so the monitor
# (started by post-up.sh) has normally already restored the standard
# resolv.conf and exited. This is a safety net (e.g. monitor was killed).
#
exec /usr/lib/wg-client/resolv-manager -t standard -l /var/log/wg-client
