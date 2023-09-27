#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <thread>

#include <absl/log/log.h>
#include <absl/log/initialize.h>
#include <absl/log/log_sink.h>
#include <absl/log/globals.h>
#include <absl/log/log_sink_registry.h>

#include <mosquittopp.h>

#include <gps.pb.h>

// TODO: set up user Ctrl-C
bool user_exit = false;

// pub/sub according with esp32 client not this one
static const char* subscriber_topic = "esp32/gps/subscribe";
static const char* publisher_topic = "esp32/gps/publish";

// Generate random gps data
gps::Coords random_gps_data()
{
    gps::Coords msg;

    // randomly populate the message
    std::time_t time = std::time(nullptr);
    std::srand(time);
    msg.set_device(std::rand());
    msg.set_latitudex1e7(std::rand());
    msg.set_longitudex1e7(std::rand());
    msg.set_altitudemillimetres(std::rand());
    msg.set_radiusmillimetres(std::rand() % 10000);
    msg.set_speedmillimetrespersecond(std::rand() % 100);
    msg.set_svs(std::rand() % 5);
    msg.set_timeutc(time);

    return msg;
}

class mqtt_client :
    public mosqpp::mosquittopp
{
    mqtt_client()
    {
        if (MOSQ_ERR_UNKNOWN == mosqpp::lib_init())
            LOG(ERROR) << "Cannot initialize MQTT";
    }

    public:

    ~mqtt_client()
    {
		unsubscribe(nullptr, publisher_topic);
        mosqpp::lib_cleanup();
    }

    static mqtt_client& get_client()
    {
        static mqtt_client client;
        return client;
    }

    protected:

    void on_connect(int rc) override
    {
        // https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/errata01/os/mqtt-v3.1.1-errata01-os-complete.html#_Toc385349256
        switch(rc)
        {
            case 0:
                LOG(INFO) << "Connection accepted";

                // launch the subscription
                if (MOSQ_ERR_SUCCESS != subscribe(nullptr, publisher_topic))
                {
                    LOG(ERROR) << "Cannot set up the subscription";
                    // exit the loop
                    disconnect();
                }
            break;
            defautl:
                LOG(ERROR) << "Connection rejected with code: " << rc;
        }
    }

    void on_subscribe(int mid, int /*qos_count*/, const int * /*granted_qos*/) override
    {
        LOG(INFO) << "Subscription accepted";
    }

    void on_message(const mosquitto_message* message) override
    {
        if(std::strcmp(message->topic, publisher_topic))
        {
            LOG(ERROR) << "Unexpected topic message " << message->topic;
        }
        else
        {
            gps::Coords msg;
            msg.ParseFromArray(message->payload, message->payloadlen);

            LOG(INFO) << std::endl
                      << "Show received message contents: " << std::endl
                      << "\tDevice: " << msg.device() << std::endl
                      << "\tLatitude: " << msg.latitudex1e7() << std::endl
                      << "\tLongitude: " << msg.longitudex1e7() << std::endl
                      << "\tAltitude: " << msg.altitudemillimetres() << std::endl
                      << "\tRadius: " << msg.radiusmillimetres() << std::endl
                      << "\tSpeed: " << msg.speedmillimetrespersecond() << std::endl
                      << "\tSatellites: " << msg.svs() << std::endl
                      << "\tTime: " << msg.timeutc();
        }
    }
};

// Create an abseil sink to send info and warnings to STDOUT
class StdOutLogSink final : public absl::LogSink {
 public:
  ~StdOutLogSink() override = default;

  void Send(const absl::LogEntry& entry) override {
    if (entry.log_severity() >= absl::StderrThreshold())
      return;

    if (!entry.stacktrace().empty()) {
      std::cout << entry.stacktrace().data() << std::endl;
    } else {
      std::cout << entry.text_message_with_prefix_and_newline();
    }
  }
} log_sink;

int main(int argc, char* argv[])
{
    absl::InitializeLog();
    // send infos and warnings to stdout
    absl::AddLogSink(&log_sink);

    //TODO: set up cli
    const char* host = "DESKTOP-BARRO";
    const int port = 6338;

    // Set up connection
    mqtt_client& client = mqtt_client::get_client();

    switch (client.connect(host, port))
    {
        case MOSQ_ERR_SUCCESS:
            break;
        case MOSQ_ERR_INVAL:
            LOG(ERROR) << "The Network Connection has been made but MQTT service is unavailable";
            return MOSQ_ERR_INVAL;
        case MOSQ_ERR_ERRNO:
            LOG(ERROR) << "OS error code: " << errno;
            break;
        default:
            LOG(ERROR) << "Unknown return code for connection";
    }

    // next publish time
    auto np = std::chrono::steady_clock::now();

    while(!user_exit)
    {
        // keep looping and publishing each second
        int res = client.loop(0);
        auto n = std::chrono::steady_clock::now();

        if (MOSQ_ERR_SUCCESS == res && n > np)
        {
            np = n + std::chrono::seconds(1);

            auto msg = random_gps_data();

            LOG(INFO) << std::endl
                      << "Show new message contents: " << std::endl
                      << "\tDevice: " << msg.device() << std::endl
                      << "\tLatitude: " << msg.latitudex1e7() << std::endl
                      << "\tLongitude: " << msg.longitudex1e7() << std::endl
                      << "\tAltitude: " << msg.altitudemillimetres() << std::endl
                      << "\tRadius: " << msg.radiusmillimetres() << std::endl
                      << "\tSpeed: " << msg.speedmillimetrespersecond() << std::endl
                      << "\tSatellites: " << msg.svs() << std::endl
                      << "\tTime: " << msg.timeutc();

            auto str = msg.SerializeAsString();
            if (MOSQ_ERR_SUCCESS != client.publish(nullptr, subscriber_topic, str.length(), str.c_str()))
                LOG(ERROR) << "Failed to publish";
        }
    }

    return 0;
}
