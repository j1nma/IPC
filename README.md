# IPC 

This project consists of learning how to use different IPCs in a POSIX system. In that manner, a system that distributes tasks has been implemented on Ubuntu.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

What things you need to install the software and how to install them

* md5sum
* openssl


### Installing

Inside IPC folder:

```
cd view && make && cd .. && make
```

Add the folder of files you wish to hash into IPC/bin.

If you wish to run on macOS:

1) Open makefile located at IPC and erase the last two libraries (lrt lpthread) from LFLAGS line. Result:
```
LFLAGS = -L /usr/local/opt/openssl/lib -lcrypto -lssl
```
2) Do the same for the makefile located at IPC/view

## Running inside IPC:

To hash files from the 'testFolder' folder:
```
cd bin && ./hash testFolder
```

To view files hashed at the moment:
```
cd bin && ./hash testFolder | ./view
```

## Authors

* **Juan Manuel Alonso** - (https://github.com/j1nma)
* **Ioni Zelig Nielavitzky** - (https://github.com/jnielavitzky)
* **Nicolás Paganini** - (https://github.com/npaganini)
* **Martina Scomazzon** - (https://github.com/mscomazzon)

## License

MIT License

Copyright (c) [2017] [Alonso, Juan Manuel; Nielavitzky, Ioni Zelig; Scomazzon, Martina; Paganini, Nicolás]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
