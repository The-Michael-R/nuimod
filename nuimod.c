#include "nuimod.h"


static void print_ptr(event_description_s *event_description) {
  DEBUG_PRINT(("print_ptr\n"));

  unsigned int i;
  unsigned int j;
  
  printf("event_description = %p\n", event_description);
  if (event_description) {
    for (i = 0; i < NUIMO_ENTRIES_LEN; i++) {
      printf("  ->characteristic[%s].command  = %p\n", N_NUIMO[i], event_description->characteristic[i].command);
      printf("       (limit = %i; threshold = %i)\n",
	     event_description->characteristic[i].limit,
	     event_description->characteristic[i].threshold);
      
      for (j = 0; j < event_description->characteristic[i].num_commands; j++) {
	printf("    -->command[%i].command  = %p (%s)\n",
	       j,
	       event_description->characteristic[i].command[j].command,
	       event_description->characteristic[i].command[j].command);
      }
    }
  }
}


static int exec_command(const char *command, char *buffer, const unsigned int length) {
  DEBUG_PRINT(("exec_command\n"));

  FILE *file_buffer;
  char line[128];
  unsigned int position; 

  file_buffer = popen(command, "r");
  if (!file_buffer) {
    fprintf(stderr, "EE: Problem while executing command!\n");
    return EXIT_FAILURE;
  }

  position = 0;
  while (fgets(line, sizeof(line), file_buffer) != NULL) {
    if (position + strlen(line) < length) {
      strcpy(&buffer[position], line);
      position += strlen(line);
    }
  }
  pclose(file_buffer);

  return EXIT_SUCCESS;
}


static int str_to_pattern(const char *string, unsigned char *pattern) {
  DEBUG_PRINT(("str_to_pattern\n"));

  unsigned int i;
  char         buffer[3];
  
  buffer[0] = '\0';
  buffer[1] = '\0';
  buffer[2] = '\0';

  
  if (strlen(string) != 22) {
    fprintf(stderr, "EE: String has wrong length!\n");
    return EXIT_FAILURE;
  }
  
  i = 0;
  while(string[i*2]) {
    buffer[0] = string[i*2];
    buffer[1] = string[i*2 + 1];
    pattern[i] = strtol(buffer, NULL, 16);
    i++;
  }
  
  return EXIT_SUCCESS;
}


static int get_reactions(config_setting_t *config_setting, command_s *command) {
  DEBUG_PRINT(("get_reactions\n"));

  config_setting_t *local_setting;
  config_setting_t *node;
  unsigned int      i;
  unsigned int      ret_val;
  const char*       temp;
  char error_msg[BUFFER_LEN];
  
  local_setting = config_setting_lookup(config_setting, "reaction");
  if (!local_setting) {
    return EXIT_SUCCESS;
  }

  command->num_actions = config_setting_length(local_setting);
  command->action_config = malloc(sizeof(action_config_s) * command->num_actions);

  i = 0;
  while(i < command->num_actions) {
    node = config_setting_get_elem(local_setting, i);
    
    ret_val = config_setting_lookup_string(node, "regex", &temp);
    if (ret_val == CONFIG_FALSE) {
      return EXIT_FAILURE;
    }
    
    ret_val = regcomp(&command->action_config[i].regex_preg,
		      temp,
		      REG_NEWLINE);
    
    if (ret_val != 0) {
      	regerror (ret_val , &command->action_config[i].regex_preg, error_msg, BUFFER_LEN);
        fprintf(stderr, "EE: Regex error compiling '%s': %s\n", temp, error_msg);
        return 1;
	return EXIT_FAILURE;
    }
    
    ret_val = config_setting_lookup_string(node, "pattern", &temp);
    if (ret_val == CONFIG_FALSE) {
      return EXIT_FAILURE;
    }

    ret_val = str_to_pattern(temp, command->action_config[i].pattern);
    if (ret_val == EXIT_FAILURE) {
      return EXIT_FAILURE;
    }

    i++;  
  }
  
  return EXIT_SUCCESS;
}


static int get_commands(config_setting_t *config_setting, command_s command[], const char *command_list[]) {
  DEBUG_PRINT(("get_commands\n"));

  unsigned int      i;
  int               ret_val;
  const char       *temp;
  config_setting_t *local_setting;

  i = 0;
  while(command_list[i]) {
    printf("command_list = %s\n", command_list[i]);
    local_setting = config_setting_lookup(config_setting, command_list[i]);
    if(!local_setting) {
      i++;
      continue;
    }
    
    ret_val = config_setting_lookup_string(local_setting, "command", &temp);
    if (ret_val == CONFIG_TRUE) {
      command[i].command = strdup(temp);

      ret_val = get_reactions(local_setting, &command[i]);
      if (ret_val == EXIT_FAILURE) {
	fprintf(stderr, "EE: Problems with reactions at %s\n", command_list[i]);
	return EXIT_FAILURE;
      }
    }
    i++;
  }

  return EXIT_SUCCESS;
}


static void get_config(const config_t my_configuration, event_s *event, const unsigned int characterisic) {
  DEBUG_PRINT(("get_config\n"));
  
  config_setting_t  *config_setting;
  int                temp;
  int                ret_val;
  
  config_setting = config_lookup(&my_configuration, N_NUIMO[characterisic]);
  if (!config_setting) {
    return;
  }

  if (characterisic == NUIMO_BATTERY) {
    ret_val = config_setting_lookup_int(config_setting, "limit", &temp);
    
    if (ret_val == CONFIG_FALSE ) {
      fprintf(stderr, "EE: NUIMO_BATTERY configuration without limit\n");
      return;
      
    } else if (temp > 100 || temp < 0) {
      fprintf(stderr, "EE: NUIMO_BATTERY.limit must be between 0 and 100\n");
      return;
    }
    event->limit = (unsigned int) temp;
    
  } else if (characterisic == NUIMO_ROTATION) {
    ret_val = config_setting_lookup_int(config_setting, "threshold", &temp);
    
    if (ret_val == CONFIG_FALSE ) {
      fprintf(stderr, "EE: NUIMO_ROTATION configuration without limit\n");
      return;
    }
    event->threshold = (unsigned int) temp;
  }

  if (characterisic == NUIMO_BATTERY) {
    ret_val = get_commands(config_setting, event->command, N_BATTERY);
  } else if (characterisic == NUIMO_BUTTON) {
    ret_val = get_commands(config_setting, event->command, N_BUTTON);
  } else if (characterisic == NUIMO_FLY) {
    ret_val = get_commands(config_setting, event->command, N_FLY);
  } else if (characterisic == NUIMO_SWIPE) {
    ret_val = get_commands(config_setting, event->command, N_SWIPE);
  } else if (characterisic == NUIMO_ROTATION) {
    ret_val = get_commands(config_setting, event->command, N_ROTATION);
  } else {
    ret_val = EXIT_SUCCESS;
  }
  
  if (ret_val == EXIT_FAILURE) {
    fprintf(stderr, "EE: %s problems reading command/reaction.\n", N_NUIMO[characterisic]);
    return;
  }
  
  return;
}


static int read_config(const char *path, const char *filename, event_description_s *event_description) {
  DEBUG_PRINT(("read_config\n"));
  
  config_t my_configuration;
  int      ret_val;
  unsigned int i;
  
  config_init(&my_configuration);

  ret_val = config_read_file(&my_configuration, filename);
 
  if (ret_val == CONFIG_FALSE) {
    fprintf(stderr,
	    "EE: Error in configuration file:\n  %s in %s:%i\n",
	    config_error_text(&my_configuration),
	    config_error_file (&my_configuration),
	    config_error_line(&my_configuration));
    config_destroy(&my_configuration);
    return EXIT_FAILURE;
  }

  for (i = 0; i < NUIMO_ENTRIES_LEN; i++) {
    if (i != BT_ADAPTER && i != NUIMO && i != NUIMO_LED) {
      get_config(my_configuration, &event_description->characteristic[i], i);
    }
  }

  config_destroy(&my_configuration);
  return EXIT_SUCCESS;
}


static int init_description (event_description_s *event_description) {
  DEBUG_PRINT(("init_description\n"));

  unsigned int type_cnt;
  unsigned int element_cnt;
  unsigned int i;

  for (type_cnt = 0; type_cnt < NUIMO_ENTRIES_LEN; type_cnt++) {
    
    if (type_cnt == NUIMO_BATTERY) {
      element_cnt = 1;
    } else if (type_cnt == NUIMO_BUTTON) {
      element_cnt = NUIMO_BUTTON_LEN;
    } else if (type_cnt == NUIMO_FLY) {
      element_cnt = NUIMO_FLY_LEN;
    } else if (type_cnt == NUIMO_SWIPE) {
      element_cnt = NUIMO_SWIPE_LEN;
    } else if (type_cnt == NUIMO_ROTATION) {
      element_cnt = NUIMO_ROTATION_LEN;
    } else {
      element_cnt = 0;
    }

    event_description->characteristic[type_cnt].limit        = 0;
    event_description->characteristic[type_cnt].threshold    = 0;
    event_description->characteristic[type_cnt].num_commands = element_cnt;

    if (element_cnt > 0) {
      event_description->characteristic[type_cnt].command = malloc(sizeof(command_s) * element_cnt);
      
      for (i = 0; i < element_cnt; i++) {
	event_description->characteristic[type_cnt].command[i].command       = NULL;
	event_description->characteristic[type_cnt].command[i].num_actions   = 0;
	event_description->characteristic[type_cnt].command[i].action_config = NULL;
      }
    } else {
      event_description->characteristic[type_cnt].command      = NULL;
    }
  }

  return EXIT_SUCCESS;
}


/**
 * Stops the g_main_loop; call this function to stop the programm
 *
 * @param data Is the g_main_loop handle returnes by ::g_main_loop_new
 */
static gboolean cb_termination(gpointer data) {
  DEBUG_PRINT(("cb_termination\n"));
  
  g_main_loop_quit(data);

  return FALSE;
}


/**
 * Main example callback function used to execute user actions in case the Nuimo characteristics
 * send a change-value notification. To install this function use ::nuimo_init_cb_function
 *
 * @param characteristic The characteristic based on ::nuimo_chars_e
 * @param value          Any decimal returnvalue from the Nuimo (in case of SWIPE/TOUCH events the value is 0)
 * @param dir            Indicated the direction of the received event. based on \ref NUIMO_DIRECTIONS
 * @param usr_data       THis is supposed to be the pointer to the event_description structure
 * @see cb_change_val_notify
 * @see nuimo_init_cb_function
 */
static void my_cb_function(const uint characteristic, const int value, const unsigned int dir, const void *user_data) {
  DEBUG_PRINT(("my_cb_function\n"));
  
  event_description_s *event_description;
  char buffer[BUFFER_LEN];
  unsigned int i;
  regmatch_t matches[1];
  
  event_description = (event_description_s*) user_data;
    
  // first check if event is covered in the event_description
  if (event_description == NULL) {
    return;
  }

  memset(buffer, '\0', BUFFER_LEN);
  
  switch (characteristic) {
  case NUIMO_BATTERY:
    if (value < event_description->characteristic[characteristic].limit) {
      if (event_description->characteristic[characteristic].command[dir].command != NULL) {
	exec_command(event_description->characteristic[characteristic].command[dir].command, buffer, BUFFER_LEN);
      }
    }
    break;

  case NUIMO_ROTATION:
    if (abs(value) > event_description->characteristic[characteristic].threshold ) {
      if (event_description->characteristic[characteristic].command[dir].command != NULL) {
	exec_command(event_description->characteristic[characteristic].command[dir].command, buffer, BUFFER_LEN);
      }
    }
    break;
    
  case NUIMO_BUTTON:
  case NUIMO_SWIPE:
  case NUIMO_FLY:
    if (event_description->characteristic[characteristic].command[dir].command != NULL) {
      exec_command(event_description->characteristic[characteristic].command[dir].command, buffer, BUFFER_LEN);
    }
    break; 
  }

  if (buffer[0]) {
    i = 0;

    while(i < event_description->characteristic[characteristic].command[dir].num_actions) {
      regexec(&event_description->characteristic[characteristic].command[dir].action_config[i].regex_preg,
	      buffer,
	      1,
	      matches,
	      0);

      // Why does the rm.so contain values outside the buffer length as match (and not -1)?
      if(matches[0].rm_so >= 0 && matches[0].rm_so < BUFFER_LEN) {
	nuimo_set_led(event_description->characteristic[characteristic].command[dir].action_config[i].pattern, 0x80, 50);
	return;
      }
      i++;
    }
  }
}


static void start_nuimo(event_description_s *event_description) {
  DEBUG_PRINT(("start_nuimo\n"));
 
  GMainLoop *loop;
 
  nuimo_init_status();
  nuimo_init_cb_function(my_cb_function, event_description);
  
  nuimo_init_bt();  // Not much will happen until the g_main_loop is started

  loop = g_main_loop_new(NULL, FALSE);
  g_unix_signal_add (SIGINT,  cb_termination, loop); // Calling a private FKT!
  g_main_loop_run(loop);

  nuimo_disconnect();

  return;
}


int main (int argc, char **argv) {
  DEBUG_PRINT(("main\n"));

  event_description_s *event_description;

  event_description = malloc(sizeof(event_description_s));
  if (!event_description) {return EXIT_FAILURE;}

  memset(event_description, '\0', sizeof(event_description_s));

  init_description(event_description);

  read_config(".", "nuimod.config", event_description);

  start_nuimo(event_description);
  
  return EXIT_SUCCESS;
}
