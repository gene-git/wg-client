Changelog
=========

**[5.0.2] ----- 2024-07-01** ::

	    Readme - clarify that gui starts the monitor daemon automatically
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[5.0.1] ----- 2024-07-01** ::

	      * Auto fix of resolv.conf (new option *--fix-dns-auto-start*)
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
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[4.2.0] ----- 2024-04-17** ::

	    Package update: "pacman -Qc wg_tool" now shows the Changelog
	    Move version info to version.py
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[4.1.3] ----- 2024-02-09** ::

	    Fix github url in PKGBUILD
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[4.1.2] ----- 2024-02-09** ::

	    update Docs/Changelog.rst Docs/wg-client.pdf
	    Fix typoe
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[4.1.1] ----- 2024-02-09** ::

	    Add missing PKGBUILD dependencies as reported on AUR by gwy
	            https://aur.archlinux.org/packages/wg-client#comment-955729
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[4.1.0] ----- 2024-01-17** ::

	    ssh_listener now handles pure IPv6 wg iface to build listening port
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[4.0.1] ----- 2024-01-08** ::

	    rst fixes for readme as github ignoring some code-blocks
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[4.0.0] ----- 2024-01-08** ::

	    dns resolv.conf fix now uses c-program with capabilities.
	      Now sudu is only needed to run wg-quick.
	      Docs updated with info on new /usr/lib/wg-client/wg-fix-resolv program
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[3.7.6] ----- 2024-01-08** ::

	    bump to 3.7.6
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[3.7.5] ----- 2024-01-08** ::

	    update Docs/Changelog.rst Docs/wg-client.pdf
	    update version for installer fix
	    update Docs/Changelog.rst Docs/wg-client.pdf
	    installer typo fix
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[3.7.4] ----- 2024-01-08** ::

	    README - document all the options of wg-client
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[3.7.3] ----- 2024-01-07** ::

	    small readme tweak
	    update Docs/Changelog.rst Docs/wg-client.pdf


**[3.7.1] ----- 2024-01-07** ::

	    wg-client provides command line and gui tool to start and stop wireguard


