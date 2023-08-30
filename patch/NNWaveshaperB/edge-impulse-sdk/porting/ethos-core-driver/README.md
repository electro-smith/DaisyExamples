# Arm(R) Ethos(TM)-U core driver

This repository contains a device driver for the Arm(R) Ethos(TM)-U NPU.

## Building

The source code comes with a CMake based build system. The driver is expeced to
be cross compiled for any of the supported Arm Cortex(R)-M CPUs, which will
require the user to setup a custom toolchain file.

The user is also required to define `CMAKE_SYSTEM_PROCESSOR` for the target CPU,
for example cortex-m55+nodsp+nofp. This can be done either in the toolchain
file or on the command line.

```
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=<toolchain> -DCMAKE_SYSTEM_PROCESSOR=cortex-m<nr><features>
$ make
```

For running the driver on Arm CPUs which are configured with datacache, the
cache maintenance functions in the driver are exported with weakly linked
symbols that should be overriden. An example implementation using the CMSIS
primitives found in cachel1_armv7.h could be as below:

```
extern "C" {
void ethosu_flush_dcache(uint32_t *p, size_t bytes) {
    if (p)
        SCB_CleanDCache_by_Addr(p, bytes);
    else
        SCB_CleanDCache();
}

void ethosu_invalidate_dcache(uint32_t *p, size_t bytes) {
    if (p)
        SCB_InvalidateDCache_by_Addr(p, bytes);
    else
        SCB_InvalidateDCache();
}
}
```

# License

The Arm Ethos-U core driver is provided under an Apache-2.0 license. Please see
[LICENSE.txt](LICENSE.txt) for more information.

# Contributions

The Arm Ethos-U project welcomes contributions under the Apache-2.0 license.

Before we can accept your contribution, you need to certify its origin and give
us your permission. For this process we use the Developer Certificate of Origin
(DCO) V1.1 (https://developercertificate.org).

To indicate that you agree to the terms of the DCO, you "sign off" your
contribution by adding a line with your name and e-mail address to every git
commit message. You must use your real name, no pseudonyms or anonymous
contributions are accepted. If there are more than one contributor, everyone
adds their name and e-mail to the commit message.

```
Author: John Doe \<john.doe@example.org\>
Date:   Mon Feb 29 12:12:12 2016 +0000

Title of the commit

Short description of the change.
   
Signed-off-by: John Doe john.doe@example.org
Signed-off-by: Foo Bar foo.bar@example.org
```

The contributions will be code reviewed by Arm before they can be accepted into
the repository.

# Security

Please see [Security](SECURITY.md).

# Trademark notice

Arm, Cortex and Ethos are registered trademarks of Arm Limited (or its
subsidiaries) in the US and/or elsewhere.
