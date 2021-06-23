### System Requirements

Like the rest of SnappyData source tree, the build system used is gradle with its [native](https://docs.gradle.org/5.6.4/userguide/native_software.html) build capabilities that allows uniform command-line builds for both Linux and Windows.

Out of the box, binaries for the required dependencies for the driver (boost, thrift with custom additions) are provided for the following distributions which thus form the primary supported platforms.

* CentOS 7 / Redhat Enterprise Linux 7
* CentOS 8 / Redhat Enterprise Linux 8
* Ubuntu 18.04
* Ubuntu 20.04
* Arch / Manjaro Linux (May 2021 snapshot)

The gradle build script adds support for the following distributions that are compatible with above:

* Fedora and compatible distributions newer than RHEL8
* Ubuntu 18.10, 19.04, 19.10, 20.10
* Ubuntu 21.x should work though newest releases have not been tested
* Linux Mint 19.x, 20.x
* PopOS 18.04, 20.04, 20.10
* Garuda Linux May 2021 snapshot and later
* Elementary OS 5.x

The driver with its dependencies should build on any fairly recent Linux distribution having GNU g++ compiler >= v4.8.1 having C++11 language support. On older distributions it should be even possible to compile a fairly recent GNU g++ first. So it should be possible to build the C++ and ODBC drivers on nearly all available Linux distributions though one may need to manually build the dependencies first and update the build script to point to the same. In many other cas
