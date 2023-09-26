#include <cstring>

#include <mosquittopp.h>
#include <absl/log/log.h>

// TODO: set up user Ctrl-C
bool user_exit = false;

// pub/sub according with esp32 client not this one
static const char* subscriber_topic = "esp32/gps/subscribe";
static const char* publisher_topic = "esp32/gps/publish";

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

    void on_message(const mosquitto_message* msg) override
    {
        if(std::strcmp(msg->topic, publisher_topic))
        {
            LOG(ERROR) << "Unexpected topic message " << msg->topic;
        }
        else
        {
            // TODO: deserialize magic
        }
    }
};

int main(int argc, char* argv[])
{
    //TODO: set up cli
    const char* host = "DESKTOP-BARRO";
    const int port = 6338;

    // Set up connection
    mqtt_client& client = mqtt_client::get_client();

    if (MOSQ_ERR_SUCCESS != client.connect(host, port, true))
    {
        LOG(ERROR) << "Cannot establish connection";
        return -1;
    }

    while(!user_exit)
    {
        // keep looping and publishing each second
        int res = client.loop_forever(1000);

        if (MOSQ_ERR_SUCCESS == res)
        {
            // TODO: generate the message magic
            // client.publish(nullptr, subscriber_topic, len, payload);
        }
    }

    return 0;
}
