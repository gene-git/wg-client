.. SPDX-License-Identifier: GPL-2.0-or-later


========
Appendix
========

.. _migration:

Migration from Pre 8.x
======================

Older versions (pre 8.x) need a small change becasue *resolv-manager* has been re-written
in C and now runs a background daemon managing /etc/resolv.conf. 

This is what is needed:

* Create /etc/wg-client/wireguard-resolv.conf
  This should be the DNS settings to be used while wireguard is running.
  Standard resolv.conf format (nameserver x.x.x.x)

* confirm that /etc/wg-client/config exists and sets thw wireguard i
  interface. For example::

    iface = wg0

* edit the wireguard client config and update the PostUp and PostDn.
  For example::

    [Interface]
    PrivateKey = ...
    Address = ...
    Postup = /etc/wg-client/post-up.sh
    PostDown = /etc/wg-client/post-down.sh

Thats should be all that's required.


Installation
============

Available on:

* `Github <https://github.com/gene-git/wg-client>`_ 
* `Archlinux AUR <https://aur.archlinux.org/packages/wg-client>`_

On Arch you can build using the PKGBUILD provided in packaging directory or from the AUR package.

To build manually, clone the repo and do:

.. code-block:: bash

    ./scripts/do-build
    ./scripts/do-install [<destination>]

If destination is not specified it defaults to *build/pkg*

Dependencies
============

**Run Time** :

* python              (3.13 or later)
* psutil              (python-psutil)
* dateutil
* libcap
* pynotify            (python-notify)
* openssl
* pyconcurrent
* PySide6 / Qt6        (for gui)
* hicolor-icon-theme
* bash
* glibc

**Building Package**:

* git
* meson
* meson-python
* rsync

Log files
=========

Each application has it's own log file. They are located in the home directory :

.. code-block:: bash

    ${HOME}/log/wg-client
    ${HOME}/log/wg-client-gui

Each log file is rotated with using numerical suffix.

License
========

Created by Gene C. and licensed under the terms of the GPL-2.0-or-later license.

- SPDX-License-Identifier: GPL-2.0-or-later
- SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>

