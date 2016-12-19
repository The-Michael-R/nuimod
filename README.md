# nuimod
Deamon to use the Nuimo to execute linux commands/scripts.

Currently the basic functionality is given. With the current configuration file it controls a local running mpd (Music Player Daemon) and reacts to the return values.


## Warning
It is currently under some development and not cleaned up (e.g. the Makefile has a problem, just cp nuimo.o inc/nuimo.o and make).
Otherwise it works nicely. But some things might change if I find time.


## Requires
As the nuimo-SDK depends on BlueZ 5.40 or later the usable linux distributions are a bit limited. ArchLinux is working fine.


## Config file
The nuimod.config file is read after start, no make run required.
```
#NUIMO_SIGNALS
  
NUIMO_BUTTON:
{
  BUTTON_PRESS:
  {
  command  = "mpc toggle";
  reaction = ( { regex   = "^\[playing\]";
                 pattern = "041870e0c1870707060400"; },
               { regex   = "^\[paused\]";
	         pattern = "eedcb973e7ce9d3b77ee00"; },
               { regex   = "^\[stop\])";
		 pattern = "FEFCF9F3E7CF9F3F7FFE00"; } );
  };
};

NUIMO_BATTERY:
...
```
Each characteristic is one of: NUIMO_BATTERY, NUIMO_BUTTON, NUIMO_FLY, NUIMO_SWIPE, NUIMO_ROTATION or NUIMO_ENTRIES_LEN
Within the caracteristic the direction is specified: e.g. FLY_LEFT, FLY_RIGHT, ... see nuimod.h for full list.
In each direction a command can be specified: command = "&lt;command or script&gt;"
Plus one or more reactions. This is using regular expressions applied on the stdout of the command (see above). To each of the regular expressins a 22 char long hex pattern (without 0x) is required. This pattern will be send to the LED-Matrix if the reguar expressin matches.

The doc/LED-Matrix.ods (Libre Office Calc) is a very crude paint programm wich calculates the pattern for you.

Complictated? Just check the provided nuimod.conf as guideline.

Don't blame me if it changes on a daily basis. But rest assured I'm lazy don't expect that to happen often.

## Current limitations
Even as it is called Daemon it is not yet that far. It is still under development but systemd might be OK with the current state.

## ToDo
That would be quiet a long list:

1. Make it a real daemon
2. Allow parameters for rotation/fly events forwarded to the command (e.g. "mpc volume +%d")
3. Multiple event-levels. Changing with fly-left/right the set of commands used (e.g. switch between controlling the light and controlling the music). This might change the current config-file format
4. Use standard-paths for fonfig files (e.g. /etc/nuimod/ and ~/.nuimod/)
5. Predefined patterns (alpanumeric). For example to temperatures.
6. Better documentation, especially of the config file.
7. ...
