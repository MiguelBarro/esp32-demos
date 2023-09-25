| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# Ethernet Example
(See the README.md file in the upper level 'examples' directory for more information about examples.)

## Overview

This example demonstrates basic usage of `Ethernet driver` together with `esp_netif`. Initialization of the `Ethernet driver` is wrapped in separate [sub-component](./components/ethernet_init/ethernet_init.c) of this project to clearly distinguish between the driver's and `esp_netif` initializations. The work flow of the example could be as follows:

1. Install Ethernet driver
2. Attach the driver to `esp_netif`
3. Send DHCP requests and wait for a DHCP lease
4. If get IP address successfully, then you will be able to ping the device

If you have a new Ethernet application to go (for example, connect to IoT cloud via Ethernet), try this as a basic template, then add your own code.

### Configure the project

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

All this can be setup doing:

```
idf.py menuconfig
```

See common configurations for Ethernet examples from [upper level](../README.md#common-configurations).

### Build, Flash, and Run

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT build flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

```bash
I (394) eth_example: Ethernet Started
I (3934) eth_example: Ethernet Link Up
I (3934) eth_example: Ethernet HW Addr 30:ae:a4:c6:87:5b
I (5864) esp_netif_handlers: eth ip: 192.168.2.151, mask: 255.255.255.0, gw: 192.168.2.2
I (5864) eth_example: Ethernet Got IP Address
I (5864) eth_example: ~~~~~~~~~~~
I (5864) eth_example: ETHIP:192.168.2.151
I (5874) eth_example: ETHMASK:255.255.255.0
I (5874) eth_example: ETHGW:192.168.2.2
I (5884) eth_example: ~~~~~~~~~~~
```

Now you can ping your ESP32 in the terminal by entering `ping 192.168.2.151` (it depends on the actual IP address you get).

## Troubleshooting

See common troubleshooting for Ethernet examples from [upper level](../README.md#common-troubleshooting).

(For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you as soon as possible.)
