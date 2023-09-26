#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <inttypes.h>
#include <memory>
#include <vector>

// This must precede any framework header
#include <sdkconfig.h>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include <esp_log.h>

#include <esp_err.h>
#include <esp_eth.h>
#include <esp_event.h>
#include <esp_idf_version.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <mqtt_client.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <nvs_flash.h>

#include <lwip/netdb.h>

// External dependencies
#include <mdns.h>
#include <ethernet_init.h>
#include <gps.pb-c.h>

static const char* ETH_TAG = "eth_log";
static const char* MDNS_TAG = "mDNS_log";
static const char* TAG = "MQTT-protobuf example";

static const char* subscriber_topic = "esp32/gps/subscribe";
static const char* publisher_topic = "esp32/gps/publish";

// MQTT client handle
static esp_mqtt_client_handle_t mqtt_client = nullptr;
static bool mqtt_connected = false;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

// Generate random gps data
Gps__Coords random_gps_data()
{
    Gps__Coords msg = GPS__COORDS__INIT; // message static initialization

    // randomly populate the message
    std::time_t time = std::time(nullptr);
    std::srand(time);
    msg.device = std::rand();
    msg.latitudex1e7 = std::rand();
    msg.longitudex1e7 = std::rand();
    msg.altitudemillimetres = std::rand();
    msg.radiusmillimetres = std::rand() % 10000;
    msg.speedmillimetrespersecond = std::rand() % 100;
    msg.svs = std::rand() % 5;
    msg.timeutc = time;

    return msg;
}

// Serialize gps data into a buffer
std::vector<uint8_t> serialize_gps_data(const Gps__Coords& msg)
{
    std::vector<uint8_t> buf;      // Buffer to store serialized data
    size_t len;                  // Length of serialized data

    // Get packet size, note uint8_t are bytes in size
    len = gps__coords__get_packed_size(&msg);

    // allocate buffer
    buf.resize(len);

    // Serialize to buffer
    gps__coords__pack(&msg, buf.data());

    // Show the length of message
    ESP_LOGI(TAG, "Writing %d serialized bytes", len);

    // Show the buffer byte by byte
    ESP_LOG_BUFFER_HEXDUMP(TAG, buf.data(), len, ESP_LOG_INFO);

    return buf;
}

std::shared_ptr<Gps__Coords> deserialize_gps_data(const uint8_t* data, size_t len)
{
    using namespace std::placeholders;

    // Unpack the buffer to an internally allocated message
    std::shared_ptr<Gps__Coords> pmsg {
        gps__coords__unpack(nullptr, len, data),
        std::bind(gps__coords__free_unpacked, _1, nullptr)
        };

    return pmsg;
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    static int msg_id = 0;

    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        if (!msg_id)
        {
            msg_id = esp_mqtt_client_subscribe(mqtt_client, subscriber_topic, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        }

        mqtt_connected = true;
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        msg_id = 0;

        mqtt_connected = false;
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);

        if (event->data_len != event->total_data_len)
        {
            ESP_LOGE(TAG,
                     "There is message fragmentation!!!! size %d and total size %d",
                     event->data_len,
                     event->total_data_len);
        }

        // Special treatment for gps data
        if (!std::strcmp(event->topic, subscriber_topic))
        {
            auto gps = deserialize_gps_data((const uint8_t*)event->data, event->data_len);

            // Show the recovered contents
            ESP_LOGI(TAG, "Show new message contents:");
            ESP_LOGI(TAG, "Device: %lld", gps->device);
            ESP_LOGI(TAG, "Latitude: %ld", gps->latitudex1e7);
            ESP_LOGI(TAG, "Longitude: %ld", gps->longitudex1e7);
            ESP_LOGI(TAG, "Altitude: %ld", gps->altitudemillimetres);
            ESP_LOGI(TAG, "Radius: %ld", gps->radiusmillimetres);
            ESP_LOGI(TAG, "Speed: %ld", gps->speedmillimetrespersecond);
            ESP_LOGI(TAG, "Satellites: %ld", gps->svs);
            std::time_t time = gps->timeutc;
            ESP_LOGI(TAG, "Time: %s", std::ctime(&time));

        }
        else
        {
            ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
        }

        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void start_mdns_service()
{
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err)
    {
        ESP_LOGI(MDNS_TAG, "MDNS Init failed: %d\n", err);
        return;
    }

    //set hostname
    mdns_hostname_set("esp32-mqtt-test");
}

const char* resolveHostAndLog(const char* host_name);
const char* resolve_mdns_host(const char * host_name);

static void mqtt_app_start(void)
{
    const char* address = nullptr;

    // try actual network framework
    address = resolveHostAndLog(CONFIG_BROKER_HOSTNAME);
    if ( nullptr == address )
    {
        // Because internal mDNS is broken and cannot listen multicast fallback to the external one
        address = resolve_mdns_host(CONFIG_BROKER_HOSTNAME);

        if ( nullptr == address )
            address = CONFIG_BROKER_HOSTNAME; // fallback to mqtt
    }

    if ( nullptr == mqtt_client)
    {
        esp_mqtt_client_config_t mqtt_cfg = {
            .broker {
                .address {
                    .hostname = address,
                    .transport = MQTT_TRANSPORT_OVER_TCP,
                    .port = CONFIG_BROKER_PORT
                },
            },
        };
        mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

        /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
        esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);

        ESP_LOGI(TAG, "Start MQTT client loop");
        esp_mqtt_client_start(mqtt_client);
    }
    else
    {
        ESP_LOGI(TAG, "Reconnect MQTT client loop");
        esp_mqtt_client_reconnect(mqtt_client);
    }
}

void eth_check_and_set_dns(const char* new_dns)
{
    // count the interfaces

    ESP_LOGI(ETH_TAG, "Number of interfaces: %d", esp_netif_get_nr_of_ifs());

    esp_netif_t * interface = esp_netif_next(nullptr);
    if (interface)
    {
        esp_netif_dns_info_t dns_info{};
        esp_err_t error =  esp_netif_get_dns_info(interface, ESP_NETIF_DNS_MAIN, &dns_info);
        if ( ESP_OK == error )
        {
            ESP_LOGI(ETH_TAG, "DNS:" IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));

            if(!dns_info.ip.u_addr.ip4.addr && new_dns) // 0.0.0.0 is undefined
            {
                // Specify DNS
                dns_info.ip.type = 0; // ipv4
                dns_info.ip.u_addr.ip4.addr = esp_ip4addr_aton(new_dns);

                if(dns_info.ip.u_addr.ip4.addr) // 0.0.0.0 is undefined
                {
                    if( ESP_OK != esp_netif_set_dns_info(interface, ESP_NETIF_DNS_MAIN, &dns_info) )
                    {
                        ESP_LOGI(ETH_TAG, "Fail to set the dns:" IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
                    }

                    ESP_LOGI(ETH_TAG, "DNS server set to %s", new_dns);
                }
                else
                {
                    ESP_LOGI(ETH_TAG, "Try to set invalid dns: %s", new_dns);
                }
            }
        }
        else
        {
            ESP_LOGI(ETH_TAG, "esp_netif_get_dns_info failed with %d", error);
        }
    }
    else
    {
        ESP_LOGI(ETH_TAG, "No ethernet interface available");
    }
}

const char* resolve_mdns_host(const char * host_name)
{
    ESP_LOGI(MDNS_TAG, "Query A: %s.local", host_name);

    esp_ip4_addr_t addr{};

    esp_err_t err = mdns_query_a(host_name, 2000,  &addr);
    if(err){
        if(err == ESP_ERR_NOT_FOUND){
            ESP_LOGI(MDNS_TAG, "Host was not found!");
            return nullptr;
        }
        ESP_LOGI(MDNS_TAG,"Query Failed");
        return nullptr;
    }

    ESP_LOGI(MDNS_TAG,"Hostname: %s resolved via mDNS to " IPSTR, host_name, IP2STR(&addr));
    return inet_ntoa(addr);
}

const char* resolveHostAndLog(const char* host_name)
{
    hostent* remoteHost;
    ESP_LOGI(TAG, "Calling gethostbyname with %s", host_name);
    remoteHost = lwip_gethostbyname(host_name);

    if (nullptr == remoteHost) {
        ESP_LOGI(TAG, "gethostbyname call failed");
        return nullptr;
    }

    ESP_LOGI(TAG, "Function returned:");
    ESP_LOGI(TAG, "\tOfficial name: %s", remoteHost->h_name);

    int i = 0;
    for (auto pAlias = remoteHost->h_aliases; *pAlias != 0; pAlias++) {
        ESP_LOGI(TAG,"\tAlternate name #%d: %s", ++i, *pAlias);
    }
    ESP_LOGI(TAG,"\tAddress type: ");
    switch (remoteHost->h_addrtype) {
    case AF_INET:
        ESP_LOGI(TAG,"AF_INET");
        break;
    default:
        ESP_LOGI(TAG," %d", remoteHost->h_addrtype);
        break;
    }
    ESP_LOGI(TAG,"\tAddress length: %d", remoteHost->h_length);

    i = 0;
    const char* address = nullptr;
    if (remoteHost->h_addrtype == AF_INET)
    {
        while (remoteHost->h_addr_list[i] != 0) {
            ip4_addr_t ip{};
            ip.addr = *(u_long *) remoteHost->h_addr_list[i++];
            const char* itaddress = inet_ntoa(ip);
            ESP_LOGI(TAG,"\tIP Address #%d: %s", i, itaddress);
            if (nullptr == address && nullptr != itaddress)
            {   // keep the first valid one
                address = itaddress;
            }
        }
    }

    return address;
}

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        {
            esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
            ESP_LOGI(ETH_TAG, "Ethernet Link Up");
            ESP_LOGI(ETH_TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                    mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        }
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(ETH_TAG, "Ethernet Link Down");
        // Disconnect the MQTT client
        if (mqtt_client)
        {
            ESP_LOGI(TAG, "MQTT client disconnected");
            esp_mqtt_client_disconnect(mqtt_client);
        }
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(ETH_TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(ETH_TAG, "Ethernet Stopped");

        // Disconnect the MQTT client
        if (mqtt_client)
        {
            ESP_LOGI(TAG, "MQTT client disconnected");
            esp_mqtt_client_disconnect(mqtt_client);
        }
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(ETH_TAG, "Ethernet Got IP Address");
    ESP_LOGI(ETH_TAG, "~~~~~~~~~~~");
    ESP_LOGI(ETH_TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(ETH_TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(ETH_TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(ETH_TAG, "~~~~~~~~~~~");

    // Show current DNS server
    eth_check_and_set_dns(nullptr);

    // Initialize mDNS
    start_mdns_service();

/*
    // Testing resolution
    resolve_mdns_host("DESKTOP-Barro");

    // .local is case sensitive
    resolveHostAndLog("DESKTOP-Barro.local");

    resolveHostAndLog("localhost");
    resolveHostAndLog("google.com");
*/

    // Start MQTT client loop
    mqtt_app_start();
}

static void ethernet_app_start(void)
{
    // Initialize Ethernet driver
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));

    // Initialize TCP/IP network interface aka the esp-netif (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create instance(s) of esp-netif for Ethernet(s)

    if (eth_port_cnt == 1) {
        // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
        // default esp-netif configuration parameters.
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        esp_netif_t *eth_netif = esp_netif_new(&cfg);
        // Attach Ethernet driver to TCP/IP stack
        ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[0])));
    } else {
        // Use ESP_NETIF_INHERENT_DEFAULT_ETH when multiple Ethernet interfaces are used and so you need to modify
        // esp-netif configuration parameters for each interface (name, priority, etc.).
        esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
        esp_netif_config_t cfg = {
            .base = &esp_netif_config,
            .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
        };
        char if_key_str[10];
        char if_desc_str[10];
        char num_str[3];
        for (int i = 0; i < eth_port_cnt; i++) {
            itoa(i, num_str, 10);
            strcat(strcpy(if_key_str, "ETH_"), num_str);
            strcat(strcpy(if_desc_str, "eth"), num_str);
            esp_netif_config.if_key = if_key_str;
            esp_netif_config.if_desc = if_desc_str;
            esp_netif_config.route_prio -= i*5;
            esp_netif_t *eth_netif = esp_netif_new(&cfg);

            // Attach Ethernet driver to TCP/IP stack
            ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[i])));
        }
    }

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    // Start Ethernet driver state machine
    for (int i = 0; i < eth_port_cnt; i++) {
        ESP_ERROR_CHECK(esp_eth_start(eth_handles[i]));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Start Ethernet driver state machine
    ethernet_app_start();

    // Loop publishing while connected
    int counter = 0;
    while(true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (mqtt_client && mqtt_connected)
        {
            // Generate a new payload message
            auto msg = random_gps_data();

            ESP_LOGI(TAG, "Show new message contents:");
            ESP_LOGI(TAG, "Device: %lld", msg.device);
            ESP_LOGI(TAG, "Latitude: %ld", msg.latitudex1e7);
            ESP_LOGI(TAG, "Longitude: %ld", msg.longitudex1e7);
            ESP_LOGI(TAG, "Altitude: %ld", msg.altitudemillimetres);
            ESP_LOGI(TAG, "Radius: %ld", msg.radiusmillimetres);
            ESP_LOGI(TAG, "Speed: %ld", msg.speedmillimetrespersecond);
            ESP_LOGI(TAG, "Satellites: %ld", msg.svs);
            std::time_t time = msg.timeutc;
            ESP_LOGI(TAG, "Time: %s", std::ctime(&time));

            auto data = serialize_gps_data(msg);
            ESP_LOGI(TAG, "publishing data: %d", counter);
            esp_mqtt_client_publish(mqtt_client, publisher_topic, (const char*)data.data(), data.size(), 1, 0);
        }
        else if(mqtt_client)
        {
            ESP_LOGI(TAG, "Not connected yet: %d", counter);
        }
        else
        {
            ESP_LOGI(TAG, "Not client set up yet: %d", counter);
        }

        ++counter;
    }
}
