.. SPDX-License-Identifier: GPL-2.0-or-later

=========
wg-client 
=========

**Changes needed when updating to 9.x**

If you are updating from pre 9.x versions please see the
chapter in the manual :ref:`updating`.

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

Getting Started
===============

Root is required to start wireguard itself. This is achieved using *sudo* 
and granting root privilileges to run *wg-quick up/down* without a password for members of 
the *wg-client* group.

Therefore, all wg-client users must be added the *wg-client* group. This is done as root::

    gpasswd -a <user> wg-client

wg-client provides */etc/sudoers.d/99-wireguard*. 


Sudoers
-------
  
wg-client uses *wg-quick* from wireguard-tools to start and stop wireguard.
Doing so requires root priviliges. As a result, any user approved to run wireguard must 
be granted permission.  wg-client provides::

    /etc/sudoers.d/99-wireguard

This permits members of the *wg-client* to use *wg-quick* to start and stop
wireguard for interfaces names that begin with *w* without a password.

By restricting wg-client users to interface names that start with *w*, allows
other system tunnels to use a different name and prevent wg-client users
from touching those.

sudo parses the files in */etc/sudoers.d/* in lexical order, with the last file
parsed being used.
Please ensure that any sudoers file that grants *wheel* access, that needs 
a password, is parsed before *99-wireguard*. For example *010-wheel*.

This ensures that things work correctly for any user in *wg-quick* group 
that is also a member of *wheel* group.

While this can also be achieved with *doas* or *run0*, the current code invokes
*sudo*. We plan to support *doas* and *run0* in the future.
 
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

    iface = wgc

Users with permission to use wireguard must be members of the
*wg-client* group. This group is given sudo permission that allows */usr/bin/wg-quick* to 
be appropriately run.

Wireguard interfaces that users may use must have interface names that start with
the letter *w*. 

The helper scripts *post-up.sh* and *post-down.sh* are part of wg-client.

The scripts also run resolv-manager to monitor /etc/resolv.conf while thw vpn is active to 
ensure that the correct resolv.conf is used so that DNS is resolved using
the DNS needed while the tunnel is up. It also restores the
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


Non-User Wireguard Tunnels
==========================

**imporant**

Members of group wg-client can bring up and down any WireGuard interface whose name starts with 
*w* (e.g. wgc, wg0, wg-home) and has a config in /etc/wireguard. Interfaces that users must not control, 
such as server or site-to-site tunnels, should use a name that does not start with w (e.g. site0, srv-link).

Renaming an interface means renaming /etc/wireguard/<name>.conf and, if used, the wg-quick@<name> systemd unit.

Coding
======

Claude as been very helpful in analyzing and improving the code, especially  
around potential security issues but also for handling errors and edge-cases.
