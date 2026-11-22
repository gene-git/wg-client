Changelog
=========

Tags
====

.. code-block:: text

	3.7.1 (2024-01-07) -> 9.1.2 (2026-11-22)
	102 commits.

Commits
=======


* 2026-11-22  : **9.1.2**

.. code-block:: text

              - 9.1.2 : Manual and man page updates (no code change)

* 2026-11-21  : **9.1.1, origin/master**

.. code-block:: text

              - **9.1.1**
                * Doc change: add thank you to `Claude (Anthropic) <https://claude.ai>`_

* 2026-11-21  : **9.0.0**

.. code-block:: text

              - **9.0.0**
            
                **Changes required**
            
                * wg-client users **must** now be added to the *wg-client* group.
                * An sudoers file is installed that allows wireguard to be used by *wg-client* group members.
                  Please remove any existing sudoers file as no longer needed and is less secure than
                  the new version using the *wg-client* group.
                * The sudoers file limits users to wireguard interface names starting with the letter *w*.
                  This way any system tunnel on same machine cannot be modified by wg-client users,
                  provided that the system interface name does not start with a *w*. E.g. a system
                  tunnel might be named *memphis-1*. Be sure any wg-client interface name
                  starts with a *w*
                * You can leave the existing sudoers.d file until user has logged out and back.
                * Please see the chapter in the documentation titled *UPDATING TO VERSION 9.X FROM 8.X*.
            
                * resolv-manager enhancements.
            
                  * Scan code for bugs, edge cases and security.
                  * Fix: Privileges management
                  * Fix: Add signal handler
                  * Fix: Handle /etc/resolv.conf being a symlink
                  * Fix: Race with external /etc/resolv.conf writers (dhcp, network manager etc)
                         This fixes a race bug where the non-wireguard resolv.conf is not
                         actually replaced by the wireguard version after some other process has modified it.
                  * Fix: Improve error checks and handling
                  * /etc/sudoers.d/100-wireguard now provided. It permits users in linux group *wg-client*
                    to start and stop wireguard.
                  * **Change Needed**:
            
                    Wireguard users *must* now be added to the wg-client group after installing the new package:
            
                        gpasswd -a <USERNAME> wg-client
            
                    As usual, users needs to logout and login to gain new group access.
                    Members of *wg-client* group are permitted to manager wireguard tunnels whose
                    interface name starts with the letter *w*.
            
                  * If there are any system wireguard tunnels running on same machine as users
                    with *wg-client* priviliges, please ensure that those system tunnels use interface
                    names that do not start with *w*.
            
                  * Fix: Monitor wireguard exit - inotify is filesystem only, so cannot be used for
                    kernel files like /sys/class/net/. Use rtnetlink instead.
            
                  * resolv-manager now started by wireguard post-up/post-down. This simplifies
                    things and removes some code from the wg-client application.
            
                  * Remove capabilities on resolv-manager executable. These are no longer needed since it is
                    now started by wireguard. resolv-manager drops all capabilites save the 2 it needs
                    to manage the resolv.conf files (cap_chown,cap_dac_override=ep)

* 2026-10-04  : **8.1.0**

.. code-block:: text

              - release 8.0.5
                Sun Oct  4 06:14:30 AM EDT 2026
                * Update Documentation
                  * Now available on https://wg-client.readthedocs.io
                * GUI
                  * Migrate from PyQt6 to PySide6
                  * Fix some GUI related warnings
                    * Could not register app ID: Connection already associated with an application ID
                    * Trying to disable 0xNNN but 0x0 is focused
              - pre-release commit for readthedocs

* 2026-09-10  : **8.0.4**

.. code-block:: text

              - **8.0.4**
            
                * Change Arch package dependencies that have been renamed:
            
                  - pyconcurrent -> python-pyconcurrent
 2026-09-03   ⋯

.. code-block:: text

              - add .gitattributes to mark docs as docs so github treats as docs not code

* 2026-09-03  : **8.0.3**

.. code-block:: text

              - **8.0.3**
            
                * Typo in Readme

* 2026-09-02  : **8.0.2**

.. code-block:: text

              - **8.02**
            
                * Add missing dependency no PyCidr package used by the ssh listener

* 2026-09-02  : **8.0.1**

.. code-block:: text

              - meson tweak
              - remove unused from meson.build
              - add meson.options

* 2026-09-02  : **8.0.0**

.. code-block:: text

                  - **8.0.0**
            
                    * Package / build management now uses meson/mesonpy
                    * New resolv-manager:
                      - wirtten in C.
                      - Runs as a background daemon managing /etc/resolv.conf
                      - replaces the python inotify based monitor
                      - see man page wg-client-resolv-manager
                    * You need to make a change.
                      Please update PostUp / PostDown in wireguard config and
                      use the helper scripts installed in /etc/wg-client:
                      post-up.sh and post-down.sh.
                      Create /etc/wg-client/wireguirs.resolv.conf with the DNS settings
                      you want to use while VPN tunnel is up. Standard resolv.conf format
                      (nameserver x.x.x.x)
 2026-01-10   ⋯

.. code-block:: text

              - update Docs/Changelog

* 2026-01-10  : **7.3.0**

.. code-block:: text

              - Correct license in pyproject.toml
                Arch PKGBUILD - move graphical dependencies into the package() function.
                  Makes these runtime and build server does not need to have them installed
 2026-01-06   ⋯

.. code-block:: text

              - update Docs/Changelog

* 2026-01-06  : **7.2.0**

.. code-block:: text

              - **Version 7.2.0**
            
                * Drop unmaintained *netifaces* module - use our own code for what we need.
              - update Docs/Changelog

* 2026-01-06  : **7.1.1**

.. code-block:: text

              - update Docs/Changelog

* 2026-01-06  : **7.1.0**

.. code-block:: text

              - Add c-code back into repo (oops)
              - update Docs/Changelog

* 2026-01-06  : **7.0.0**

.. code-block:: text

              - **7.0.0**
            
                * Code Reorg
                * Switch packaging from hatch to uv
                * Add source checks for C-code as well as python
                * Testing to confirm all working correctly on python 3.14.2
                * C-code (wg-fix-resolve): Use more const pointer to structs to improve security.
                * Modern coding standards: PEP-8, PEP-257 and PEP-484 style and type annotations
                * Use pyconcurrent module (additional dependency)
 2025-03-20   ⋯

.. code-block:: text

              - update Changelog and Docs/wg-client.pdf

* 2025-03-20  : **6.11.0**

.. code-block:: text

              - GUI Quit button - make saure stop resolv monitor before stopping wireguard
                  Wireguard will restore the correct rsolv.conf from resolv.conf.saved
                Better Changelog format (for packaging/Changelog and Docs/Changelog.rst)
 2025-03-17   ⋯

.. code-block:: text

              - update Changelog and Docs/wg-client.pdf

* 2025-03-17  : **6.10.1**

.. code-block:: text

              - Add Changelog to Arch package (pacman -Qc wg-client)

* 2025-03-17  : **6.10.0**

.. code-block:: text

              - Change wg-fix-resolv: Ignore comments when comparing resolv.conf files.
                  More efficient/correct when only change is a commented time stamp for example.
                resolv monitor: Increase maximum time to wait for wireguard to start before running the monitor.
                  No reported issues with 5 seconds - no harm in being able to wait a bit longer if needed for some reason.
                Resolv monitor log when it starts
 2025-03-09   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2025-03-09  : **6.8.0**

.. code-block:: text

              - Improve readme and fix typo in help message
                Additional self protections in kill_ssh() function
                Tidy some code.
 2025-03-02   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2025-03-02  : **6.7.0**

.. code-block:: text

              - Improve logging when ssh listener exits.
 2025-02-27   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2025-02-27  : **6.6.0**

.. code-block:: text

              - Improve ssh retry loop after ssh session is dropped
                Increase saved logs 2x10k to 5x100k
                wg-dn stops any ssh listener as well
 2024-12-31   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-12-31  : **6.5.0**

.. code-block:: text

              - Git tags are now signed.
                Update SPDX tags
                Add git signing key to Arch Package
                Bump python vers
 2024-12-23   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-12-23  : **6.4.0**

.. code-block:: text

              - Fix bug with root checking whether non-root users have ssh running
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-12-23  : **6.3.0**

.. code-block:: text

              - Make sure pid is always int (fixes bug where reading pid returned None)
 2024-12-22   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-12-22  : **6.2.0**

.. code-block:: text

              - Bug fix with display of ssh prefix in status / show-info
 2024-12-21   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-12-21  : **6.1.0**

.. code-block:: text

              - Timeout between ssh reconnects now 30 seconds
              - ssh listener is now auto restarted if it exits unexpectedly.
                  There are normal, quite common situations where ssh process can exit prematurely. (After sleep/resume, remote server sshd restarts/reboot, changing IP address such as location change of laptop)
 2024-10-20   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-10-20  : **5.10.0**

.. code-block:: text

              - Use ipaddress in place of netaddr
 2024-09-07   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-09-07  : **5.9.3**

.. code-block:: text

              - rst continued (gh seems different to sphinx)
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-09-07  : **5.9.2**

.. code-block:: text

              - More rst tidy ups
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-09-07  : **5.9.1**

.. code-block:: text

              - Tidy restructured text formatting in readme
 2024-07-07   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-07  : **5.9.0**

.. code-block:: text

              - wg-fix-resolv: Improve compiler / loader options - see Makefile for details
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-07  : **5.8.1**

.. code-block:: text

              - Typo in version string
 2024-07-06   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-06  : **5.7.0**

.. code-block:: text

              - --status as root now displays ssh/resolv for other users if active
 2024-07-04   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-04  : **5.6.0**

.. code-block:: text

              - Improve comments and log more in wg-fix-resolv
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-04  : **5.5.0**

.. code-block:: text

              - wg-fix-resolv: tidy up code add mem_alloc() helper.
                No need to null terminate date read from file
 2024-07-03   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-03  : **5.4.0**

.. code-block:: text

              - wg-fix-resolv: simplify file_compare() which now returns bool
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-03  : **5.3.0**

.. code-block:: text

              - wg-fix-resolv: chown(root) if write resolv.conf.saved.
                  Fixes (benign) bug where owner of the file resolv.conf.saved can be user instead of root
 2024-07-02   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-02  : **5.2.0**

.. code-block:: text

              - When comparing file digests use strncmp() with known dynamic length not EVP_MAX_MD_SIZE
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-02  : **5.1.0**

.. code-block:: text

              - wg-fix-resolv.c: Generalize the file hashing and switch to SHA384
                  The hash is used to compare two of the resolv.conf files for any changes
                Code tidy ups
 2024-07-01   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-01  : **5.0.2**

.. code-block:: text

              - Readme - clarify that gui starts the monitor daemon automatically
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-07-01  : **5.0.1**

.. code-block:: text

                - * Auto fix of resolv.conf (new option *--fix-dns-auto-start*)
            
                    Network refresh often happens after sleep/resume (e.g. laptop lid close/open) or
                    when a DHCP lease expires. If VPN is up and running
                    when this occurs the /etc/resolv.conf file can be reset and then DNS will no longer use
                    the vpn DNS but will then use whatever resolver DHCP provided by default.
                    Earlier versions of wg-client offered a manual fix available
                    by clicking the *VPN Start* button again or by using wg-client on command line.
            
                    This is now done automatically using a daemon which can be started/stopped from command line
                    using  the new options *--fix-dns-auto-start* and *--fix-dns-auto-stop*
            
                    The GUI app does this whenever it starts wireguard.
            
                  * *--version*
            
                    Display wg-client version
            
                  * NB version 5 has 2 additional dependencies:
            
                    - openssl library for wg-fix-resolv.c
                    - python-pynotify library available via github and AUR
 2024-04-17   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-04-17  : **4.2.0**

.. code-block:: text

              - Package update: "pacman -Qc wg_tool" now shows the Changelog
                Move version info to version.py
 2024-02-09   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-02-09  : **4.1.3**

.. code-block:: text

              - Fix github url in PKGBUILD
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-02-09  : **4.1.2**

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf
              - Fix typoe
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-02-09  : **4.1.1**

.. code-block:: text

              - Add missing PKGBUILD dependencies as reported on AUR by gwy
                        https://aur.archlinux.org/packages/wg-client#comment-955729
 2024-01-17   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-01-17  : **4.1.0**

.. code-block:: text

              - ssh_listener now handles pure IPv6 wg iface to build listening port
 2024-01-08   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-01-08  : **4.0.1**

.. code-block:: text

              - rst fixes for readme as github ignoring some code-blocks
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-01-08  : **4.0.0**

.. code-block:: text

              - dns resolv.conf fix now uses c-program with capabilities.
                  Now sudu is only needed to run wg-quick.
                  Docs updated with info on new /usr/lib/wg-client/wg-fix-resolv program
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-01-08  : **3.7.6**

.. code-block:: text

              - bump to 3.7.6
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-01-08  : **3.7.5**

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf
              - update version for installer fix
              - update Docs/Changelog.rst Docs/wg-client.pdf
              - installer typo fix
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-01-08  : **3.7.4**

.. code-block:: text

              - README - document all the options of wg-client
 2024-01-07   ⋯

.. code-block:: text

              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-01-07  : **3.7.3**

.. code-block:: text

              - small readme tweak
              - update Docs/Changelog.rst Docs/wg-client.pdf

* 2024-01-07  : **3.7.1**

.. code-block:: text

              - wg-client provides command line and gui tool to start and stop wireguard


