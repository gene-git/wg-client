.. SPDX-License-Identifier: GPL-2.0-or-later

.. _config-sect:

Configuration
-------------

wg-client reads its configuration from 

.. code-block:: bash

   /etc/wg-client/config

Please copy the sample config and edit appropriately. The format is in *TOML* format.

.. code-block:: 

   # Sample
   iface = 'wgc'
   ssh_server = 'vpn.example.com'
   ssh_pfx = '47'

This config file provides:

* iface - required

  Wireguard interface; defaults to *wgc*. It is *<iface>* of */etc/wireguard/<iface>.conf*

* ssh_server - optional

  Hostname of the remote ssh server accessible over the vpn;   
  this is where the ssh listening port is run.
  Hostname must be accessible over the wg vpn.

* ssh_pfx - used with ssh_server

  1 or 2 digit number, 65 or smaller, to be used as ssh listening port number prefix.
  The port number is of the form PPxxx, with *PP* the prefix and
  *xxx* is taken from the last octet of the wireguard vpn internal IP address.

  The prefix can also be given as a range of numbers (*'n-m'*). 
  In this case the prefix used is randomly chosen from that range

  Keep in mind that the largest port number is 65535, which limits *ssh_pfx* to be 65 or lower.

The port number chosen will be written to the log file.

The remote ssh host will then listen on *127.0.0.1:<port>*.
It will also listen on *<remote-ip-address>:<port>*
provided the remote ssh server permits it by having the sshd option set: 

.. code-block:: bash

    GatewayPorts yes

