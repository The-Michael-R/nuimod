#include "nuimo_communication.h"


typedef struct nuimo_message_s nuimo_message_s;

struct nuimo_message_s{
  unsigned int     characteristic;
  unsigned int     direction;
  int              value;
  nuimo_message_s *next;
};


typedef struct {
  unsigned int     entries;

  nuimo_message_s *first;
  nuimo_message_s *last;  
} nuimo_buffer_s;


nuimo_buffer_s *nuimo_buffer;


void init_buffer() {
  DEBUG_PRINT(("init_buffer\n"));
  
  if (nuimo_buffer == NULL) {
    nuimo_buffer = malloc(sizeof(nuimo_buffer_s));

    nuimo_buffer->entries = 0;

    nuimo_buffer->first = NULL;
    nuimo_buffer->last  = NULL;
  }
}


gboolean pop_buffer(unsigned int *characteristic, unsigned int *direction, int *value) {
  DEBUG_PRINT(("pop_buffer\n"));
  
  nuimo_message_s *old;

  if (nuimo_buffer->entries == 0) {
    return FALSE;
  }

  old = nuimo_buffer->first;
  nuimo_buffer->first = old->next;

  *characteristic = old->characteristic;
  *direction      = old->direction;
  *value          = old->value;
  
  free(old);
  
  nuimo_buffer->entries--;

  if (nuimo_buffer->entries == 0) {
    nuimo_buffer->first = NULL;
    nuimo_buffer->last  = NULL; 
  }  
  
  return TRUE;
}


void push_buffer(unsigned int characteristic, unsigned int direction, int value) {
  DEBUG_PRINT(("push_buffer\n"));
  
  // Inteligent buffering for fast ocuring events
  if (nuimo_buffer->entries > 0 &&
      characteristic == NUIMO_ROTATION &&
      nuimo_buffer->last->characteristic == NUIMO_ROTATION) {
    nuimo_buffer->last->value += value;
    if (nuimo_buffer->last->value > 0) {
      nuimo_buffer->last->direction = NUIMO_ROTATION_LEFT;
    } else {
      nuimo_buffer->last->direction = NUIMO_ROTATION_RIGHT;
    }      
    return;
  }
  
  
  nuimo_message_s *new;

  new = malloc(sizeof(nuimo_message_s));

  if (nuimo_buffer->entries == 0) {
    nuimo_buffer->first = new;
    nuimo_buffer->last  = new; 
  } else {
    nuimo_buffer->last->next = new;
    nuimo_buffer->last       = new;
  }
  
  new->characteristic = characteristic;
  new->direction      = direction;
  new->value          = value;
  new->next           = NULL;

  nuimo_buffer->entries++;
}

/**
 * Receiving a message from Nuimo and add this to the buffer 
 * To install this function use ::nuimo_init_cb_function
 *
 * @param characteristic The characteristic based on ::nuimo_chars_e
 * @param value          Any decimal returnvalue from the Nuimo (in case of SWIPE/TOUCH events the value is 0)
 * @param dir            Indicated the direction of the received event. based on \ref NUIMO_DIRECTIONS
 * @param usr_data       This is supposed to be the pointer to the event_description structure
 * @see cb_change_val_notify
 * @see nuimo_init_cb_function
 */
static void cb_nuimo_communication(const uint characteristic, const int value, const unsigned int dir, const void *user_data) {
  DEBUG_PRINT(("cb_nuimo_communication\n"));

  // first check if event is covered in the event_description
  if (characteristic < NUIMO_BATTERY ||
      characteristic > NUIMO_ROTATION) {
    return;
  }
  
  push_buffer(characteristic, dir, value);
}


void establish_nuimo_comm() {
  DEBUG_PRINT(("esablish_nuimo_comm\n"));
 
  init_buffer();
  
  nuimo_init_status();
  nuimo_init_cb_function(cb_nuimo_communication, NULL);
  
  nuimo_init_bt();  // Not much will happen until the g_main_loop is started
}
