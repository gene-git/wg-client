Recent Changes
==============

**8.02**

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

**7.2.0**

* Drop unmaintained *netifaces* module - use our own code for what we need.

**7.0.0**

* Code Reorg
* Switch packaging from hatch to uv
* Add source checks for C-code as well as python
* Testing to confirm all working correctly on python 3.14.2
* C-code (wg-fix-resolve): Use more 'const' pointer to structs to improve security.
* Modern coding standards: PEP-8, PEP-257 and PEP-484 style and type annotations
* Use pyconcurrent module (additional dependency)

  Available on `Github <https://github.com/gene-git/pyconcurrent>`_
  and `AUR <https://aur.archlinux.org/packages/pyconcurrent>`_


