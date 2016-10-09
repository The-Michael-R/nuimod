#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <libconfig.h>
#include <glib-unix.h>

#include "inc/nuimo.h"


/**
 * Debug-printouts in case the compiler has the option -DDEBUG or 'make debug' is used.
 * It mainly prints out the function name. Should not used for productiion executable
 */
#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif


#define BUFFER_LEN 4096


static const char *N_NUIMO[] = {
  "BT_ADAPTER",
  "NUIMO",
  "NUIMO_BATTERY",
  "NUIMO_LED",
  "NUIMO_BUTTON", 
  "NUIMO_FLY",
  "NUIMO_SWIPE",  
  "NUIMO_ROTATION",
  "NUIMO_ENTRIES_LEN"
};


static const char *N_BATTERY[] = {
  "BATTERY_LEVEL",
  NULL 
};

  
static const char *N_ROTATION[] = {
  "ROTATION_LEFT",
  "ROTATION_RIGHT",
  NULL
};

  
static const char *N_FLY[] = {
  "FLY_LEFT",
  "FLY_RIGHT"
  "FLY_UP",
  "FLY_DOWN",
  NULL
};


static const char *N_BUTTON[] = {
  "BUTTON_RELEASE",  
  "BUTTON_PRESS",
  NULL
};

  
static const char *N_SWIPE[] = {
  "SWIPE_LEFT",
  "SWIPE_RIGHT",  
  "SWIPE_UP",     
  "SWIPE_DOWN",   
  "TOUCH_LEFT",   
  "TOUCH_RIGHT",  
  "TOUCH_TOP",    
  "TOUCH_BOTTOM",
  NULL
};


typedef struct {
  regex_t regex_preg;
  unsigned char    pattern[11];
} action_config_s;


typedef struct {
  char            *command;
  unsigned int     num_actions;
  action_config_s *action_config;
} command_s;


/**
 *
 */
typedef struct {
  unsigned int  limit;
  unsigned int  threshold;
  unsigned int  num_commands;
  command_s    *command;
} event_s;

/**
 * Wasting a bit of memory here, but the performance might be better and the code is much more compact. paghetti-code anyone? 
 */
typedef struct {
  event_s characteristic[NUIMO_ENTRIES_LEN];
} event_description_s;
  


