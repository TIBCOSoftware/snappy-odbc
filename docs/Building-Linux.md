### System Requirements

Like the rest of SnappyData source tree, the build system used is gradle with its [native](https://docs.gradle.org/5.6.4/userguide/native_software.html) build capabilities that allows uniform command-line builds for both Linux and Windows.

The default driver build needs and supports unixODBC. Though iODBC support has been provided (using the `-Piodbc=1` option to `gradlew`), it is not supported and untested.

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

The driver with its dependencies should build on any fairly recent Linux distribution having GNU g++ compiler >= v4.8.1 having C++11 language support. On older distributions it should be even possible to compile a fairly recent GNU g++ first. So it should be possible to build the C++ and ODBC drivers on nearly all available Linux distributions though one may need to manually build the dependencies first and update the build script to point to the same. For other distributions known to be compatible with one of those listed above, see the `linuxFlavour` option in the "Build Steps" section below.

The driver build and testing has a few binary dependencies (Boost, Thrift, GoogleTest) which are downloaded automatically by gradle from the snappy-odbc github releases page. It also needs OpenSSL, GMP and unixODBC libraries to be installed. On Debian and Ubuntu based distributions:

```
apt-get install libssl-dev libgmp-dev unixodbc-dev
```

On CentOS/Redhat/Fedora based distributions use:

```
yum install openssl-devel gmp-devel unixODBC-devel
```

On Arch based distributions:

```
pacman -S openssl gmp unixodbc
```

Not that when building both 64-bit and 32-bit drivers, you will also need to install 32-bit versions of the above libraries -- refer to your distribution's documentation (e.g. on debian/ubuntu use `libssl-dev:i386` etc).


### Build Steps

* Checkout snappy-odbc repository in some directory say DIR: `git clone https://github.com/tibco/snappy-odbc.git`

* The snappy-odbc build assumes that top-level snappydata is checked out at the same level as snappy-odbc, so that the relative path of snappy-store repository is `../snappydata/store`. So you can either checkout the entire snappydata repository at the same level as snappy-odbc. Choose one of the options below (working directory assumed to be same as DIR above):
  - `git clone https://github.com/TIBCOSoftware/snappydata.git --recursive`
  - OR just checkout snappy-store while creating empty snappydata repository: `mkdir snappydata && cd snappydata && git clone --branch snappy/master https://github.com/TIBCOSoftware/snappy-store.git store`
  - OR checkout just snappy-store somewhere and set the environment variable SNAPPYSTORE to point to it for all the other build steps below: `git clone --branch snappy/master https://github.com/TIBCOSoftware/snappy-store.git`

* To build just 64-bit ODBC driver (release and debug versions), run `./gradlew product` from within snappy-odbc checkout directory. To build both 64-bit and 32-bit drivers (release and debug versions for both), run `./gradlew product -PbothArch=1`. For all other gradlew invocations `-PbothArch=1` needs to be passed if both the architectures are built. If you are going to be dealing with only the final builds to be released, it is better to set it permanently in your gradle.properties (i.e. add the line `bothArch=1` to it). The global gradle.properties is located in `.gradle/gradle.properties` in your home (in bash shell it can be accessed as `~/.gradle/gradle.properties`).

* For cases the distribution is not one of the listed ones but is known to be compatible with one of the supported ones listed above, you can add the option `-PlinuxFlavour` like: `./gradlew product -PlinuxFlavour=ubuntu20 -PbothArch=1`. See the [native/build.gradle](https://github.com/TIBCOSoftware/snappy-store/blob/snappy/master/native/build.gradle) file in snappy-store repository for the available values.

The ODBC driver can be found in `build-artifacts/lin/snappyodbc` under the subdirectories `linux64` and `lin32` for 64-bit and 32-bit driver respectively. The `debug` subdirectory inside those will have the driver with full debugging symbols while the other one outside has minimal debugging symbols (to just enable proper stack traces etc).

To install the driver and DSNs, copy the template ini files in `src/driver`, uncomment and change as per your requirements and append to your odbc configuration files. Take note of the `Driver` and `Setup` options in the `odbcinst.ini` file which should point to the driver shared libries built in the steps before. So you can copy `src/driver/odbcinst.ini.template`, edit it as required, then append to either `/etc/odbcinst.ini` or to `$HOME/.odbcinst.ini` (note that for latter you also need to set `ODBCSYSINI` environment variable to the path of that file for unixODBC, and `ODBCINSTINI` environment variable when using iODBC). Likewise you can copy and edit `src/driver/odbc.ini.template`, then append to `/etc/odbc.ini` or to `$HOME/.odbc.ini`.

The ODBC `Connect/DriverConnect` URLs can use DSNs defined above or explicit property list in `DriverConnect` like shown in documentation [here](https://tibcosoftware.github.io/snappydata/howto/connect_using_odbc_driver/#connecting-to-the-snappydata-cluster) or [here](https://tibcosoftware.github.io/snappydata/security/authentication_connecting_to_a_secure_cluster/#using-odbc-driver). The new features listed [here](https://tibcosoftware.github.io/snappydata/release_notes/release_notes/#odbc-driver) apply to Linux case too.
