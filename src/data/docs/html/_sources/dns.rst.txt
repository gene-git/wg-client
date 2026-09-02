
.. _managing_dns:

Managing DNS
============

When using a VPN in any non-trusted environment it is standard practice to use DNS servers via
the VPN tunnel. The host the VPN server is running on should have access to trusted DNS servers.
It is these servers the VPN client should be using. These DNS servers have ``IP`` addresses
on a network accessible to to the VPN server being used.

Let's go over an example. If the VPN endpoint the client is using offers 2 DNS servers 
on ``IP`` addresses *10.10.10.100* and *10.20.20.100* then on the wireguard client machine
create a *resolv.conf* file using these ``IP`` addresses. Lets put this file in
*/etc/wg-client/wireguard-resolv.conf* with content::

    #
    # Wireguard client resolv.conf
    # Standard format with IP address(es) 
    # and optional search line if so desired.
    nameserver 10.10.10.100
    nameserver 10.20.20.100

In order to have this DNS resolv.conf file used while the VPN is running, we 
add instructions to the wireguard config using PostUp and PostDown.
The goal is once wireguard is up, we want to use the wireguard resolv.conf for DNS
and after wireguard is down, we want to restore the standard resolv.conf.

Lets say the wireguard interface is *wgc*. Then the */etc/wireguard/wgc.conf* file
would be::

    [Interface]
    PrivateKey      = xxx
    Address         = nnn.nnn.nnn.X/32, xxxx:xxxx:xxxx::X/128
    PostUp          = /etc/wg-client/post-up.sh wgc /etc/wg-client/wireguard-resolv.conf
    PostDown        = /etc/wg-client/post-down.sh

    [Peer]
    PublicKey       = xxx
    PresharedKey    = yyy
    AllowedIPs      = 0.0.0.0/0, ::/0
    Endpoint        = vpn.example.com:55001
    PersistentKeepalive = 25

The post-up script requires the first argument to be the wireguard interface (*wgc* in the above example).
By default the wireguard resolv.conf to use is taken from */etc/wc-client/wireguard-resolv.conf*,
but you can also provide the full path name as an optional second argument.

Both *post-up.sh* and *post-down.sh* use the *resaolv-manager* to do the actual work.

With these in place, when wireguard is brought up,  the provided wireguard DNS resolv file 
is installed to /etc/resolv.conf. Before doing that a copy of the current resolv.conf file is saved
so it can be safely restored when needed.

When wireguard is shut down, it restores the standard resolv.conf. 
*resolv-manager* log files are found in */var/log/wg-client/*

When *wg-client* brings wireguard up, the PostUp script installs the appropriate resolv.conf file
and when it is shut down, it restores the standard resolv.conf

In addition, *wg-client* starts a *resolv-manager* dameon in monitor mode. The monitor's job
is to watch the */etc/resolv.conf* file for any changes and if it changes, save that new version
and re-install the wireguard resolv.conf. This will enusre that while the VPN is running DNS requests
continue to use the VPN tunnel. This uses kernel provided *inotify* and is very efficient.
The monitor application sleeps until the kernel wakes it up to let it know a resolv.conf file
has changed.

There are a couple of reasons the resolv.conf file can change. It can happen after a sleep resume cycle,
a DHCP lease renewal or if a new WiFI network is connected. Wireguard uses UDP, so changing networks
does not impact its function, but if the resolv.conf changes it needs to be *fixed*.

Once wireguard goes down, the monitor daemon restores the standard resolv.conf and exits.

We still suggest having the PostDown above, to handle any cases where the VPN is brought up
directly rather than by *wg-client*. For example *wg-quick up wgc* / *wg-quick down wgc*.

