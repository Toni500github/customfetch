#!/bin/bash

array=("awesome" "bspwm" "dtwm" "dwm" "herbstluftwm" "hyprland" "i3" "i3wm" "icewm" "openbox" "qtile" "sway" "tinywm" "wayfire" "weston" "xmonad" "xfwm4")

sorted_array=($(for i in "${array[@]}"; do echo $i; done | sort))

echo "Sorted array:"
for i in "${sorted_array[@]}"; do
    printf "\"%s\", " $i
done
