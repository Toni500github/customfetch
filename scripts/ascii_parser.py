'''
    Parsing the distro ascii arts ripped from neofetch
    This is lazy and WILL make mistakes, the resulting file should be examined and corrected by someone.
    As I did for the json file included in the same commit.
'''

from json import dump

d = {}

with open('distro_ascii_arts.txt', 'r') as f:
    lines = f.readlines()
    name = ""

    i_offset = 0

    for i in range(len(lines)):
        if i + i_offset > len(lines):
            break

        if lines[i + i_offset] == '\n':
            continue

        line = lines[i + i_offset]
        first_quote = line.find('"')+1
        second_quote = line.find('"', first_quote)
        name = line[first_quote:second_quote]

        if name.startswith('[') and name.endswith(']'):
            name = name[1:-1]

        print(name)

        ascii_art = ""

        i_offset += 3
        for art_line in range(i + i_offset, len(lines)):
            i_offset += 1

            if lines[art_line].startswith("EOF"):
                break

            ascii_art += lines[art_line]

        d[name] = ascii_art

        i_offset += 1

dump(d, open("distro_ascii_arts.json", "w+"))
