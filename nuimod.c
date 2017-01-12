#include "nuimod.h"

/**
 * Debug function; it just prints out the imported config file structure.
 *
 * @param event_description The structure that holds the configuration.
 */
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

/**
 * This function executes the given command and receives the STDOUT
 *
 * @param command  The command which will be executed
 * @param buffer   Buffer which receives the STDOUT message of the executed command
 * @param length   length of the ::buffer. If the received answer is longer the content gets truncated.
 * @return Returns EXIT_FAILURE if any problems executing the command occoured or EXIT_SUCCESS
 */
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

/**
 * This function is used to convert the hex-denoted string into the required bitmap.
 *
 * @param string   An 22 char long hex string holding the LED-Matrix pattern.
 * @param pattern  The 11 char long converted pattern.
 * @return Returns EXIT_FAILURE if the provided string has the wrong length or EXIT_SUCCESS
 */
static int str_to_pattern(const char *string, unsigned char *pattern) {
  DEBUG_PRINT(("str_to_pattern\n"));

  unsigned int  i;
  unsigned char buffer;

  if (strlen(string) != 22) {
    fprintf(stderr, "EE: LED-Pattern string has wrong length (22)%s!\n", string);
    return EXIT_FAILURE;
  }

  i = 0;
  buffer = string[0] | 0b00100000; // Make the char lower case

  while (string[i]) {
    if (buffer >= '0' && buffer <= '9') {
      buffer = buffer - '0';
    } else if (buffer >= 'a' && buffer <= 'f') {
      buffer = buffer - 'a' + 10;
    } else {
      fprintf(stderr, "EE: LED-Pattern string holds unknown chars (0...9, A...F, a...f): %s!\n", string);
      return EXIT_FAILURE;
    }

    if ((i & 1) == 1) {
      pattern[(i-1)/2] += buffer;
    } else {
      pattern[i/2] = buffer * 16;
    }

    i++;
    buffer = string[i] | 0b00100000;
  }

  return EXIT_SUCCESS;
}

/**
 * Evaluate the config file reactions to the given command.
 * It read the reactions, if present, and pre-compile the regular expressions.
 *
 * @param  config_setting The sub-structure of the configuration for the curren command which will be evaluated
 * @param  command        The pointer to the structure we store the pre-compiled regex into.
 * @return Returns EXIT_FAILURE or EXIT_SUCCESS
 */
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

    ret_val = regcomp(&command->action_config[i].regex_preg, temp, REG_NEWLINE);

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

/**
 * Evaluating the configuration file looking for the commands that should be executed in case the Nuimo emits a event
 *
 * @param config_setting This is the preloaded config file which will be evvaluated
 * @param command        The structure of possible Nuimo actions we need to cover; no need to use them all
 * @param command_list   The pointer to the structure that will be used during operation. Holding the commands, actions, ...
 * @return Returns EXIT_FAILURE or EXIT_SUCCESS
 */
static int get_commands(config_setting_t *config_setting, command_s command[], const char *command_list[]) {
  DEBUG_PRINT(("get_commands\n"));

  unsigned int      i;
  int               ret_val;
  const char       *temp;
  config_setting_t *local_setting;

  i = 0;
  while(command_list[i]) {
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


/**
 * Evaluating the config file. Needs to be called for each possible characteristic of the Nuimo that can issue a event
 *
 * @param my_configuration The structure of the config file which will be evaluated.
 * @param event            The pointer to the event which needs to be filled by this function
 * @param charateristic    The current characteristic
 */
static int get_config(const config_t my_configuration, event_s *event, const unsigned int characterisic) {
  DEBUG_PRINT(("get_config\n"));

  config_setting_t  *config_setting;
  int                temp;
  int                ret_val;

  config_setting = config_lookup(&my_configuration, N_NUIMO[characterisic]);
  if (!config_setting) {
    return EXIT_FAILURE;
  }

  if (characterisic == NUIMO_BATTERY) {
    ret_val = config_setting_lookup_int(config_setting, "limit", &temp);

    if (ret_val == CONFIG_FALSE ) {
      fprintf(stderr, "EE: NUIMO_BATTERY configuration without limit\n");
      return EXIT_FAILURE;

    } else if (temp > 100 || temp < 0) {
      fprintf(stderr, "EE: NUIMO_BATTERY.limit must be between 0 and 100\n");
      return EXIT_FAILURE;
    }
    event->limit = (unsigned int) temp;

  } else if (characterisic == NUIMO_ROTATION) {
    ret_val = config_setting_lookup_int(config_setting, "threshold", &temp);

    if (ret_val == CONFIG_FALSE ) {
      fprintf(stderr, "EE: NUIMO_ROTATION configuration without limit\n");
      return EXIT_FAILURE;
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

  return ret_val;
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
      if (get_config(my_configuration, &event_description->characteristic[i], i) == EXIT_FAILURE) {
        return EXIT_FAILURE;
      }
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
  nuimo_disconnect();

  return FALSE;
}


/**
 * This function is called approx. every 100ms (not catching up delayed events). It empties the
 * whole buffer which may be accumulated since the last call.
 *
 * @param usr_data       This is supposed to be the pointer to the event_description structure
 * @see cb_change_val_notify
 */
static gboolean cb_main_timing(void *user_data) {
  //  DEBUG_PRINT(("cb_main_timing\n"));

  event_description_s *event_description;
  char buffer[BUFFER_LEN];
  unsigned int i;
  regmatch_t matches[1];
  unsigned int characteristic;
  unsigned int dir;
  int value;
  int regex_res;

  event_description = (event_description_s*) user_data;

  // Execute all messages which came in since last time
  while (pop_buffer(&characteristic, & dir, &value)) {

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
    }

    if (buffer[0]) {
      i = 0;

      while(i < event_description->characteristic[characteristic].command[dir].num_actions) {
        regex_res = regexec(&event_description->characteristic[characteristic].command[dir].action_config[i].regex_preg,
                            buffer,
                            1,
                            matches,
                            0);

        if(!regex_res) {
          nuimo_set_led(event_description->characteristic[characteristic].command[dir].action_config[i].pattern, 0x80, 50, 1);
          return TRUE;
        }
        i++;
      }
    }
  }
  return TRUE;
}


int main (int argc, char **argv) {
  DEBUG_PRINT(("main\n"));

  GMainLoop *loop;
  event_description_s *event_description;

  loop = g_main_loop_new(NULL, FALSE);

  event_description = malloc(sizeof(event_description_s));
  if (!event_description) {
    return EXIT_FAILURE;
  }
  memset(event_description, '\0', sizeof(event_description_s));

  init_description(event_description);

  if (read_config(".", "nuimod.config", event_description) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  if (establish_nuimo_comm() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  };

  // Adding additional signals to the GMainLoop
  g_unix_signal_add (SIGINT,  cb_termination, loop);
  g_timeout_add(100, cb_main_timing, event_description);

  // Start GMainLoop
  g_main_loop_run(loop);

  return EXIT_SUCCESS;
}
