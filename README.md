# Customfetch

A command-line system information tool (or [neofetch](https://github.com/dylanaraps/neofetch) like program) which its focus point is the customizability and perfomance.\
`customfetch` is designed to provide a really really customizable way to display your system informations in the way you like or want.

Currently supports Linux distros only, but our current goal is to be cross-platform, which will happen soon (windows support incoming)

## Config (with explaination)
Here's an example by using the default config

![image](https://github.com/Toni500github/customfetch/assets/88878648/8b1f6ca7-28a7-4b8a-b302-18dbfcd6af87)

The config:
```toml
[config]
layout = [
    "${red}$<user.name>${0}@${cyan}$<os.hostname>",
    "───────────────────────────",
    "${red}OS${0}: $<os.name>",
    "${cyan}Uptime${0}: $<os.uptime_hours> hours, $<os.uptime_mins> minutes",
    "${green}Kernel${0}: $<os.kernel_name> $<os.kernel_version>",
    "${yellow}Arch${0}: $<os.arch>",
    "${magenta}CPU${0}: $<cpu.name>",
    "${blue}GPU${0}: $<gpu.name>",
    "${#03ff93}RAM usage${0}: $<ram.used> MB / $<ram.total> MB",
    "",
    "${\e[40m}   ${\e[41m}   ${\e[42m}   ${\e[43m}   ${\e[44m}   ${\e[45m}   ${\e[46m}   ${\e[47m}   ",
    "${\e[100m}   ${\e[101m}   ${\e[102m}   ${\e[103m}   ${\e[104m}   ${\e[105m}   ${\e[106m}   ${\e[107m}   "
]

# Colors can be with: hexcodes (#55ff88) OR bash escape code colors like "\e[1;34m"
# remember to add ${0} where you want to reset color
red = "#ff2000"
green = "#00ff00"
blue = "#00aaff"
cyan = "#00ffff"
yellow = "#ffff00"
magenta = "#ff11cc"
```

The ascii art `file.txt`:
```
${\e[1;31m}                     ./${\e[1;35m}o${\e[1;34m}.
${\e[1;31m}                   ./${\e[1;35m}sssso${\e[1;34m}-
${\e[1;31m}                 `:${\e[1;35m}osssssss+${\e[1;34m}-
${\e[1;31m}               `:+${\e[1;35m}sssssssssso${\e[1;34m}/.
${\e[1;31m}             `-/o${\e[1;35m}ssssssssssssso${\e[1;34m}/.
${\e[1;31m}           `-/+${\e[1;35m}sssssssssssssssso${\e[1;34m}+:`
${\e[1;31m}         `-:/+${\e[1;35m}sssssssssssssssssso${\e[1;34m}+/.
${\e[1;31m}       `.://o${\e[1;35m}sssssssssssssssssssso${\e[1;34m}++-
${\e[1;31m}      .://+${\e[1;35m}ssssssssssssssssssssssso${\e[1;34m}++:
${\e[1;31m}    .:///o${\e[1;35m}ssssssssssssssssssssssssso${\e[1;34m}++:
${\e[1;31m}  `:////${\e[1;35m}ssssssssssssssssssssssssssso${\e[1;34m}+++.
${\e[1;31m}`-////+${\e[1;35m}ssssssssssssssssssssssssssso${\e[1;34m}++++-
${\e[1;31m} `..-+${\e[1;35m}oosssssssssssssssssssssssso${\e[1;34m}+++++/`
${\e[1;34m}   ./++++++++++++++++++++++++++++++/:.
${\e[1;34m} `:::::::::::::::::::::::::------``
```
You may be confused and have difficulty to understand, but this is why customfetch is different from the others.\
We use our own parser for displaying the system informations or anything else, and so we use the variable `layout` along side the file.txt, which contains the ascii art.

We use something we call "modules" and they starts with a '$', **we use them on both the ascii art text file and the `layout` variable**\
There are 3 modules:

* <strong>The info module</strong> ($<>) let's you access a sub-member of a member\
  e.g `$<user.name>` will print the username, `$<os.kernel_version>` will print the kernel version and so on.\
  list of builti-in components coming soon

* **The color module** (${}) displays the text with a color\
  e.g "${red}hello world" will indeed print "hello world" in red (or the color you set in the variable).\
  you can even put a custom hex color e.g: ${#ff6622} OR bash escape code colors e.g ${\e[1;32m} or ${\e[0;34m}.\
  To reset color use ${0}

* **The bash command module** ($()) let's you execute bash commands.\
  e.g $(echo \"hello world\") will indeed echo out hello world\
  you can even use pipes:\
  e.g $(echo \"hello world\" | cut -d' ' -f2) will only print world

Hope that explained well
