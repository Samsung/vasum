# Vasum
[Vasum](https://wiki.tizen.org/wiki/Security:Vasum) is a Linux daemon and a set of utilities used for managing para-virtualization. It uses Linux Containers to create separate, graphical environments called *zones*. One can concurrently run several zones on one physical device. Vasum exports a rich C/Dbus API that the application frameworks can use to interact with zones. 

For now Vasum uses [LXC](https://linuxcontainers.org/lxc/introduction/) for Linux Containers management. The project is mostly written in modern C++, is [well tested](https://wiki.tizen.org/wiki/Weekly_test_results_for_Tizen_3.X_security_framework).

Vasum's development takes place on [review.tizen.org/gerrit/](http://review.tizen.org/gerrit/) (registration on [tizen.org](http://tizen.org) is required).

## Installation and usage
The installation process and simple verification is described [here](https://wiki.tizen.org/wiki/Security:Vasum:Usage).

## Client interface
Vasum daemon can be accessed via C API or Dbus. You can find the API documentation [here](https://wiki.tizen.org/wiki/Security:Vasum:API). Be aware that the API will most likely change in the near future.

## Documentation
More comprehensive documentation is kept [here](https://wiki.tizen.org/wiki/Security:Vasum). You can generate the code documentation by executing *generate_documentation.sh* script from *doc* directory. Documentation will be generated in doc/html directory.

    cd ./doc
    ./generate_documentation.sh


## Code formatting
We use [astyle](http://astyle.sourceforge.net/) for code formatting (Use the latest version)
You can find the options file in the root of the project.

For example to format all .cpp and .hpp files run in the project directory:

    astyle --options=./astylerc --recursive ./*.cpp ./*.hpp