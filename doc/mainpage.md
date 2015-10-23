Introduction {#mainpage}
============

[Vasum](https://wiki.tizen.org/wiki/Security:Vasum) is a Linux daemon and a set of utilities used for managing para-virtualization. It uses Linux Containers to create separate, graphical environments called the *zones*. One can concurrently run several zones on one physical device. Vasum exports a rich C/DBus API that the application frameworks can use to interact with zones.


For now Vasum uses [LXC](https://linuxcontainers.org/lxc/introduction/) for Linux Containers management. The project is mostly written in modern C++ and is [well tested](https://wiki.tizen.org/wiki/Weekly_test_results_for_Tizen_3.X_security_framework).

Vasum's development takes place on [review.tizen.org/gerrit/](http://review.tizen.org/gerrit/) (registration on [tizen.org](http://tizen.org) is required).

## Vasum demo
Fedora 22 Desktop running in Linux container on [youtube](http://www.youtube.com/watch?v=hsNvI9kHTvI)

## Installation and usage

The installation process and simple verification is described [here](https://wiki.tizen.org/wiki/Security:Vasum:Usage "Vasum on Tizen").

## API

Vasum daemon can be accessed via C API or DBus. You can find the API documentation [here](https://wiki.tizen.org/wiki/Security:Vasum:API). Be aware that the API will most likely change in the near future.
