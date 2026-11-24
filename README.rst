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

A companion package, `wg-tool`_ is for adminstration the server side of things.

wg-client also has an option to run ssh and create a remote listening port 
connected back to local (client end) ssh daemon.

This is useful to facilitate remote ssh back to client computer 
if it's needed.  For example; it can be used to provide access to a git repo
on the client, or for remote backups of laptop, or for admin to login to client
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
DNS traffic is routed through the VPN tunnel to trusted DNS servers.
While this isn't a hard requirement, and is not strictly applicable with split 
routing, it is always desirable from a security standpoint.

There is a little gotcha to be aware of.
The system may, at times, modify */etc/resolv.conf* removing the safety of the 
VPN provided DNS servers to different ones. This also breaks LAN related services.

Consequently, it is important to identify if this happens and deal with it.
It is handled is by saving a copy of any new resolv.conf file and then restoring the 
one that should be used while wireguard is running. When wireguard shuts down 
the standard resolv.conf is restored from the saved copy.

What causes this to happen?
It can happen for various reasons. It happens if the WiFi network changes, after a 
sleep resume cycle or when a DHCP lease is renewed. It can also happen when WiFi briefly
drops which may occurr after too many missed beacons for example. In other words, it
is quite common.

Wireguard itself uses UDP and is mostly indifferent to such network changes, 
nonetheless it is important to keep DNS properly managed.

Taking care of this automatically and robustly is what *resolv-manager* does.

resolv-manager ensures that DNS resolver file being used is the appropriate 
one whether wireguard is running or not. This is done by keeping up to date copies
if both resolv.conf file; one for wireguard running and one standard one.

When resolv-manager starts it makes a copy of the resolv.conf file to be used
while wireuguard is running and adds a *tag* to that file that identifies it as
the one to be used with the VPN up. It also saves a copy of the standard resolv.conf
so it can be restored when wireguard shuts down. These are saved to::

    /etc/resolv.conf.wg
    /etc/resolv.conf.saved

With those in place *resolv-manager* then tracks of any changes in */etc/resolv.conf*.
If that file changes, as discussed above, It saves that updated version of */etc/rsolv.conf* 
and puts back the file that is used while wireguard is up.

The one to use while wireguard is up is provided in::

    /etc/wg-client/wireguard-resolv.conf

Be sure that this has the appropriate DNS server information.

resolv-manager watches this file as well, and if it changes then it is tagged, saved to
*/etc/resolv.conf.wg* and installed to */etc/resolv.conf*.  This feature gives admins
a simple way to modify the wireguard DNS servers live while the tunnel is up. Convenient 
if not likely to be used very often.

resolv-manger is started from /etc/wg-client/post-up.sh.  The wireguard config 
therefore needs this for it's PostUp config (e.g. */etc/wireguard/wgc.conf*)::

    [Interface]
    PrivateKey = ...
    Address    = ...
    Postup     = /etc/wg-client/post-up.sh
    PostDown   = /etc/wg-client/post-down.sh
    ...

resolv-manager monitors for wireguard being shut down, and restores the
standard resolv.conf. This is also done, if needed, in /etc/wg-client/post-down.sh.
This is simply a safety net in case, for some reason, resolv-manager is killed in 
a way that it does not restore the standard resolv file.

The helper scripts *post-up.sh* and *post-down.sh* are provided by wg-client.

In addition wg-client'w own config file::

    /etc/wg-client/config
    
must have the wireguard interface being used. For example with a line::

    iface = wgc

Users permitted to use wireguard must be members of the *wg-client* group. 
This group is given sudo permission allowing */usr/bin/wg-quick* to 
be run to start/stop wireguard but only for interface names that start with
the letter *w* (e.g. wgc or wg0).

More information about resolv-manager is available in the man page::

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

**important**

Members of group wg-client can bring up and down any WireGuard interface whose name starts with 
*w* (e.g. wgc, wg0, wg-home) and has a config in /etc/wireguard. Interfaces that users must not control, 
such as server or site-to-site tunnels, should use a name that does not start with w (e.g. site0, srv-link).

Renaming an interface means renaming /etc/wireguard/<name>.conf and, if used, the wg-quick@<name> systemd unit.

Coding
======

Claude as been very helpful in analyzing and improving the code, especially  
around potential security issues but also for handling errors and edge-cases.
