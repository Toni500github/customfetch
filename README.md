# Customfetch

A system information fetch tool (or [neofetch](https://github.com/dylanaraps/neofetch) like program), which its focus point is the customizability and perfomance.\
`customfetch` is designed to provide a really customizable way to display your system informations in the way you like or want.

Currently supports Linux distros only, but our current goal is to be cross-platform, which will happen soon (windows support incoming)

## Key Features

* **GUI support (for GTK3 (GTK4 soon))**
* Really customizable and fast, check [Config (with explanation)](#config-with-explanation) section
* Lightweight
>[!NOTE]
>enabling GUI support may slow down customfetch a bit because of linking the GUI libraries at runtime\
>consider using the non-GUI version if you only want to use customfetch on the terminal.\
>To check if it's enable or not, run "cufetch --version"

## Depends
### Debian/Ubuntu and based
```sh
$ sudo apt-get install libpci3 libgtkmm-3.0-1v5
```
### Arch and based
```sh
# the depends such as "pciutils" are already required by the "base" package
$ sudo pacman -S gtkmm3 gtk3
```

## Installation
### Arch and based (unstable) (AUR)
```bash
# with GUI support
# btw checkout our other project https://github.com/BurntRanch/TabAUR ;)
taur -S customfetch-git

# WITHOUT GUI support
taur -S customfetch-nogui-git
```

### Compile from source
```bash
# clone the git dir
git clone --depth=1 https://github.com/Toni500github/customfetch
cd customfetch

# DEBUG=0 for release build
# GUI_SUPPORT=1 for having GUI mode, or =0 for not
make install DEBUG=0 GUI_SUPPORT=1

# automatically generates a config
cufetch
```
## Config (with explanation)

Here's an example using the default config

![image](screenshot.png)

The config:

```toml
[config]
# includes directive, include the top name of each module you use.
# e.g. if you want to use $<os.name>, then `includes = ["os"]`.
# you can also put specific includes, for example if you only want os.name, then `includes = ["os.name"]`
includes = ["os", "cpu", "gpu", "ram"]

layout = [
    "${red}$<os.username>${0}@${cyan}$<os.hostname>",
    "───────────────────────────",
    "${red}OS${0}: $<os.name>",
    "${cyan}Uptime${0}: $<os.uptime_hours> hours, $<os.uptime_mins> minutes",
    "${green}Kernel${0}: $<os.kernel_name> $<os.kernel_version>",
    "${yellow}Arch${0}: $<os.arch>",
    "${magenta}CPU${0}: $<cpu.name> ($<cpu.nproc>) @ $<cpu.freq_max>GHz",
    "${blue}GPU${0}: $<gpu.name>",
    "${#03ff93}RAM usage${0}: $<ram.used> MB / $<ram.total> MB",
    "",
    "${\e[40m}   ${\e[41m}   ${\e[42m}   ${\e[43m}   ${\e[44m}   ${\e[45m}   ${\e[46m}   ${\e[47m}   ", # normal colors
    "${\e[100m}   ${\e[101m}   ${\e[102m}   ${\e[103m}   ${\e[104m}   ${\e[105m}   ${\e[106m}   ${\e[107m}   " # light colors
]

# display ascii-art or image/gif (GUI only) near layout
# put "os" for displaying the OS ascii-art
# or the "/path/to/file" for displaying custom files
# or "off" for disabling ascii-art or image displaying
source-path = "os"

# Path to where we'll take all the distros/OSs ascii arts
# it MUST contain an "ascii" subdirectory
data-dir = "/usr/share/customfetch"

# offset between the ascii art and the system infos
offset = 5

# Colors can be with: hexcodes (#55ff88) and for bold put '!' (!#55ff88)
# OR ANSI escape code colors like "\e[1;34m"
# remember to add ${0} where you want to reset color
black = "\e[1;90m"
red = "\e[1;91m"
green = "\e[1;92m"
yellow = "\e[1;93m"
blue = "\e[1;94m"
magenta = "\e[1;95m"
cyan = "\e[1;96m"
white = "\e[1;97m"

# GUI options
# note: customfetch needs to be compiled with GUI_SUPPORT=1 (check with "cufetch --version")
[gui]
enable = false

# Font to be used
# syntax must be [FAMILY-LIST] [STYLE-OPTIONS] [SIZE]
# e.g "Liberation Mono Normal 12"
# check https://lazka.github.io/pgi-docs/Pango-1.0/classes/FontDescription.html#Pango.FontDescription for more infos
font = "Liberation Mono Normal 12"

# These are the colors palette you can use in the GUI mode.
# They can overwritte with ANSI escape code colors
# but they don't work with those, only hexcodes
black = "!#000005"
red = "!#ff2000"
green = "!#00ff00"
blue = "!#00aaff"
cyan = "!#00ffff"
yellow = "!#ffff00"
magenta = "!#f881ff"
white = "!#ffffff"
```

You may be confused and have difficulty to understand, but this is why customfetch is different from the others.\
We use our own parser for displaying the system informations or anything else, and so we use the variable `layout` along side the OS ascii art text file.

We use something we call "modules" and they starts with a '$'. **We use them on both the ascii art text file and the `layout` variable**\
There are 3 modules:

* **The info module** ($<>) lets you access a sub-member of a built-in component\
  e.g `$<user.name>` will print the username, `$<os.kernel_version>` will print the kernel version and so on.\
  run "cufetch -l" for a list of builti-in components

* **The color module** (${}) displays the text with a color\
  e.g "${red}hello world" will indeed print "hello world" in red (or the color you set in the variable).\
  you can even put a custom hex color e.g: ${#ff6622} (for bold put '!' at start) OR bash escape code colors e.g ${\e[1;32m} or ${\e[0;34m}.\
  To reset color use ${0}

* **The bash command module** ($()) let's you execute bash commands.\
  e.g $(echo \"hello world\") will indeed echo out hello world\
  you can even use pipes:\
  e.g $(echo \"hello world\" | cut -d' ' -f2) will only print world

Any end brackets (')', '}', '>') can be escaped with \\
