[![GitHub top language](https://img.shields.io/github/languages/top/Toni500github/customfetch?logo=cplusplusbuilder&label=)](https://github.com/Toni500github/customfetch/blob/main/src)
[![GitHub Workflow Status (with event)](https://img.shields.io/github/actions/workflow/status/Toni500github/customfetch/makefile.yml)](https://github.com/Toni500github/customfetch/actions)\
[![forthebadge](https://forthebadge.com/images/badges/works-on-my-machine.svg)](https://forthebadge.com)

# Customfetch

A system information fetch tool (or [neofetch](https://github.com/dylanaraps/neofetch) like program), which its focus point is the customizability and perfomance.\
`customfetch` is designed to provide a really customizable way to display your system informations in the way you like or want.

Currently supports Linux distros only. Android may be coming when stable release
<!-- Comment this because it's still in WIP for 3 weeks, no shit it won't work on some OSs
>[!NOTE]
>The goal is to be cross-platform, so maybe Android and MacOS support will come some day\
>but if you're using a UNIX OS, such as FreeBSD or MINIX, or those "obscure" OSs\
>then some, if not most, query infos won't probably work.\
>So you may want to relay to shell commands for quering\
>or maybe continue using neofetch/fastfetch if it still works great for you
-->

<img align=left width=52% height=50% src="assets/screenshots/nitch_catpan-style.png" />
<img src="https://upload.wikimedia.org/wikipedia/commons/2/24/Transparent_Square_Tiles_Texture.png" width="49%" height="16px" align="left" />
<img align=left width=52% height=50% src="assets/screenshots/modern-simple.png"/>
<p align="center">
    <img align="top" width=44% height=20% src="assets/screenshots/cbonsai.png" />
</p>
<img src="assets/screenshots/pipeline-style.png" />

## Key Features

* **GUI mode (GTK3)**
* Really customizable and fast, check [Config (with explanation)](#config-with-explanation) section
* Lightweight

## Depends
currently requires **C++20**, but it's possible to compile with C++17 too (not officially supported)

* `libdl` (should come already installed in almost every distro)

If you want to install with GUI mode install from your package manager:
* `gtk3`
* `gtkmm3`

## Installation

>[!NOTE]
>installing with GUI mode may slow down customfetch a bit because it needs to load the GUI libraries at runtime.\
>To check if it's enabled or not, run "cufetch --version"

### Debian/Ubuntu and based
Download the latest `.deb` package in [releases](https://github.com/Toni500github/customfetch/releases/latest)

### Arch and based (AUR)
```bash
# btw checkout our other project https://github.com/BurntRanch/TabAUR ;)
# either
taur -S customfetch-bin

# or wiht GUI mode
taur -S customfetch-gui-bin
```

### General Distros (Manual installation)
Download the latest `.tar.gz` tarball file in [releases](https://github.com/Toni500github/customfetch/releases/latest) \
It contains the binary `cufetch` and the manual `cufetch.1` with the `LICENSE`.\
Togheter with the directory `assets/ascii` with the distro ascii art logos.\
If installing the GUI mode version, there's `cufetch.desktop`

### Arch and based (AUR) (source)
```bash
# either
taur -S customfetch

# or wiht GUI mode
taur -S customfetch-gui
```

### Arch and based (unstable) (AUR) (source)
```bash
# either
taur -S customfetch-git

# or wiht GUI mode
taur -S customfetch-gui-git
```

### Compile from (source) (unstable)
```bash
# clone the git dir
git clone https://github.com/Toni500github/customfetch
cd customfetch

# DEBUG=0 for release build
# GUI_MODE=0 for disabling GUI mode, or =1 for enable it (will slow down a bit if run in terminal)
make install DEBUG=0 GUI_MODE=0

# automatically generates a config and prints the infos
cufetch
```

## Config (with explanation)

Read the manual `cufetch.1` or the comments in the default generated config for knowing more about the configuration in customfetch.\
This is only a brief explaination and preview.

Here's an example using my config

![image](screenshot.png)

```toml
[config]

# The array for displaying the system infos
layout = [
    "$<builtin.title>",
    "$<builtin.title_sep>",
    "${auto}OS: $<os.name> $<system.arch>",
    "${auto}Host: $<system.host>",
    "${auto}Kernel: $<os.kernel>",
    "${auto}Uptime: $<os.uptime>",
    "${auto}Terminal: $<user.terminal>",
    "${auto}Shell: $<user.shell>",
    "${auto}Packages: $<os.pkgs>",
    "${auto}Theme: $<theme-gtk-all.name>",
    "${auto}Icons: $<theme-gtk-all.icons>",
    "${auto}Font: $<theme-gtk-all.font>",
    "${auto}Cursor: $<theme.cursor>",
    "${auto}WM: $<user.wm_name>",
    "${auto}DE: $<user.de_name>",
    "${auto}Disk(/): $<disk(/).disk>",
    "${auto}CPU: $<cpu.cpu>",
    "${auto}GPU: $<gpu.name>",
    "${auto}RAM: $<ram.ram>",
    "",
    "$<builtin.colors>", # normal colors palette
    "$<builtin.colors_light>" # light colors palette
]


```

In the config we got an array variable called "layout". That's the variable where you customize how the infos should be displayed.\
There are 5 tags:
* `$<module.member>` - Used for printing the value of a member of a module.
* `${color}` - Used for displaying text in a specific color.
* `$(bash command)` - Used to execute bash commands and print the output.
* `$[something,equalToSomethingElse,iftrue,ifalse]` - Conditional tag to display different outputs based on the comparison.
* `$%n1,n2%` - Used to print the percentage and print with colors

They can be used in the ascii art text file and layout, but how to use them?

* **The info tag (`$<>`)** will print a value of a member of a module\
 e.g `$<user.name>` will print the username, `$<os.kernel_version>` will print the kernel version and so on.\
 All the modules and their members are listed in the `--list-modules` argument

* **The bash command tag (`$()`)** let's you execute bash commands and print the output\
 e.g `$(echo \"hello world\")` will indeed echo out Hello world.\
 you can even use pipes\
 e.g `$(echo \"hello world\" | cut -d' ' -f2)` will only print world

* **The conditional tag (`$[]`)** is used for displaying different outputs based on the comparison.\
  Syntax MUST be `$[something,equalToSomethingElse,iftrue,ifalse]` (**note**: putting spaces between commas can change the expected result).\
  Each part can have a tag or anything else.\
  e.g `$[$<user.name>,$(echo $USER),the name is correct,the name is NOT correct]`\
  This is useful when on some terminal or WM the detection can be different than others,\
  Or maybe even on holidays for printing special texts\

* **The color tag (`${}`)** is used for printing the text in a certain color.\
 e.g `${red}hello world` will indeed print "hello world" in red (or the color you set in the variable/tag).\
 The colors can be: <ins>black</ins>, <ins>red</ins>, <ins>green</ins>, <ins>blue</ins>, <ins>cyan</ins>, <ins>yellow</ins>, <ins>magenta</ins>, <ins>white</ins> and they can be configured in the config file.\
 **ANSI escape colors** can be used, e.g `\e[1;31m` or `\e[38;2;160;223;11m`.\
 Alternatively, You can put a custom **hex color** e.g: `#ff6622`.\
 You can also use them inside the tag, like `${!#343345}` or `${\e[1;31m}`.\
 It's possible to enable multiple options, put these symbols before `#`:\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**Terminal and GUI**\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`b` - for making the color in the background\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`u` - to  underline the text\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`!` - for making the text bold\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`i` - for making the text italic\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`s` - for strikethrough text\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**GUI Only**\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`o` - for overline\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`a(value)` - for fg alpha (either a percentage value like `50%` or a plain integer between 1 and 65536)\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`L(value)` - for choosing an underline style (`none`, `single`, `double`, `low`, `error`)\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`U(value)` - for choosing the underline color (hexcode without #)\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`B(value)` - for choosing the bg color text (hexcode without #)\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`S(value)` - for choosing the strikethrough color (hexcode without #)\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`O(value)` - for choosing the overline color (hexcode without #)\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`A(value)` - for choosing the bg text alpha (either a percentage value like `50%` or a plain integer between 1 and 65536)\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`w(value)` - for choosing the font weight (`ultralight`, `light`, `normal`, `bold`, `ultrabold`, `heavy`, or a numeric weight)\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**Terminal Only**\
 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`l` - for blinking text\
 \
 To reset colors, use `${0}` for a normal reset or `${1}` for a bold reset.\
 For auto coloring, depending on the ascii logo colors, use `${auto}`.\
 They can be used for different colors too. So for getting the 2nd color of the ascii logo,\
 use `${auto2}`, for the 4th one use `${auto4}` and so on.\
 If you're in GUI mode and the source path is an image, all the auto colors will be the same colors as distro ascii art.

* **The Percentage tag (`$%%`)** is used for displaying the percentage between 2 numbers.\
  It **Must** contain a comma for separating the 2. They can be either be taken from a tag or it put yourself.\
  For example: $%10,5%
  For inverting colors of bad and great (red and green), before the first `%` a put `!`

Any `$` or brackets can be escaped with a backslash `\`. You need to escape backslashes too :(\
**NOTE:** For having compatibility with GUI mode, you need to escape `<` (EXCEPT if you are using in a info tag, like `$<os.name>`) and `&`\
e.g `the number 50 is \< than 100 \& 98`
Won't affect the printing in terminal

# TODOs
* ~~Color all ASCII arts (157/262) will take long ahh time~~ DONE
* Release 0.10.0

# Thanks
I would like to thanks:
* my best-friend [BurntRanch](https://github.com/BurntRanch/),\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;For helping me initialize this project and motivate me to keep going\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;And also for making my customizability idea come true with the parser.

* the Better C++ [discord server](https://discord.gg/uSzTjkXtAM), \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;For helping me improving the codebase and helping me with any issues I got,\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;And also for being patient with me XD

* [fastfetch](https://github.com/fastfetch-cli/fastfetch/) and [neofetch](https://github.com/dylanaraps/neofetch),\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;For inspiring this project

* [{fmt}](https://github.com/fmtlib/fmt) and [toml++](https://github.com/marzer/tomlplusplus) libraries\
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Our favorite libraries that me and BurntRanch uses

* this string switch-case [library](https://github.com/xroche/stringswitch), \
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Really amazing, thanks for making this

I hope you'll like customfetch, and also checkout [TabAUR](https://github.com/BurntRanch/TabAUR/tree/dev), our other project that was made before customfetch.\
Don't forgot [sdl_engine](https://github.com/BurntRanch/sdl_engine) too ;)

![meme.png](assets/screenshots/meme.png)
