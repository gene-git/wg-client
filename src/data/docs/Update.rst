.. _updating:

Updating to Version 9.x from 8.x
================================

Updating is straightforward and can even be done while a road warrier is using wg-client.

wg-client will keep working without any change. However, we recommend making the following
changes after installing the 9.x version.

* Add any wg-client users to the *wg-client* group

  - gpasswd -a <user> wg-client

* Remove any older sudoers file:
  
  - Version 9.x and later provides /etc/sudoers.d/99-wireguard


Live Remote Update
------------------

A remote user can have wg-client updated and everything should continue working normally.
The recommended changes to sudoers and adding user(s) to the new wg-client group can be
done when convenient. For a remote user they can also be done live over the wireguard tunnel.

his is for convenience and not a reuquirement:

* ssh in to the machine. If don't have local access.
  Have the user press the 'start ssh' button which opens an ssh listening port on the wireguard server.

* Install the new wg-client package.
* Add the wg-client user(s) to the *wg-client* group (required step)::

    gpasswd -a <user> wg-client for each one.

* Leave the old sudoers file in place. It’s a fallback if the group step went wrong for some reason.
  or if the user did not logout and back in, which is needed to gain access to the new *wg-client* 
  group.

* The next time the user stops the VPN, quits the GUI or reboots, the leftover 
  legacy monitor is cleaned up automatically. 

  You can also ask the user to logout from the graphical session, log back in restart wireguard and ssh listener
  so you can log back in.

* When you’re next in over SSH, check for the new monitor which runs as nobody (instead of as user).
  and check that the user has group *wg-client*::

    pgrep -af resolv-manager
    ps -fp $(pgrep -f resolv-maager)
    id <username>


  If the new resolv-manager is running, then delete the older sudoers.d/ file. The new approach
  is more secure.


Updating to 8.x from 7.x
========================

Older versions (pre 8.x) need a small change becasue *resolv-manager* has been re-written
in C and now runs a background daemon managing /etc/resolv.conf.

This is what is needed:

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

