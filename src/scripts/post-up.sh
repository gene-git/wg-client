#!/usr/bin/bash
#
# /etc/wg-client/post-up.sh
#
# Optional arguments:
#
#   [iface [dns_file]]
#
# If dns_file is not provided, /etc/wg-client/wireguard-resolv.conf is used.
# If iface is not provided, it is read from the "iface = xxx" line in /etc/wg-client/config
#
# Examples : 
#   PostUp = /etc/wg-client/post-up.sh
#   PostUp = /etc/wg-client/post-up.sh wgc 
#   PostUp = /etc/wg-client/post-up.sh wg0 /etc/wg-client/wireguard-resolv.conf
#

#
# helper - read wg interface from /etc/wg-client/config
#
iface_from_config() {
    iface=$(sed -n "s/^iface *= *['\"]\([^'\"]*\)['\"].*/\1/p" /etc/wg-client/config)
    echo "$iface"
}

#
# interface
#
iface=''
if (( $# < 1 )) ; then
    iface=$(iface_from_config)
    if [[ "$iface" == "" ]] ; then
        echo "Missing wireguard interface" >&2
        exit 1
    fi
else
    iface="$1"
fi

#
# wireguard resolv.conf file
# #
if (( $# > 1 )) ; then
    wg_resolv="$2"
else
    wg_resolv="/etc/wg-client/wireguard-resolv.conf"
fi

if [[ ! -f "$wg_resolv" ]] ; then
    echo "Wireguard resolv file not found: $wg_resolv" >&2
    exit 1
fi

#
# Run resolv-manager (as root - wg-quick runs PostUp as root)
#  1) save the wireguard resolv.conf (tagged) and install it.
#     Any failure here is returned to wg-quick, which then brings the tunnel down.
#  2) start the monitor daemon: keeps the wireguard resolv.conf installed and
#     restores the standard one when the interface goes away (wg-quick down)
#     or on SIGTERM. It drops to nobody keeping only CAP_CHOWN/CAP_DAC_OVERRIDE.
#
rm_bin=/usr/lib/wg-client/resolv-manager
logdir=/var/log/wg-client

"$rm_bin" -i "$iface" -t wireguard -l "$logdir" -f "$wg_resolv" || exit 1
exec "$rm_bin" -i "$iface" -t monitor -d -l "$logdir"
