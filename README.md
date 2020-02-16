Easy Flash Programmer
=====================

Bluetooth Easy Flash Programmer based on ESP32 chip.

## Pinmap

| FLASH | CS | SCLK | MOSI | MISO |
| :---- | -: | ---: | ---: | ---: |
| ESP32 | 15 |   14 |   13 |   12 |

## Commands

* `MTD+ERASE:ALL!`: Erase Full Flash Chip
* `MTD+ERASE:0x%x+0x%x`: Erase Flash: Addr Length
* `MTD+WRITE:0x%x+0x%x`: Write Flash: Addr Length
* `MTD+READ:0x%x+0x%x`: Read Flash: Addr Length
* `MTD+INFO?`: Flash Info

## Preparing

### Obtain the Source

```
git clone --recursive https://github.com/redchenjs/easy_flash_programmer_esp32.git
```

### Update an existing repository

```
git pull
git submodule update --init --recursive
```

### Setup the Tools

```
./esp-idf/install.sh
```

## Building

### Setup the environment variables

```
export IDF_PATH=$PWD/esp-idf
source ./esp-idf/export.sh
```

### Configure

```
idf.py menuconfig
```

* All project configurations are under the `Easy Flash Programmer` menu.

### Flash & Monitor

```
idf.py flash monitor
```
