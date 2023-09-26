#include <mosquittopp.h>
#include <absl/log/log.h>

class mqtt_client :
    public mosqpp::mosquittopp
{
    protected:

//    void on_connect(int /*rc*/) override;
//    void on_connect_with_flags(int /*rc*/, int /*flags*/) override;
//    void on_disconnect(int /*rc*/) override;
//    void on_publish(int /*mid*/) override;
//    void on_message(const struct mosquitto_message * /*message*/) override;
//    void on_subscribe(int /*mid*/, int /*qos_count*/, const int * /*granted_qos*/) override;
//    void on_unsubscribe(int /*mid*/) override;
//    void on_log(int /*level*/, const char * /*str*/) override;
//    void on_error() override;
};

int main(int argc, char* argv[])
{
    mqtt_client client;

    //TODO: set up cli
    const char* host = "DESKTOP-BARRO";
    const int port = 6338;

    // Set up connection
    client.connect(host, port, true);



    return 0;
}
