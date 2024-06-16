from json import load

d : dict = load(open('distro_ascii_arts.json', 'r'))

for k, v in d.items():
    with open('assets/ascii/' + k.replace('/', '_') + '.txt', 'w+') as f:
        f.write(v)
