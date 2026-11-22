.. SPDX-License-Identifier: GPL-2.0-or-later


========
Appendix
========

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

