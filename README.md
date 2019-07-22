nswarm
------

*n[et]swarm*

A network cluster comprised of a master controller and worker nodes connected to it. The cluster contains two servers: an api server and a node server. Nodes are composed of a daemon which connects to an upstream cluster, and a local service server that applications can connect to. The node daemon acts as a relay between service applications and the cluster.

## Network Flow

!["Networking Components"](https://github.com/kevr/nswarm/raw/master/doc/networking_components.png)

## Building

	# Build entire project
	[user@host] nswarm$ mkdir build && cd build
	[user@host] build$ cmake ..
	...
	[user@host] build$ make
	# Run all tests built via `make`
	[user@host] build$ make runtests

	# Install everything (daemons, sdk libraries and headers)
	[user@host] build$ sudo make install

## Extending

You can write applications with the public headers and libraries produced by this project.
See `$INSTALL_PREFIX/include/nswarm` for headers.

## Thread Safety

This codebase uses an interleaved executor to run it's asynchronous functions, and thus,
when socket operations are occuring it is not safe to modify a socket from another
thread.Asynchronous read operations run frequently, so the only safe way to operate
on these sockets are within the thread in which their io_service is executing.

This means that a server and it's child connections can be altered on the same thread,
but if you bind a client to the server's io_service running in another thread, you will
encounter data races because the client's async functions are running in the server's
thread.

	ns::host::node_server m_server{6666};
	m_server.start();

	// client *must* be created without an external io_service
	auto client = std::make_shared<ns::node::upstream>();
	// ...
	// client->run();

	ns::io_service io;
	client = std::make_shared<ns::node::upstream>(io);
	// or an io_service on this thread
	// ...
	io.run();

In contrast, the following would be unsafe.

	ns::host::node_server m_server{6666};
	m_server.start();

	// client *must* be created without an external io_service
	auto client = std::make_shared<ns::node::upstream>(m_server.get_io_service());
	// ...
	client->run();
	client->close();

## Authors

* Kevin Morris &lt;kevr.gtalk@gmail.com&gt;

