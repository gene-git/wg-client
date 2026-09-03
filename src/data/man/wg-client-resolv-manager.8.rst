==============
resolv-manager
==============

----------------------------------------------
Manages /etc/resolv.conf while using Wireguard 
----------------------------------------------

:Author: Gene C <arch@sapience.com>
:Manual section: 8
:Manual group: Linux Tools
:Date: @DATE@
:Version: @VERSION@

SYNOPSIS
========

**/usr/lib/wg-client/resolv-manager** -i <iface> -t <task-name> [*options* ...]

DESCRIPTION
===========

Part of the wg-client package. When wireguard VPN is running DNS is typically
used via the VPN tunnel. When wireguard is started an appropriate /etc/resolv.conf
should be installed and when it is stopped the standard version should be restored.

This is simple to do using PostUp and PostDown in the wireguard config file.
For example if the config file is /etc/wireguard/wgc.conf, so the wireguard interface 
is wgc then it would have::

    [interface]
    ...
    PostUp    = /etc/wg-client/post-up.sh wgc /etc/wg-client/wireguard-resolv.conf
    PostDown  = /etc/wg-client/post-down.sh

Where the file /etc/wg-client/wireguard-resolv.conf is the resolv.conf to use
while the VPN is running.

With no arguments post-up.sh reads the wireguard interface from /etc/wg-client/config
and uses /etc/wg-client/wireguard-resolv.conf for the VPN resolv.conf.

post-up.sh uses resolv-manager to save a copy of the current standard
resolv.conf and install the wireguard provided file to /etc/resolv.conf by running::

    resolv-manager -i wgc -t wireguard -f /etc/wg-client/wireguard-resolv.conf -l /var/log/wg-client

The standard resolv.conf is saved to resolv.conf.saved and the caller provided
wireguard file is tagged and saved to resolv.conf.wg.

When shutting down wireguard, post-down.sh uses it to restore the standard resolv.conf by 
using::

    resolv-manager -t standard -l /var/log/wg-client

In this case the standard resolv.conf is restored from the saved version.

The wireguard version has a comment added as the first line in the file in order
to mark it as to be used only while wireguard is active.

With the above DNS will be work correctly when starting and stopping wireguard using::

    wg-quick up wgc

and ::

    wg-quick down wgc

It is quite normal that /etc/resolv.conf can be changed by some network
related events. For example, This can happen after a sleep/resume cycle,
when a DHCP lease renews or when a new WiFi network is connected. 

While wireguard itself is largely immune to such changes, since it uses UDP, DNS 
needs to be fixed up to keep using the wireguard version of resolv.conf.

resolv-manager handles this. When **wg-client --wg-up** is used to start
wireguard (or the GUI which runs the same command) it starts *ressolve-manager*
in *monitor* mode.

MONITOR MODE
============

**wg-client --wg-up** starts wireguard and then runs::

    resaolv-manager -i wgc -t monitor 

This runs as a background daemon and monitors /etc/resolv.conf for any changes.
This is done using *inotify* whereby it asks the kernel to wake it up when the file
changes. This is exremely efficient and allows it to sleep until the kernel wakes it up.

When it detects that resolv.conf has changed, it saves the new standard version
and re-installs the VPN resolv.conf so that DNS continues to use the tunnel.

It also monitors /sys/class/net/wgc (or whatever interface is being used) and
when it detects that wireguard is no longer running it shuts itself down.

It can also monitor has an option to monitor a **semaphore** file and 
deleting that semaphore file, signals the monitor to shutdown.
wg-client uses a semaphore file.

The reason is because resolv-manager uses locking to guarantee
there is only one version running at a time. The semaphore gives **wg-client**
tha ability to instuct resolv-manager to shut down before it turns off wireguard. 
This way wireguard's PostDOwn can safely use resolv-manager to restore
the standard resolv.conf since the monitor instance is no longer running.

Clearly there cannot be more than one program monitoring and modifying the same files.
File Locking is a sound way to make sure this does not happen.
The lock file used is **/run/lock/wg-client/lock**.
resolv-manager will only monitor or change any files after it as acquired the lock.

SECURITY
========

Since resolv-manager needs privileges to modify file in /etc and it is
run by non-root user, it uses just enough **capabilities** to do its job. 

These are all that are required::

    /usr/bin/setcap cap_chown,cap_dac_override=ep /usr/lib/wg-client/resolv-manager

The Arch PKGBUILD sets these capabilities when the package is installed.

It is designed to be run as ordinary unprivileged user. 

If it is run as root, then it drops privileges to nobody and retains 
the capabilities to function properly. Starting wireguard itself needs root privileges
so resolv-manager is started by root in the PostUp and PostDown settings.


OPTIONS
=======

The required options are **--iface** and **--task**.

.. code-block:: text

    -t, --task       <name>   Task name is one of: s[tandard], w[ireguard] or m[onitor]
    -i, --iface      <iface>  The wireguard inteface name (e.g. wg0)
    -l, --logdir     <dir>    Log directory
    -n, --numlogs    <num>    Number of log files to keep in the rotation.
    -d, --daemon              Run as a background daemon.
    -f, --file       <file>   Used with -t wireguard to provide the wireguard resolv.conf to use
    -s, --semaphores <file>   Watch this file and exit if removed or not found (monitor mode)
    -T, --testing             Developer option. Fles are relative to the current directory.
                              For example it looks for ./etc/resolv.conf
    --help, -h                Show help message and exit.

At least one task is required and both the *wireguard* and *monitor* tasks both need 
the interface to be provided as well.
                                
ERRORS
======

The exit codes are taken from "sysexits.h":

* EX_OK

  A successful termination.

* EX_USAGE

  Problematic command line arguments.

* EX_OSFILE

  File related errors, typically unable to open a file.

* EX_TEMPFAIL

  Not ready as wireguard is not running.

* EX_OSERR

  Likely due to failure to daemonize

* EX_IOERR

  IO errors are likely to to reading or writing files. Probably /etc/resolv.conf et al.

* EX_SOFTWARE

    Some other problem occurred. Most likely the monitor encountered an unrecoverable 
    error related to inotify.


EXAMPLES
========

Run as a daemon to monitor the dns resolv.conf file for changes::

    /usr/lib/wg-client/resolv-monitor -i wg0 -t monitor -d -l /var/log/wg-client

Run as non-priveleged user and exit when a semaphore file is removed::

    /usr/lib/wg-client/resolv-monitor -i wg0 -t monitor -s /tmp/i-am-running

Install a wireguard file to */etc/resolv.conf* and save current standard file::

    /usr/lib/wg-client/resolv-monitor -i wg0 -t wireguard -f /etc/wg-client/wg-resolv.conf

Restore the standard /etc/resolv.conf back to */etc/resolv.conf*::

    /usr/lib/wg-client/resolv-monitor -t standard -l /var/log/wg-client


SEE ALSO
========

wg(8), wg-quick(8) 

