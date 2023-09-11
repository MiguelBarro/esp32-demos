#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <gps.pb-c.h>

void app_main(void)
{
//    AMessage msg = AMESSAGE__INIT; // AMessage
//    void *buf;                     // Buffer to store serialized data
//    unsigned len;                  // Length of serialized data
//
//    if (argc != 2 && argc != 3)
//    {   // Allow one or two integers
//    fprintf(stderr,"usage: amessage a [b]\n");
//    return 1;
//    }
//
//    msg.a = atoi(argv[1]);
//    if (argc == 3) { msg.has_b = 1; msg.b = atoi(argv[2]); }
//    len = amessage__get_packed_size(&msg);
//
//    buf = malloc(len);
//    amessage__pack(&msg,buf);
//
//    fprintf(stderr,"Writing %d serialized bytes\n",len); // See the length of message
//    fwrite(buf,len,1,stdout); // Write to stdout to allow direct command line piping
//
//    free(buf); // Free the allocated serialized buffer

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
