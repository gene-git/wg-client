.. SPDX-License-Identifier: GPL-2.0-or-later

=========
wg-client 
=========

**Migration required from pre 8.x versions**

If you are updating from pre 8.x versions please see the
Appendix to user newer versions :ref:`migration`.

Overview
========

wg-client makes it simple for any user to start and stop a wireguard client on Linux.
The command line program, *wg-client*, provides all the functionality.
A GUI tool makes it super convenient. The GUI tool runs the command line client
and thus offeres exactly the same functionality in a user friendly application.

A companion package, `wg-tool`_ is for adminstering the server side of things.

wg-client also has an option to invoke ssh to create a remote listening port 
connected back to local (client end) ssh daemon.

This can be useful to facilitate remote ssh back to client computer 
if it's needed.  For example; it can be used to provide access to a git repo
on the client, or for remote backups of laptop, or even for admin to login to client
should the need arise.

All git tags are signed with arch@sapience.com key which is available via WKD
or download from https://www.sapience.com/tech. Add the key to your package builder gpg keyring.
The key is included in the Arch package and the source= line with *?signed* at the end can be used
to verify the git tag.  You can also manually verify the signature

Documentation
-------------

The manual provides detailed information about using wg-client.
It is available in HTML and PDF formats installed under */usr/share/wg-client/docs*

It is also available at: `readthedocs <https://wg-client.readthedocs.io>`_.

Why I made wg-client
====================

After building `wg-tool`_ which simplified administraion of wireguard servers, I needed
a simple way for non-tech (unprivileged) users to connect their laptops to the server. 

Thus wg-client was born.  The gui client makes it really simple for non-tech users, 
though I find it pretty convenient as well. 

.. _`wg-tool`: https://github.com/gene-git/wg_tool

Key features
============

* Graphical app makes it simple for any user to get VPN running.
* Standalone tool makes it easy to test and also keeps sudo outside of gui to minimize any 
  security implications. The gui relies completely on the command line tool to do the real work.
* resolv-manager is written on C. It uses capabilities and is designed to be run
  by non-root users. When run as root, it drops privileges to (user, group) =  (*nobody*, *nobody*)..
  Code has been checked and is clean using:
  - clang-tidy
  - valgrind
  - compiler based sanitizers

* python code is checked with mypy

===============
Getting Started
===============

DNS Notes
=========

When using any VPN client, it is standard practice to ensure all 
DNS traffic is routed through the VPN tunnel in order to trusted DNS servers.
While this isn't a hard requirement, and is not strictly applicable with split 
routing, it is always desirable from a security standpoint.

There is a little gotcha to be aware of.
The system may, at times, modify */etc/resolv.conf* removing the safety of the 
VPN provided DNS servers to different ones. This also breaks LAN related services.

Consequently, it is important to be identify if this happens and deal with it.
It is handled is by saving any newly resolv.conf file and then restoring the 
correct one that should be used with wireguard. The resolv.conf that was just 
saved is the the right file to use when the VPN is turned off but not while it
is runninf. By saving a copy, it can be restored when the wireguard VPN shuts down.

What causes this to happen?
It can happen for different reasons. It can happen if the WiFi network changes, after a 
sleep resume cycle or when a DHCP lease is renewed. 

While wireguard itself uses UDP and is mostly indifferent to such network changes, 
nonetheless it is quite important to keep DNS properly managed.

Taking care of all of this is eactly what *resolv-manager* does.

To ensure that DNS resolver files are all managed correctly requires
a copy of the standard resolv.conf file saved to */etc/resolv.conf.saved*
and the wireguard version to */etc/resolv.conf.wg*.  

With those in place *resolv-manager* will keep track of any changes.
It will save any updated standard */etc/rsolv.conf* file and put back 
the appropriate one for wireguard.

After creating /etc/wg-client/wireguard-resolv.conf with the 
the DNS nameservers provided by the wireguard server end, then
set the wireguard configuration file (for example */etc/wireguard/wgc.conf*) 
to use PostUp and PosDown.  


.. code-block:: text

    [Interface]
    PrivateKey = ...
    Address    = ...
    Postup     = /etc/wg-client/post-up.sh
    PostDown   = /etc/wg-client/post-down.sh
    ...

In addition the wg-client config file, */etc/wg-client/config*, must
set the wireguard interface that is used. For example with a line::

    iface = wg0

There helper scripts *post-up.sh* and *post-down.sh* are part of wg-client.
wg-client handles everything else including running resolv-manager to monitor
/etc/resolv.conf while thw vpn is active to ensures that DNS is resolved using
the appropriate DNS servers while wireguard is active as well as restoring the
standard resolv file when wireguard is shut down.

More information about resolv-manager can be found in the man page::

    man wg-client-resolv-manageer

wg-client application
=====================

Usage
-----

wg-client is a command line program and can be run in any terminal.
It has two primary options, one to start wireguard and one to stop it.

.. code-block:: bash

   wg-client --wg-up
   wg-client --dg-dn

It uses a config file (/etc/wg-client/config) where the wireguard interface
is specified along with optional ssh information.

To see the list of available options use *-h*. 

The *wg-client* options are described fully in the manual 
section :ref:`options-sect` and the configuration file in section :ref:`config-sect`.


wg-client-gui application
=========================

GUI Usage
---------

The gui app can be run using the wg-client.desktop file and can be added
to launchers in the usual way. For example in gnome simply search applications for wg-cliient
and right click to pin the launcher. The gui is built on PyQt6 which in relies on Qt6.

The gui has buttons to start and stop wireguard and a button to run ssh to set up the listener 
on the host configured in the config file.

The gui should be left running while the vpn is in use. Pressing quit in the gui will shutdown wireguard
and shutdown the ssh listener as well.


Sudoers
=======
  
To start and stop wireguard wg-client uses *wg-quick* (from wireguard-tools).
Doing so requires root priviliges. As a result, any user approved to run wireguard must 
be granted permission.  Any non-root user will need a NOPASSWD sudoers entry. 

You can keep all local sudoers in a single file or in separate files.
If in single file, make this one come after any group wheel ones.
This is to ensure this one is chosen becuase sudo uses the last
matching entry.

Simply add this sample line adjusting WGUSERS to list whatever user(s) are 
permitted to run wireguard. If more than one use comma separated list as shown below.

.. code-block:: bash

    User_Alias WGUSERS = alice, bob, sally
    WGUSERS   ALL = (root) NOPASSWD: /usr/bin/wg-quick
    WGUSERS   ALL = (root) NOPASSWD: /usr/lib/wg-client/wg-fix-dns
   
If using separete files, then care is need to ensure this entry comes after any
wheel group entries. Where WGUSERS is 1 or more usernames or a group such as
*%wgusers*.

Then, 

.. code-block:: bash

    visudu /etc/sudoers.d/100-wireguard
    
Edit *WGUSERS* as above.

visudo enforces the correct permissions which should be '0440'. If permissions
are too loose, sudo will ignore the file.

Why the prefix number?  Because sudo uses the **last** matching entry and
we need to be sure the NOPASSWD wg-quick entry comes after any group wheel lines.

For example if there are 2 files in */etc/sudoers.d* - say wg-quick and wheel,
where the wheel entry requires a password for members of group wheel.

Now if user listed in wg-quick is also a member of *wheel* group, since wg-quick
is first and wheel is second (files are treated in lexical order) the *wheel* one
will prevail and user will be prompted for a password when running *sudo /usr/bin/wg-quick*.
Not what we want. To fix this use numbers to prefix the sudoers filenames. So in this
example it would be::

   /etc/sudoers.d/010-wheel
   /etc/sudoers.d/100-wg-client

thereby ensuring that wg-client entries follow the wheel ones.

For convenience this is also noted in the sample file::

    /etc/wg-client/samples/sudoers.sample

.. code-block:: bash

    chmod 440 /etc/sudoers.d/wg-client

This can also be achieved with doas or run0.
