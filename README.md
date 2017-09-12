# IPC 

This project consists of learning how to use different IPCs in a POSIX system. In that manner, a system that distributes tasks has been implemented on macOS.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

What things you need to install the software and how to install them

* md5sum
* openssl


### Installing

Inside IPC folder:

```
make
```

If you are running on Linux, edit the makefile located under IPC. Add to the LFLAGS line:

```
-lrt -lpthread
```

Add the folder you wish to hash into IPC/bin

## Running inside /bin:

```

```

## Authors

<!-- * **Billie Thompson** - *Initial work* - [PurpleBooth](https://github.com/PurpleBooth) -->

See the list of [contributors](https://github.com/j1nma/IPC/contributors) who participated in this project.

## License

## Acknowledgments

* Volkan Yazıcı https://github.com/vy/libpqueue
