Recent Changes
==============

**8.0.4**

* Change Arch package dependencies that have been renamed:

  - pyconcurrent -> python-pyconcurrent

**8.0.3**

* Typo in Readme 

**8.0.2**

* Add missing dependency no PyCidr package used by the ssh listener 

**8.0.0**

* Package / build management now uses meson/mesonpy
* New resolv-manager:
  - wirtten in C.
  - Runs as a background daemon managing /etc/resolv.conf
  - replaces the python inotify based monitor
  - see man page wg-client-resolv-manager
* You need to make a change.
  Please update PostUp / PostDown in wireguard config and
  use the helper scripts installed in /etc/wg-client:
  post-up.sh and post-down.sh. 
  Create /etc/wg-client/wireguirs.resolv.conf with the DNS settings
  you want to use while VPN tunnel is up. Standard resolv.conf format
  (nameserver x.x.x.x) 

