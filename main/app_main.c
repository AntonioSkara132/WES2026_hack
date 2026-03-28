


//--------------------------------- INCLUDES ----------------------------------
#include "gui.h"
<<<<<<< HEAD
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_event.h"
=======
#include "Audio_play.h"
>>>>>>> 421efaa1d8044674cf6c60346a2590f4f05c27f4
//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

//------------------------- STATIC DATA & CONSTANTS ---------------------------

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

#define TASK_STACK_DEPTH 2048
extern void tcp_client(void);
void tcp_task(void *pVparameters){
   tcp_client();
}


void app_main(void)
{
   gui_init();
<<<<<<< HEAD
   ESP_ERROR_CHECK(nvs_flash_init());
   ESP_ERROR_CHECK(esp_netif_init());
   ESP_ERROR_CHECK(esp_event_loop_create_default());

   /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
   * Read "Establishing Wi-Fi or Ethernet Connection" section in
   * examples/protocols/README.md for more information about this function.
   */
   ESP_ERROR_CHECK(example_connect());

   xTaskCreate(tcp_task, "tcp_client", TASK_STACK_DEPTH, NULL, 0, NULL);
   tcp_client();

=======
   audio_play();
>>>>>>> 421efaa1d8044674cf6c60346a2590f4f05c27f4
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

//---------------------------- INTERRUPT HANDLERS -----------------------------

