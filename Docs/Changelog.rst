Changelog
=========

Tags
====

.. code-block:: text

	3.7.1 (2024-01-07) -> 7.3.0 (2026-01-10)
	88 commits.

Commits
=======


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


