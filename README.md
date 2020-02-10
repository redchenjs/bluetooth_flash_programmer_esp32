SPP Flash Programmer
====================

Bluetooth SPP flash programmer based on ESP32 chip.

## Pinmap

| FLASH | CS | SCLK | MOSI | MISO |
| :---- | -: | ---: | ---: | ---: |
| ESP32 | 15 |   14 |   13 |   12 |

## Preparing

### Obtain the Source

```
git clone --recursive https://github.com/redchenjs/spp_flash_programmer_esp32.git
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

* All project configurations are under the `SPP Flash Programmer` menu.

### Flash & Monitor

```
idf.py flash monitor
```
