# MQTT esp32 client example

This example generates a basic ESP32 MQTT client for an Olymex Gateway Rev. G board. 

## Overview

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

## How to use example

Set up MQTT server and clients to test the ESP32 mqtt client. For a windows computer, for example, this involves:
 + Downloading [mosquitto](https://github.com/eclipse/mosquitto) binaries from [here](https://mosquitto.org/files/binary/win64/mosquitto-2.0.17-install-windows-x64.exe). 
 + Set up the firewall to allow inbound traffic from the ESP32. In this case we are using a special port (6338)
   different from the usual 1883.
```powershell
    > $server = @{
        DisplayName = "Mosquitto testing"
        Direction = "Inbound"
        Program = (ls -Path $env:ProgramFiles/mosquitto/* -R -Filter mosquitto.exe).fullname
        Action = "Allow"
        Enabled = "True"
        Protocol = "TCP"
        LocalPort = 6338
    }

    > New-NetFirewallRule @server
```
   Note that windows firewall already is set up for *mDNS* traffic.
 + Launch the *mosquitto server* using an ad hoc config file config to allow anonymous connections, test.conf:
```
    listener 6338 0.0.0.0
    log_dest stderr
    allow_anonymous true
```
   from cli that is:
```powershell
    > mosquitto -c $Env:TMP\test.conf -v
```
   launch a client publisher that maches the `esp32/subscribe` topic.
```powershell
    > mosquitto_pub -h localhost -p 6338 -t esp32/subscribe -m "hello world!" -q 1 --repeat 100 --repeat-delay 1
```
   launch a client subscriber that maches the `esp32/publisher` topic.
```powershell
    > mosquitto_sub -h localhost -p 6338 -t esp32/publish -q 1
```

 + Connect the *Olimex board* ethernet's socket. And reset it. After a few seconds (DHCP assigns address and the board
   resolves the server hostname) both the *mosquitto server* and *mosquito_sub* should start showing esp32 activity.
   Launch the monitor in order to check if the ESP32 is able to receive the publisher topic (use ESP-IDF terminal):
    ```powershell
        > $ENV:ESPPORT = "COM3" # Olimex board assigned port 
        > cmake --build <project binary build dir> --target monitor
    ```
    or resorting to the idf framework directly:
    ```powershell
        > $ENV:ESPPORT = "COM3" # Olimex board assigned port 
        project binary source dir> idf.py monitor -p COM3
    ```

### Configure the project

From an ESP-IDF environment, either:
``` powershell
> cmake --build <project binary build dir> --target menuconfig
```
or
``` powershell
project binary source dir> idf.py menuconfig
```
The ethernet options are set up like in the [ethernet basic example](../../ethernet_basic/README.md#Configure-the-project).
The example options are:
 + *CONFIG_BROKER_HOSTNAME* which in my case is my workstation hostname.
 + *CONFIG_BROKER_PORT* which according with the firewall rule is 6338.

### Build, Flash, and Run

Build the project and flash it to the board, then run monitor tool to view serial output.
From an ESP-IDF environment, either:
``` powershell
> cmake --build <project binary build dir> --target flash
> $ENV:ESPPORT = "COM3" # Olimex board assigned port 
> cmake --build <project binary build dir> --target monitor
```
or
``` powershell
<project source dir> idf.py -p COM3 -B <project binary build dir> build flash monitor
```

### Hostname resolution issues

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

## Example Output

+ mosquitto server:
```
...
1695634578: Received PUBACK from auto-6C19F6A4-179C-B868-9207-88753FA91D4C (Mid: 4383, RC:0)
1695634579: Received PUBLISH from auto-8D573B54-A8FC-D7B8-040D-FFABA28E2457 (d0, q1, r0, m3043, 'esp32/subscribe', ... (12 bytes))
1695634579: Sending PUBLISH to ESP32_665E1C (d0, q0, r0, m0, 'esp32/subscribe', ... (12 bytes))
1695634579: Sending PUBACK to auto-8D573B54-A8FC-D7B8-040D-FFABA28E2457 (m3043, rc0)
...
```
+ mosquitto sub:
```
6
7
8
9
10
11
12
```
+ ESP32 monitor:
```
I (4346128) MQTT_EXAMPLE: publishing data
I (4346128) MQTT_EXAMPLE: MQTT_EVENT_PUBLISHED, msg_id=3495
I (4347128) MQTT_EXAMPLE: publishing data
I (4347128) MQTT_EXAMPLE: MQTT_EVENT_PUBLISHED, msg_id=28923
I (4347328) MQTT_EXAMPLE: MQTT_EVENT_DATA
TOPIC=esp32/subscribe
DATA=hello world!
```

Now it should be possible to resolve the actual ESP32 name as set up in the example through the mDNS service in the
call:
```cpp
    // set hostname
    mdns_hostname_set("esp32-mqtt-test");
```
by doing:
```powershell
> Resolve-Host esp32-mqtt-test

HostName              Aliases AddressList
--------              ------- -----------
esp32-mqtt-test.local {}      {192.168.0.26}
```
