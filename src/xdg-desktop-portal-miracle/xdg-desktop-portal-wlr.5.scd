xdg-desktop-portal-wlr(5)

# NAME

xdg-desktop-portal-wlr - an xdg-desktop-portal backend for wlroots

# DESCRIPTION

xdg-desktop-portal-wlr (or xdpw for short) allows applications to request
screenshots and screencasts via xdg-desktop-portal in wlroots-based Wayland
compositors.

xdpw will try to load the configuration file from these locations:

- $XDG_CONFIG_HOME/xdg-desktop-portal-wlr/$XDG_CURRENT_DESKTOP
- $XDG_CONFIG_HOME/xdg-desktop-portal-wlr/config
- /etc/xdg/xdg-desktop-portal-wlr/$XDG_CURRENT_DESKTOP
- /etc/xdg/xdg-desktop-portal-wlr/config

_$XDG_CONFIG_HOME_ defaults to _~/.config_.
_$XDG_CURRENT_DESKTOP_ can be a colon separated list. Each element of that list will be tried.

The configuration files use the INI file format. Example:

```
[screencast]
output_name=HDMI-A-1
max_fps=30
exec_before=disable_notifications.sh
exec_after=enable_notifications.sh
chooser_type=simple
chooser_cmd=slurp -f %o -or
```

# SCREENCAST OPTIONS

These options need to be placed under the **[screencast]** section.

**output_name** = _name_
	Select which output will be screencast.

	This option is used with **chooser_type** = none. The list of available outputs
	can be obtained via **wayland-info**(1) (under the _wl_output_ section).

**max_fps** = _limit_
	Limit the number of frames per second to the provided rate.

	This is useful to reduce CPU usage when capturing frames at the output's
	refresh rate is unnecessary.

**exec_before** = _command_
	Execute _command_ before starting a screencast. The command will be executed within sh.

**exec_after** = _command_
	Execute _command_ after ending all screencasts. The command will be executed within sh.

**chooser_cmd** = _command_
	Run this command to select an output.

	For more details see **OUTPUT CHOOSER**.

**chooser_type** = _type_
	Specifies the input send to the chooser.

	The supported types are:
	- default: xdpw will try to use the first chooser found in the list of hardcoded choosers
	  (slurp, wofi, bemenu) and will fallback to an arbitrary output if none of those were found.
	- none: xdpw will allow screencast either on the output given by **output_name**, or if empty
	  an arbitrary output without further interaction.
	- simple, dmenu: xdpw will launch the chooser given by **chooser_cmd**. For more details
	  see **OUTPUT CHOOSER**.

**force_mod_linear** = _bool_
	Force buffers with implicit modifiers to be linear (experimental)

	Setting this option to 1 will force xdpw to allocate dma-bufs with implicit modifier as linear.
	This option shouldn't be required on single gpu setups, but can increase compatibility
	especially on setups with multiple gpus.

	This option is experimental and can be removed or replaced in future versions.

## OUTPUT CHOOSER

The chooser can be any program or script with the following behaviour:
- It returns any error code except 127. The error code 127 is internally used to signal
  that no command could be found and all output from it will be ignored.
- It returns the name of a valid output on stdout as given by **wayland-info**(1).
  Everything else will be handled as declined by the user.
- To signal that the user has declined screencast, the chooser should exit without
  anything on stdout.

Supported types of choosers via the **chooser_type** option:
- simple: the chooser is just called without anything further on stdin.
- dmenu: the chooser receives a newline separated list (dmenu style) of outputs on stdin.

# SEE ALSO

**pipewire**(1)
