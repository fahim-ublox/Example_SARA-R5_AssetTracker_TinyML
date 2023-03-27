# TinyML firmware using u-blox product and libraries
## Description
You can use tensorflow lite for microcontroller using u-blox products. This project is being added as example program to help u-blox customers for rapid prototyping and evaluating u-blox products. This repository is tested using C030-R412M board and Sparkfun's Asset tracker Micromod setup using STM32F405 microcontroller and SARA-R5 cellular CATM1 module.

This repo uses Git submodules: make sure that once it has been cloned you do something like:
```sh
 git submodule update --init --recursive
```

### Build using build configuration through CMake.

** Use following commands to build the firmware. **
```sh
cmake -S . -B build/debug --warn-uninitialized -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_TOOLCHAIN_FILE=toolchain-STM32F405.cmake -GNinja
cmake --build build/debug
```

Flash the hex file.

### Hardware requirements
| HW type | Name |
| ------ | ------ |
| Carrier Board | SparkFun MicroMod Asset Tracker |
| Application board | MicroMod_STM32_Processor |
| Cellular | SARA-R5 Module |
| Microcontroller | STM32F405RGT6 |

### Build requirements
| Toolchain or Build Software | Version |
| ------ | ------ |
| CMake | 3.25.1 |
| Ninja | 1.11.1 |
| GNU Tools for ARM Embedded Processors | 6-2017-q2-update |

### Memory requirements of Firmware
| Memory region |  Used Size   |	Region Size 	|	%age Used |
| ------ | ------ | ------ |  ------  | 
| CCMRAM        |  36 B        |		64 KB      	|	0.05%     |
| RAM       	  |  51464 B     |  	128 KB     	|	39.26%    |
| FLASH      	  |  480004 B    |  	1 MB     	|	45.78%    |

