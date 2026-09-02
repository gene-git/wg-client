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
#   PostUp = /etc/wg-client/post-up.sh wg0 /etc/wg-client/wireguard-resolv.confc 
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
        echo "Missing wireguard interface"
        return 1
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
    echo "Wireguard resolv file not found: $wg_resolv"
    exit 1
fi

#
# Run resolv-manager
#
exec /usr/lib/wg-client/resolv-manager -i "$iface" -t wireguard -l /var/log/wg-client -f "$wg_resolv"
