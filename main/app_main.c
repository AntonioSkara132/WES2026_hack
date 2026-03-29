


//--------------------------------- INCLUDES ----------------------------------
#include "freertos/idf_additions.h"
#include "gui.h"
#include "Audio_play.h"
//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static QueueHandle_t send_queue, recv_queue;
static const uint8_t msg_queue_len = 20;
static const uint8_t msg_max_len = 64;
//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------


void app_main(void)
{
	send_queue = xQueueCreate(msg_queue_len, sizeof(char) * msg_max_len);
	recv_queue = xQueueCreate(msg_queue_len, sizeof(char) * msg_max_len);
	gui_init(&send_queue, &recv_queue);
	play();
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

//---------------------------- INTERRUPT HANDLERS -----------------------------

