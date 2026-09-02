.. SPDX-License-Identifier: GPL-2.0-or-later

=========
wg-client 
=========

**Migration required from pre 8.x versions**

Here's what is needed:

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

When using a any VPN client, it is standard practive to make sure that all 
DNS traffic is routed through the tunnel in order to trusted DNS servers.
While this is not a hard requirement, and may not always be applicable with split 
routing, it is always desirable from the security standpoint.

There is a little gotcha to be aware of.
The system may, at times, modify */etc/resolv.conf* removing the safety of the 
VPN provided DNS servers by replacing this file. 

Consequently it is important to be aware if this occurrs. 
The way it is handled is by saving this just updated file and restore the 
one that should be used with wireguard. The one that just got saved is the 
the right one to use when the VPN is turned off and by saving a copy
of it, it can be restored to /etc/resolv.conf when wireguard shuts down.

What causes this to happen?
It can happen for different reasons. For example if WiFi network changes, or after a 
sleep resume cycle or when a dhcp lease is renewed. 

While wireguard itself uses UDP and is largely indifferent to such network, 
it is important to keep DNS properly managed.

Taking care of all of this is eactly what *resolv-manager* does.

To ensure that DNS resolver files are all managed correctly requires
a copy of the standard resolv.conf file saved to */etc/resolv.conf.saved*
and the wireguard version to */etc/resolv.conf.wg*.  

With those in place *resolv-manager* will keep track of any changes.
It will saev any new standard */etc/rsolv.conf* and put back the appropriate
one for wireguard.

After creating /etc/wg-client/wireguard-resolv.conf which has the nameservers
using the DNS servers provided by the wireguard server end, then
set wireguard's configuration file (*/etc/wireguard/wgc.conf* for example) 
to use PostUp and PosDown.  Check that the wg-client config file,
/etc/wg-client/config, sets the wireguard interface. For example with a line::

    iface = wg0

.. code-block:: text

    [Interface]
    PrivateKey = ...
    Address    = ...
    Postup     = /etc/wg-client/post-up.sh
    PostDown   = /etc/wg-client/post-down.sh
    ...

There helper scripts *post-up.sh* and *post-down.sh* are part of wg-client.
wg-client handles everything else including running resolv-manager to monitor
/etc/resolv.conf while thw vpn is active to ensure the resolv.conf file
remains correct.

More information about resolv-manager can be found in the man page::

    man wg-client-resolv-manage

wg-client application
=====================

Usage
-----

wg-client is a command line program and can be run in any terminal.
It has two primary options, one to start wireguard and one to stop it.

.. code-block:: bash

   wg-client --wg-up
   wg-client --dg-dn

It also has a config file (/etc/wg-client/config) where the wireguard interface
is specified and ssh information if that is being used.

To get a list of all options use *-h*. 

The command line options for *wg-client* are described in the manual section :ref:`options-sect`.
The configuration file contents in section :ref:`config-sect`.


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
  
To start and stop wireguard wg-client uses *wg-quick* (in wireguard-tools).
Doing so requires root privieles and so any user approved to run wireguard must 
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
example it would be:

.. code-block:: bash

   /etc/sudoers.d/010-wheel
   /etc/sudoers.d/100-wg-client

thereby ensuring that wg-client entries follow the wheel ones.

For convenience this is also noted in the sample file:

.. code-block:: bash

    /etc/wg-client/sudoers.sample

.. code-block:: bash

    chmod -440 /etc/sudoers.d/wg-client

The same thing asl can be achieved using doas or run0.
