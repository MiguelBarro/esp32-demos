#include <inttypes.h>
#include <cstdlib>
#include <ctime>

#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <gps.pb-c.h>

static const char* TAG = "protobuf-example";

extern "C" void app_main(void)
{
      Gps__Coords msg = GPS__COORDS__INIT; // message static initialization
      uint8_t* buf;                     // Buffer to store serialized data
      size_t len;                  // Length of serialized data

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

      ESP_LOGI(TAG, "Show new message contents:");
      ESP_LOGI(TAG, "Device: %lld", msg.device);
      ESP_LOGI(TAG, "Latitute: %ld", msg.latitudex1e7);
      ESP_LOGI(TAG, "Longitud: %ld", msg.longitudex1e7);
      ESP_LOGI(TAG, "Altitute: %ld", msg.altitudemillimetres);
      ESP_LOGI(TAG, "Radius: %ld", msg.radiusmillimetres);
      ESP_LOGI(TAG, "Speed: %ld", msg.speedmillimetrespersecond);
      ESP_LOGI(TAG, "Satellites: %ld", msg.svs);
      time = msg.timeutc;
      ESP_LOGI(TAG, "Time: %s", std::ctime(&time));

      // Get packet size
      len = gps__coords__get_packed_size(&msg);

      // allocate buffer
      buf = static_cast<uint8_t*>(std::malloc(len));

      // Serialize to buffer
      gps__coords__pack(&msg, buf);

      // Show the length of message
      ESP_LOGI(TAG, "Writing %d serialized bytes", len);

      // Show the buffer byte by byte
      ESP_LOG_BUFFER_HEXDUMP(TAG, buf, len, ESP_LOG_INFO);

      // Unpack the buffer to an internally allocated message
      Gps__Coords* pmsg = gps__coords__unpack(nullptr, len, buf);

      // Show the recovered contents
      ESP_LOGI(TAG, "Show new message contents:");
      ESP_LOGI(TAG, "Device: %lld", pmsg->device);
      ESP_LOGI(TAG, "Latitute: %ld", pmsg->latitudex1e7);
      ESP_LOGI(TAG, "Longitud: %ld", pmsg->longitudex1e7);
      ESP_LOGI(TAG, "Altitute: %ld", pmsg->altitudemillimetres);
      ESP_LOGI(TAG, "Radius: %ld", pmsg->radiusmillimetres);
      ESP_LOGI(TAG, "Speed: %ld", pmsg->speedmillimetrespersecond);
      ESP_LOGI(TAG, "Satellites: %ld", pmsg->svs);
      time = pmsg->timeutc;
      ESP_LOGI(TAG, "Time: %s", std::ctime(&time));

      // free the recovered message
      gps__coords__free_unpacked (pmsg, nullptr);

      std::free(buf); // Free the allocated serialized buffer

      for (int i = 10; i >= 0; i--)
      {
          ESP_LOGI(TAG,"Restarting in %d seconds...\n", i);
          vTaskDelay(1000 / portTICK_PERIOD_MS);
      }

      ESP_LOGI(TAG,"Restarting now.\n");
      esp_restart();
}
