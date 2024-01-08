.. SPDX-License-Identifier: MIT

#########
wg-client 
#########

Overview
========

Linux wireguard client tools make it simple to start and stop wireguard.
Comes with command line tool, *wg-client*, and a convenient GUI tool which
uses it.

This is a companion to the wireguard server config tools `wg-tool`_.

Also offers an option to invoke ssh which creates a remote listening port connected back to local ssh daemon.

This can be useful to facilitate remote ssh back to client computer 
if it's needed.  For example; it can be used to provide access to a git repo
on the client, or for remote backups of laptop, or even for admin to login to client
should the need arise.

There is a command line program (*wg-client*) and for user convenience there is 
a GUI program (*wg-client-gui*) which is available via a *.desktop* file.

The graphical tool invokes the command line tool, and the command line tool does
all the real work. The GUI provides user convenience.

Why I made wg-client
====================

After building `wg-tool`_ which simplified administering wireguard servers, I needed
a simple way for non-tech users to connect their laptops to the server. 

Thus wg-client was born.  The gui client makes it simple for non-tech users, 
though I find it convenient too. 

.. _`wg-tool`: https://github.com/gene-git/wg_tool

Key features
============

 * Graphical tool makes it simple for any user to get VPN running.
 * Standalone tool makes it easy to test and also keeps sudo outside of gui to minimize any 
   security implications. The gui relies on command line to do the real work.


New or Interesting
==================

  * wg-client
  * wg-client-gui
  * Sleep/Resume DNS fixup - restore correct /etc/resolv.conf, see `sleep_resume`_.

###############
Getting Started
###############

wg-client application
=====================

Usage
-----

To use run from a terminal. For example to start wireguard from a terminal, use:

.. code-block::bash

   wg-client --wg-up

To get a list of options run it with *-h*. Options are also documented in 
config section `config-sect`_ below.

.. _sleep_resume:

laptop sleep / resume
----------------------

WHen laptop sleeps, from lid close for example, and then is woken up - the vpn will continue working 
as normal and likewise the ssh provided the sleep time is not *too long*. However, on wake the
networking is typically re-initialized and part of that may install the dns */etc/resolve.conf*.

When wg-client starts the vpn, it saves the current */etc/resolv.conf* and installs one that
uses the vpn tunnel and this is what gets broken on resume. 

The simple fix is simply to click *Vpn Start* on GUI or equivalently

.. code-block:: bash

   wg-client --fix-dns

This will check what needs to be done and do it. Could be do nothing, or could start up wireguard
or in the scenario here, it could just restore the correct resolv.conf file for using the VPN.

.. _config-sect:

Configuration
-------------

wg-client reads its configuration from 

.. code-block::bash

   /etc/wg-client/config

Please copy the sample config and edit appropriately. The format is in *TOML* format.
This config file provides:

  * iface - required

    Wireguard interface; defaults to *wgc*. It is *<iface>* of */etc/wireguard/<iface>.conf*

  * ssh_server - optional

    Hostname of the remote ssh server accessible over the vpn;   
    this is where the ssh listening port is run.
    Hostname must be accessible over the wg vpn.

  * ssh_pfx - used with ssh_server

    1 or 2 digit number to be used as ssh listening port number prefix.
    The port number is of the form PPxxx, with *PP* the prefix and
    *xxx* is taken from the last octet of the wireguard vpn internal IP address.

    The prefix can also be given as a range of numbers (*'n-m'*). 
    In this case the prefix used is randomly chosen from that range

The port number chosen will be written to the log file.

The remote ssh host will then listen on *127.0.0.1:<port>*.
It will also listen on *<remote-ip-address>:<port>*
provided the remote ssh server permits it by having the sshd option set: 

.. code-block::bash

    GatewayPorts yes

.. wg-client-opts:

Options
-------

Summary of available options for wg-client.

   * Positional argument : Optional  

     wireguard client interface name.   
     Default taken from 'iface' in config file.
     The config is looked for first in *./etc/wg-client/config* (for development purposes)
     and then in */etc/wg-client/config*.  If not found the wg interface defaults to *wgc*

* Options:

   * (*-h, --help*)

     Show this help message and exit

   * (*--wg-up*) and (*--wg-dn*)  

     Start and stop wireguard client

   * (*--ssh-start*) 

     ssh to remote server over vpn and listen on remote port.
     Port number used is described above in Overview section `config-sect`_.

   * (*--ssh-stop*)

     End ssh to remote server

   * (*--ssh-pfx*)

     Set the ssh port prefix. Can be 2 digits: "nn" or a range "nn-mm". If using a range, then
     prefix will be randomly drawn from the range

   * (*--fix-dns*)

     Restore wireguard dns resolv.conf. Typical use is after sleep resume when the network
     is set up it can mess up the resolv.conf file - this restored the correct version.
     
     This will also be done by GUI, if needed, by simply clicking the Start VPN button.

   * (*--show-iface*)  

     Report wireguard interface name is used.

   * (*--show-ssh-server*)  

     Report the ssh server name

   * (*--show-ssh-running*)  

     Report if ssh is active

   * (*--show-wg-running*)

     Report if wireguard is active

   * (*--show-info*)

     Show all info

   * (*--test-mode*)

      Test mode - print what would be done rather than doing it.

wg-client-gui application
=========================

GUI Usage
---------

The gui is installable using the provided wg-client.desktop file and can be added
to launchers in the usual way. For example in gnome simply search applications for wg-cliient
and right click to pin the launcher. The gui ised PyQt6 which in turn relies on Qt6.

The gui has buttons to start and stop wireguard and a button to run ssh to set up the listener 
host configured in the config file.

The gui should be left running while the vpn is in use. Pressing quit the gui will shutdown wireguard
and shutdown the ssh listener as well.

GUI Options
-----------

wg-client-gui has no command line options. It invokes *wg-client*, and thus the configuration
described above `config-sect`_ is used:

.. code-block:: bash

   /etc/wg-client/config

Log files
=========

Each application has it's own log file. These are located in users 
home directory : 

.. code-block::bash

    ${HOME}/log/wg-client
    ${HOME}/log/wg-client-gui

Each of the log files are rotated with companion log suffixed with *.1*

Sudoers
=======
  
wg-client uses *wg-quick* from wireguard tools to start and stop the vpn.
and since this requires root to do it's job, any non-root user will 
need a NOPASSWD sudoers entry. 

You can keep all local sudoers in a single file or in separate files.
If in single file, make this one come after any group wheel ones.
This is to ensure this one is chosen becuase sudo uses the last
matching entry.

Simply add this sample line replacing USERS whatever user or users are 
permitted. If more than one use comma separated list.

.. code-block::bash

    User_Alias WGUSERS = alice, bob, sally
    WGUSERS   ALL = (root) NOPASSWD: /usr/bin/wg-quick
    WGUSERS   ALL = (root) NOPASSWD: /usr/lib/wg-client/wg-fix-dns
   
If using separete files, then care is need to ensure this entry comes after any
wheel group entries. Where USERS is 1 or more usernames or a group such as
*%wgusers*.

Then, 

.. code-block::bash

    visudu /etc/sudoers.d/100-wireguard
    
Replace *USERS* as above.

visudo enforces the correct permissions which should be '0440'. If permissions
are too loose, sudo will ignore the file.

Why the prefix number?  Because sudo uses the **last** matching entry and
we need to be sure the NOPASSWD wg-quick entry comes after any group wheel lines.

For example if there are 2 files in */etc/sudoers.d* - say wg-quick and wheel,
where the wheel entry requires a password for members of group wheel.

Now if user listed in wg-quick is also a member of *wheel* group, since wg-quick
is first and wheel is second (files are treated in lexical order) the *wheel* one
will prevail and user will be prompted for a password when running *sudo /usr/bin/wg-quick*.
Not what we want. To fix this I use numbers ahead of the sudoers filenames. So in this
example it would be:

.. code-block:: bash

   /etc/sudoers.d/001-wheel
   /etc/sudoers.d/100-wg-client

thereby ensuring that wg-client entries follow the wheel ones.

For convenience this is also noted in the sample file:

.. code-block:: bash

    /etc/wg-client/sudoers.sample

.. code-block:: bash

    chmod -440 /etc/sudoers.d/wg-client



########
Appendix
########

Installation
============

Available on:

 * `Github`_ 
 * `Archlinux AUR`_

On Arch you can build using the PKGBUILD provided in packaging directory or from the AUR package.

To build manually, clone the repo and do:

.. code-block:: bash

    rm -f dist/*
    /usr/bin/python -m build --wheel --no-isolation
    root_dest="/" ./scripts/do-install $root_dest

When running as non-root then set root\_dest a user writable directory


Dependencies
============

* Run Time :

  * python              (3.11 or later)
  * netifaces
  * PyQt6 / Qt6         (for gui)
  * hicolor-icon-theme 
  * psutil              (aka python-psutil)

* Building Package:

  * git
  * hatch (aka python-hatch)
  * wheel (aka python-wheel)
  * build (aka python-build)
  * installer (aka python-installer)
  * rsync

* Optional for building docs:

  * sphinx
  * myst-parser
  * texlive-latexextra  (archlinux packaguing of texlive tools)

Philosophy
==========

We follow the *live at head commit* philosophy. This means we recommend using the
latest commit on git master branch.

This approach is also taken by Google [1]_ [2]_.


License
========

Created by Gene C. and licensed under the terms of the MIT license.

 - SPDX-License-Identifier: MIT
 - SPDX-FileCopyrightText: © 2023-present Gene C <arch@sapience.com>

.. _Github: https://github.com/gene-git/wg-client
.. _Archlinux AUR: https://aur.archlinux.org/packages/wg-client

.. [1] https://github.com/google/googletest  
.. [2] https://abseil.io/about/philosophy#upgrade-support
