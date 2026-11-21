
Updating to Version 9.x from 8.x
================================

Updating is straightforward and can even be done while road warrier is using wg-client.

The older code used a semaphore file in wg-client to tell resolv-manager to shutdown. 
While this is no longer used, since it is now done by wireguard itself, 
the new wg-client code will remove the legacy semaphore file which tells the monitor
to shut itself down. So it is safe to update live.

* ssh in to the machine if don't have local access.

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
  i.e. *pgrep -af resolv-manager* and check user has group access via *id <user>* which should
  list *wg-client*

  If the new resolv-manager is running, then delete the older sudoers.d/ file. The new approach
  is more secure.

