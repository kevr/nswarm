nswarm
------

*n[et]swarm*

A network cluster comprised of a master controller and worker nodes connected to it. The cluster contains two servers: an api server and a node server. Nodes are composed of a daemon which connects to an upstream cluster, and a local service server that applications can connect to. The node daemon acts as a relay between service applications and the cluster.

Building
--------

	# Build entire project
	[user@host] nswarm$ mkdir build && cd build
	[user@host] build$ cmake ..
	...
	[user@host] build$ make

	# Install everything (daemons, sdk libraries and headers)
	[user@host] build$ sudo make install

Extending
---------

You can write applications with the public headers and libraries produced by this project.
See `$INSTALL_PREFIX/include/nswarm` for headers.

Authors
-------

* Kevin Morris &lt;kevr.gtalk@gmail.com&gt;

