# ESP32 protobuf over mqtt client

## Overview

The purpose of this example is generate an ESP32 client capable of publish and subscribe in the topics:
    - publishes on topic `esp32/publish`
    - subscribes on topic `esp32/subscribe`
And send a specific message encoded in [protobuf](https://protobuf.dev/overview/). This message is defined in the folder
proto.

The example basically:
 + sets up the ethernet driver using the specific board PHY settings.
 + once the ethernet driver is assigned via DHCP an IP address it checks the DNS and starts an external mDNS service in
   order to be able to resolve hostnames from the local network (we will se later the current lwIP framework cannot do
   it by itself).
 + the example tries to resolve the hostname address passed as an example option `BROKER_HOSTNAME` using mDNS. If it
   fails then it passes the unresolved hostname to the MQTT client (this will relay it to lwIP for DNS name resolution).
 + the example:
    - publishes on topic `esp32/publish`
    - subscribes on topic `esp32/subscribe`
 + dummy data is randomly generated and encoded into *protobuf* using the ESP-IDF builtin protobuf-c library.

The communication loop is closed with a desktop client example provided in the same folder.

## How to build

Set up the dependencies. On linux and Mac there are package managers that readily provide both protobuf and protobuf-c
(apt & brew).
On windows download the binaries from [here](https://github.com/MiguelBarro/protobuf-c/suites/16478201319/artifacts/941704108).
Preferable the static linked ones.

For simplicity sake the message proxy-stub generation is integrated into the cmake build stage using a [Custom CMake
Project](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#using-esp-idf-in-custom-cmake-projects).
On Windows we must hint CMake where to find the dependencies using **_ROOT** variables.

## Generate the project:

From an ESP-IDF console environment:

```powershell
> cmake -DCMAKE_BUILD_TYPE=Release `
        -Dprotobuf_ROOT=<install-path>/protobuf `
        -Dprotobuf-c_ROOT=<install-path>/protobuf-c `
        -B $Env:TMP/mqtt_esp32 -G Ninja mqtt/esp32_proto_client
```

## Once generated open the configuration menu:

```powershell
> cmake --build $Env:TMP/mqtt_esp32 --target menuconfig
```

There:
    + under `Example MQTT Configuration` the mqtt broker hostname and listening port can be set up.
    + in the `components` submenu under `Example Ethernet Configuration` is possible to specify the Ethernet PHY setup.
      For an Olimex Gateway board the set up will be:
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

## build+flash the set up project

```powershell
> cmake --build $Env:TMP/mqtt_esp32 --target flash
```
or
``` powershell
> idf.py -p COM3 -B $Env:TMP/mqtt_esp32  monitor
```

## Monitor the board operation

```powershell
> $ENV:ESPPORT = "COM3" # Olimex board assigned port 
> cmake --build $Env:TMP/mqtt_esp32 --target monitor
```
or
``` powershell
> idf.py -p COM3 -B <project binary build dir> monitor
```

## Hostname resolution issues

Hostname resolution is usually given for granted if all the computers in the local network run the same OS but it can be
troublesome otherwise as explained [here](https://www.eiman.tv/blog/posts/lannames/). ESP32 doesn't even run an
operative system ... let's explained the issue:
+ socket libraries like [Berkeley sockets](https://en.wikipedia.org/wiki/Berkeley_sockets) or
[winsocks](https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-start-page-2) are supposed to resolve
both DNS names and host names. But only the DNS resolution is standarized. Been non-standard host name resolution
between different OS is an issue. <br>
My educated guess is that corporate networks doesn't suffer from this because all computers have an actual DNS name
avoiding the host name resolution completely. This is the case for example of Microsoft Domains where Active Directory's
DNS Server will resolve any computer on the network. <br>
+ DNS fallbacks are OS dependent:
    - On windows is explained in [TCP/IP Fundamentals for Microsoft Windows](https://download.microsoft.com/download/9/4/6/946958ef-7b86-4ddc-bfdb-c7ed2af4ce51/TCPIP_Fund.pdf) 
    on Chapter 7 (Host Name Resolution).
    On Chapter 9 (Windows Support for DNS) we discover that only domain joined computers can actually be reach using DNS.  
    If DNS is not available windows fallsback into [NetBIOS](https://timothydevans.me.uk/nbf2cifs/nbf-addressing.html)
    (old timer already deprecated) and [LLMNR](https://learn.microsoft.com/en-us/previous-versions//bb878128(v=technet.10)?redirectedfrom=MSDN)
    which uses multicast to query DNS-style the other computers.
    LLMNR (Local Link Multicast Name Resolution) is deprecated too due to security issues (query answers come back as
    unicast been vulnerable to spoofing). 
    LLMNR makes our non-domained joined windows computers talk to each other (we can check that doing:

        ```pwsh
        > Resolve-DNSName -Name DESKTOP-whatever -LlmnrFallback
        ```

        which works and:

        ```pwsh
        > Resolve-DNSName -Name DESKTOP-whatever -DnsOnly
        ```

        which doesn't).

+ On MacOs they ignored the security flawed LLMNR and develop their own protocol [mDNS](http://www.multicastdns.org/).
+ On linux most distros embraced mDNS.

The happy ending is that Windows 11 has already builtin support for mDNS, thus, all popular OS can interoperate with the
ESP32 flawlessly ... or not? According with [espressif docs](https://docs.espressif.com/projects/esp-idf/en/release-v5.1/esp32/api-guides/lwip.html)
and I quote:

> * **mDNS** uses a different implementation to the lwIP default **mDNS**
> (see [mDNS Service](https://docs.espressif.com/projects/esp-idf/en/release-v5.1/esp32/api-reference/protocols/mdns.html)), but
> lwIP can look up **mDNS** hosts using standard APIs such as `gethostbyname()` and the convention `hostname.local`, provided the
> [CONFIG_LWIP_DNS_SUPPORT_MDNS_QUERIES](https://docs.espressif.com/projects/esp-idf/en/release-v5.1/esp32/api-reference/kconfig.html#config-lwip-dns-support-mdns-queries)
> setting is enabled.

That's not true.
+ The [mDNS Service](https://github.com/espressif/esp-protocols/tree/master/components/mdns) implementation works. In
fact, is the one used in this example for hostname resolution.
+ The builtin lwIP implementation doesn't. And the explanation is easy, that implementation doesn't listen in the mDNS
multicast port. In the [actual
implementation](https://github.com/espressif/esp-lwip/blob/7896c6cad020d17a986f7e850f603e084e319328/src/core/dns.c)
there are not actual `igmp_joingroup_netif()` calls (unlike in the mDNS Service one). Thus, resolution is not possible.

This example uses the *mDNS Service* as component in order to workaround host name resolution.
