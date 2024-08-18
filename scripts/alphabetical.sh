#!/bin/env bash

# https://github.com/KittyKatt/screenFetch/blob/master/screenfetch-dev#L96C1-L146C2
array=(
		'fluxbox'
		'openbox'
		'blackbox'
		'xfwm4'
		'metacity'
		'kwin'
		'twin'
		'icewm'
		'pekwm'
		'flwm'
		'flwm_topside'
		'fvwm'
		'dwm'
		"hyprland"
		'awesome'
		'tinywm'
		'wmaker'
		'qtile'
		'stumpwm'
		'musca'
		'xmonad'
		'i3'
		'i3wm'
		'ratpoison'
		'scrotwm'
		'spectrwm'
		'wmfs'
		'wmii'
		'weston'
		'wayfire'
		'beryl'
		'subtle'
		'e16'
		'enlightenment'
		'sawfish'
		'emerald'
		'monsterwm'
		'dminiwm'
		'compiz'
		'Finder'
		'herbstluftwm'
		'howm'
		'notion'
		'bspwm'
		'cinnamon'
		'2bwm'
		'echinus'
		'swm'
		'budgie-wm'
		'dtwm'
		'9wm'
		'chromeos-wm'
		'deepin-wm'
		'sway'
		'mwm'
)

sorted_array=($(for i in "${array[@]}"; do echo $i; done | sort))

echo "Sorted array:"
for i in "${sorted_array[@]}"; do
    printf "\"%s\", " $i
done
