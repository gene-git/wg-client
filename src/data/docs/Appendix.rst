.. SPDX-License-Identifier: GPL-2.0-or-later


========
Appendix
========

Installation
============

Available on:

* `Github`_ 
* `Archlinux AUR`_

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
* PyQt6 / Qt6         (for gui)
* hicolor-icon-theme
* bash
* glibc

**Building Package**:

* git
* meson
* meson-python
* uv
* rsync

**Optional for building docs**:
* sphinx
* texlive-latexextra  (archlinux packaguing of texlive tools)

Log files
=========

Each application has it's own log file. These are located in users
home directory :

.. code-block:: bash

    ${HOME}/log/wg-client
    ${HOME}/log/wg-client-gui

Each of the log files are rotated with companion log suffixed with *.1*

Philosophy
==========

We follow the *live at head commit* philosophy as recommended by
Google's Abseil team [1]_.  This means we recommend using the
latest commit on git master branch. 

License
========

Created by Gene C. and licensed under the terms of the GPL-2.0-or-later license.

- SPDX-License-Identifier: GPL-2.0-or-later
- SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>

.. _Github: https://github.com/gene-git/wg-client
.. _Archlinux AUR: https://aur.archlinux.org/packages/wg-client

.. [1] https://abseil.io/about/philosophy#upgrade-support
