==============
resolv-manager
==============

----------------------------------------------
Manages /etc/resolv.conf while using Wireguard
----------------------------------------------


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

post-up.sh (run as root by wg-quick) starts resolv-manager in *monitor* mode::

    resolv-manager -i wgc -t monitor -d -l /var/log/wg-client -f /etc/wg-client/wireguard-resolv.conf

Before it backgrounds itself, it saves a copy of the current standard resolv.conf
to /etc/resolv.conf.saved, saves the provided wireguard file (tagged) to
/etc/resolv.conf.wg and installs it as /etc/resolv.conf. Any failure in these
steps is returned to wg-quick, which then takes the tunnel down again.

The wireguard version has a comment added as the first line in the file in order
to mark it as to be used only while wireguard is active. A file with this tag is
never saved or restored as the standard resolv.conf.

When wireguard is stopped (wg-quick down) the monitor sees the interface go away,
restores the standard resolv.conf and exits. post-down.sh then runs::

    resolv-manager -t standard -l /var/log/wg-client

which normally finds the standard resolv.conf already restored. It is a safety net
in case the monitor was not running (e.g. it was killed).

With the above DNS will work correctly when starting and stopping wireguard using
**wg-client** (or its GUI), or directly with::

    wg-quick up wgc

and ::

    wg-quick down wgc

MONITOR MODE
============

It is quite normal that /etc/resolv.conf can be changed by some network
related events. For example, this can happen after a sleep/resume cycle,
when a DHCP lease renews or when a new WiFi network is connected.

While wireguard itself is largely immune to such changes, since it uses UDP, DNS
needs to be fixed up to keep using the wireguard version of resolv.conf.

The monitor runs as a background daemon and uses *inotify* (the kernel wakes it
up when a file changes) and *rtnetlink* (the kernel tells it when a network
interface is added or removed). It sleeps until one of these wakes it up.

* /etc/resolv.conf changed

  It saves the new standard version to resolv.conf.saved and re-installs the
  wireguard resolv.conf so that DNS continues to use the tunnel.
  It waits until there have been no further changes for 200 ms (at most 2 s), so a
  burst of changes is handled once, using the final content, and a file is never
  read while another program is still writing it. An empty resolv.conf is never
  saved. Files are replaced atomically; anything written by another program while
  resolv-manager was replacing the file is detected and not lost.

* The -f file changed (e.g. /etc/wg-client/wireguard-resolv.conf)

  It is saved (tagged) as resolv.conf.wg and installed. Edit this file to change
  the VPN DNS settings while the tunnel is up. If it is removed or left empty
  the current settings are kept.

* /etc/resolv.conf.wg changed

  It is installed (it must keep the tag line). If it is removed, and still gone
  after the 200 ms settle time, the standard resolv.conf is restored and the
  monitor exits. Editors that save by renaming the original first are handled.

* The wireguard interface is removed (wg-quick down)

  The standard resolv.conf is restored and the monitor exits.

* SIGTERM, SIGINT, SIGHUP or SIGQUIT

  The standard resolv.conf is restored and the monitor exits.

If /etc/resolv.conf is a symlink (e.g. systemd-resolved) the symlink itself is
saved and restored.

Clearly there cannot be more than one program monitoring and modifying the same files.
File Locking is a sound way to make sure this does not happen.
The lock file used is **/run/lock/wg-client/lock**.
resolv-manager will only monitor or change any files after it has acquired the lock.
While another instance holds the lock it waits (up to 30 seconds).

SECURITY
========

resolv-manager must be run as root - it is run by wireguard using PostUp and
PostDown. It needs no file capabilities.

It starts as root and, before doing its work, changes to user nobody and drops
its privileges, retaining only the 2 capabilities it needs to manage files in /etc::

    cap_chown,cap_dac_override

These are only in its permitted set; they are made effective just for the
moments it reads or writes files in /etc. It also sets *no_new_privs* and
removes all other capabilities from its bounding set.

The -T (testing) option may be used by a normal user. All capabilities are then dropped.

OPTIONS
=======

The required options are **--iface** and **--task**.

.. code-block:: text

    -t, --task       <name>   Task name is one of: s[tandard], w[ireguard] or m[onitor]
    -i, --iface      <iface>  The wireguard inteface name (e.g. wg0)
    -l, --logdir     <dir>    Log directory
    -n, --numlogs    <num>    Number of log files to keep in the rotation.
    -d, --daemon              Run as a background daemon (monitor).
    -f, --file       <file>   The wireguard resolv.conf to use. Saved (tagged) as /etc/resolv.conf.wg.
                              With monitor, changes to this file are applied while running.
    -T, --testing             Developer option. Files are relative to the current directory.
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

* EX_NOPERM

  Not run as root (and not testing mode).

* EX_OSFILE

  File related errors, typically unable to open a file.

* EX_TEMPFAIL

  Not ready as wireguard is not running, or the lock is held by another instance.

* EX_OSERR

  Likely due to failure to daemonize or drop privileges.

* EX_IOERR

  IO errors are likely to to reading or writing files. Probably /etc/resolv.conf et al.

* EX_SOFTWARE

    Some other problem occurred. Most likely the monitor encountered an unrecoverable 
    error related to inotify.


EXAMPLES
========

Start the monitor as a daemon (what post-up.sh does)::

    /usr/lib/wg-client/resolv-manager -i wgc -t monitor -d -l /var/log/wg-client -f /etc/wg-client/wireguard-resolv.conf

Install a wireguard file to */etc/resolv.conf* and save current standard file (no monitor)::

    /usr/lib/wg-client/resolv-manager -i wgc -t wireguard -f /etc/wg-client/wireguard-resolv.conf

Restore the standard /etc/resolv.conf (what post-down.sh does)::

    /usr/lib/wg-client/resolv-manager -t standard -l /var/log/wg-client


SEE ALSO
========

wg(8), wg-quick(8) 

