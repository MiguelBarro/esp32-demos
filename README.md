# Examples pick up from expressif/esp-idf repos and modify to work both on platformio and sdk v5.0.2

In order to avoid installing the SDK twice we can reuse the platformio deployed version.

+ Install the deployed sdk by running:

```
PS> C:\Users\<your user>\.platformio\packages\framework-espidf\install.ps1
```
note that python.exe must be accessible from the `$Env:PATH`.

+ To load the ESP32-IDF environment in a terminal run:

```
PS> C:\Users\<your user>\.espressif\python_env\idf5.0_py3.11_env\Scripts\Activate.ps1
PS> C:\Users\<your user>\.platformio\packages\framework-espidf\export.ps1
```

## Building a project

For example the hello_world. Using CMake:

```
esp32-demos> cmake -B build/hello_world -G Ninja .\hello_world\
esp32-demos> cmake --build build/hello_world --target menuconfig
esp32-demos> cmake --build build/hello_world --target all
esp32-demos> cmake --build build/hello_world --target flash

esp32-demos> [System.IO.Ports.SerialPort]::getportnames()
    COM3
esp32-demos> $ENV:ESPPORT = "COM3"
esp32-demos> cmake --build build/hello_world --target monitor
```

Using python:

```
esp32-demos> $src = "-C hello_world", "-B build/hello_world"
esp32-demos> idf.py $src menuconfig
esp32-demos> idf.py $src all
esp32-demos> idf.py $src flash
esp32-demos> idf.py $src monitor
```

## Example settings

[schematic]: https://raw.githubusercontent.com/OLIMEX/ESP32-GATEWAY/master/HARDWARE/Hardware%20revision%20G/ESP32-GATEWAY_Rev_G.pdf
[gateway]: https://github.com/OLIMEX/ESP32-GATEWAY.git

All the examples are set up for an [Olimex ESP32 Gateway][gateway].
For the settings understanding are very helpful:
- the explanations associated to the examples in the [Olimex Gateway repo](https://raw.githubusercontent.com/OLIMEX/ESP32-GATEWAY/master/SOFTWARE/README.md).
- the [board schematic][schematic] showing the electrical connections and ancillary chipsets.

### blink example

The only set up is on the example side `CONFIG_BLINK_GPIO=33`. According with the [board schematic](1) there is
only a single user settable led connected to GPIO 33.

### led button example

The only set up is on the example side. According with the [board schematic](1):
- there is only a single user settable led connected to GPIO 33.
- there is only a single user settable button connected to GPIO 34.

```
#define OLIMEX_LED_PIN      33
#define OLIMEX_BUT_PIN      34
```

### ethernet basic example

According with the [board schematic](1) the ethernet PHY chip is a LAN8710A that:
- expects [RMII](https://en.wikipedia.org/wiki/Media-independent_interface#RMII) use because its 9 pin (RMIISEL) is connected to VCC (+3.3V). 
- in order to use RMII is necessary to synchronize the PHY chip and the micro. The EMAC clock must be exported through
  the GPIO 17 which is the one connected to the PHY chip pin 5 (CLKIN). 
- physical address is 0 because the PHY chip pins 13 (PHYAD0), 7 (PHYAD1) and 8 (PHYAD2) are grounded.
- all PHY chip EMAC pins are connected to the ESP32, thus we must use the micro internal MAC.

The above board requirements are translated into the following configs:
- micro:
```
    CONFIG_ETH_USE_SPI_ETHERNET=n
    CONFIG_ETH_ENABLED=y
    CONFIG_ETH_USE_ESP32_EMAC=y
    CONFIG_ETH_PHY_INTERFACE_RMII=y
    CONFIG_ETH_RMII_CLK_OUTPUT=y
    CONFIG_ETH_RMII_CLK_OUT_GPIO=17
```
- example:
```
    CONFIG_EXAMPLE_ETH_PHY_ADDR 0
    CONFIG_EXAMPLE_ETH_PHY_RST_GPIO 5
    CONFIG_EXAMPLE_ETH_MDC_GPIO 23
    CONFIG_EXAMPLE_ETH_MDIO_GPIO 18
    EMAC_CLK_OUT 2
    EMAC_CLK_OUT_180_GPIO 17
    CONFIG_EXAMPLE_ETH_PHY_LAN87XX 1
    CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET 1
```
