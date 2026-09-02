#!/usr/bin/bash
#
# /etc/wg-client/post-down.sh
# 
# Example : 
#   PostDown = /etc/wg-client/post-down.sh
#
exec /usr/lib/wg-client/resolv-manager -t standard -l /var/log/wg-client
