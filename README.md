Virtual PPG Yocto Layer
=======================

Instructions to integrate this layer to a Yocto distribution:
-------------------------------------------------------------

#### Clone the layer to Poky installation directory.
Starting from poky directory:
```
$git clone https://github.com/Edward-Manca/meta-virtualppg.git
```

#### Add the layer
Starting from your poky directory:
```
$source oe-init-build-env <your_target_machine>
$bitbake-layers add-layer ../meta-virtualppg
```
or properly configure the file `./conf/bblayers.conf`

#### Add the recipies to your build target
Starting from your build directory edit the file `./conf/local.conf`, for example:
```
$code ./conf/local.conf
```
In your editor interface add the following lines:
```
IMAGE_INSTALL_append += "heartrateapp"
IMAGE_INSTALL_append += "vppgmod"
KERNEL_MODULE_AUTOLOAD += "vppgmod"
```
In a RaspberryPI distribution, according to my tests, it was necessary to add also the following line to create the image in `.rpi-sdimg` format:
```
IMAGE_FSTYPES += "rpi-sdimg"
```

#### Check the compatibility of Virtual PPG kernel module with your target machine
The kernel module is compatible with a generic "qemuarm" machine and a RaspberryPI machine named "raspberrypi3", it is tested via qemu emulator and RaspberryPI 3 board, to make it compatible with your machine you may have to edit the `vppgmod.bb` file, to do this starting from `meta-virtualppg` directory:
```
$cd recipies-virtualppg/vppgmod
$code vppgmod.bb
```
In your editor interface add or change according to the following line:
```
COMPATIBLE_MACHINE += "name_of_your_machine"
```
It should be compatible also with other machine types and other boards following the same steps on this guide, but this must be tested.

#### Build the new image
Ensure you have execute the following script from poky directory:
```
$source oe-init-build-env <your target machine>
```
Then from your build directory:
```
$bitbake core-image-full-cmdline
```
or every other compatible image types you are using.

Check the kernel module and execute the application:
--------------------------------------------------------

#### Check if the Virtual PPG kernel module is loaded properly into the kernel:
In the kernel messages it is possible to see messages related to vppgmod module, in particular, if the module is loaded correctly some messages are present and also the major numer is shown to the user. Instead if the module is not loaded correctly a brief explanation of the error is then shown. To see all kernel messages during the boot procedure it is possible to use the following command:
```
$cat /var/log/dmesg
```
To print more information through kernel messages open the file `vppgmod.c`, in `meta-virtualppg/recipes-virtualppg/vppgmod/files` directory, and uncomment the first line `#define DEBUG`, in this way all debugging messages will be printed and every message will contain also the function name that printed it.

#### Run the heart rate application
The application is added in `/usr/bin/` directory with the name `HeartRateApp`
The application will print the heart rate starting from virtual data, the correct value is always 76 bpm, it requires around 40 second to print the first results since it acquires 2048 values from the driver at a 50 Hz frequency; it is possibile to change data starting from `meta-virtualppg/recipes-virtualppg/vppgmod/files/data.h` definition.
