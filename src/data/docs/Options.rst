.. SPDX-License-Identifier: GPL-2.0-or-later

.. _options-sect:

Options
=======

wg-client
---------

Summary of available options for wg-client.

**Positional argument** : Optional  

* wireguard interface name

  Default interface is taken from *iface* in config file.
  The config file is chosen by first checking for *./etc/wg-client/config* [#]_ 
  and then in */etc/wg-client/config*.  If not found the wg interface defaults to *wgc*

.. [#] Useful during development and testing

**Options**:

These are all the options available in wg-client::

    positional arguments:
      iface                 Optional wg interface (or test-dummy for test mode)

    options:
      -h, --help            show this help message and exit
      --test                Test mode
      --wg-up               Turn wireguard on
      --wg-dn               Turn wireguard off
      --resolv-manager-start
                            Try (re)start resolv-manager daemon - development option
      --ssh-start           Run ssh over vpn to create remote listener
      --ssh-stop            Kill ssh listener if its running
      --ssh-pfx SSH_PFX     ssh port(s). 2 digits: "nn" or "nn-mmm" for range
      --ssh-server SSH_SERVER
                            Remote ssh server to set up listening port
      --show-iface          Report the wg interface name
      --show-ssh-server     Report the ssh server name
      --show-ssh-running    Report if ssh is running
      --show-wg-running     Report if wireguard is running
      --show-resolv-manager
                            Report if resolv-manager is running
      --show-info           Display status - alias for --status
      --status              Display status
      --version             Display version



Uaing ssh
^^^^^^^^^

When asking wg-client to start ssh, it runs ssh to the remote host inside the vpn tunnel,
sets it to listen on a remote port.
Port number used is described in section :ref:`config-sect`.

Note that this blocks waiting for ssh. To stop ssh, simply make a separate 
invocation of *wg-client -ssh-stop*. If using the GUI tool, simply click the *Stop Ssh* button. 

In the event that ssh connection is dropped, it will automatically be restarted.
There are normal, quite common situations where ssh process can exit prematurely.

  For example:

   * After sleep/resume (longer than tcp timeout)
   * if remote server sshd restarts (reboot for example)
   * changing IP address (e.g. happens when location changes. e.g. Move from hotel to starbucks)

While attempting to reconnect ssh there is a waiting period between each attempt. 
Currently the wait time is 30 seconds.


The ssh port prefix can be 2 digits: "nn" or a range of 2 digit numbers "nn-mm". When using a range, 
prefix used will be randomly drawn from the range. Maximum value is 65. This can also be set in 
the config file.

Log files
=========

There are 3 kinds of log files: one for resolv-manager, one for command line app
and one for the gui app.

resolv-manager logs are in /var/log/wg-client and while wg-client and wg-client-gui
logs are in::

    ${HOME}/log/wg-client
    ${HOME}/log/wg-client-gui

All log files are rotated with the usual integer suffix scheme.

