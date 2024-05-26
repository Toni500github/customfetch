from copy import copy

vendor_array = []
location_array = []

with open("pci.ids", "r") as f:
    lines = f.readlines()

    current_vendor = None

    location = 0

    for line_num in range(len(lines)):
        line = lines[line_num]
        location += len(line)

        if line.startswith('#') or line == '\n':
            continue

        # This is the start of "device classes", We don't care anymore.
        if line.startswith('C'):
            break

        indent_level = 0
        for c in line:
            if c != '\t':
                break
            indent_level += 1
        
        if not (2 >= indent_level >= 0):
            raise ValueError("File includes a line with more than a 2 or less than a 0 indent level. (line: %s)" % line)
        
        line = line.lstrip('\t')

        match indent_level:
            case 0:
                current_vendor = line.split(' ')[0]
                vendor_array.append(current_vendor)
                location_array.append(location - len(line))
            case 1:
                # if current_vendor is None:
                #     raise ValueError("File includes an indented line before any vendor declaration.")
                
                # device_id = line.split(' ')[0]
                # device_name = ' '.join(line.split(' ')[1:]).removeprefix(' ').removesuffix('\n')

                # # first character is ' '

                # pci_table[current_vendor][device_id] = device_name
                continue
            case 2:
                # subvendor = line.split(' ')[0]
                # subdevice_id = line.split(' ')[1]
                # subdevice_name = line.split(' ')[2]

                # if subvendor not in pci_table:
                #     pci_table[subvendor] = {}

                # pci_table[subvendor][subdevice_id] = subdevice_name
                continue

# def dict_to_cpp_map(d : dict, iter : int = 0):
#     return_string = "{"

#     for k in d.keys():
#         # v = copy(d[k])
#         v = d[k]
#         # isDict = False

#         # try:
#         #     v = dict_to_cpp_map(v, iter + 1)
#         #     isDict = True
#         # except Exception:
#         #     pass
        
#         #return_string += "{" + ('"' + k.replace('"', "'") + '"') + "," + (('"' + v.replace('"', "'") + '"') if not isDict else v) + "},"
#         return_string += "{" + ('"' + k.replace('"', "'") + '"') + "," + repr(v) + "},"

#         if iter == 0:
#             return_string += '\n'
    
#     return_string += "}"

#     return return_string

with open("pci.ids.hpp", 'w+') as f:
    f.write("""#ifndef PCI_IDS_HPP
#define PCI_IDS_HPP
#include <string>
#include <array>

std::array<std::string, %s> pci_vendors_array = %s;
std::array<int, %s> pci_vendors_location_array = %s;


#endif  // PCI_IDS_HPP""" % (len(vendor_array), repr(vendor_array).replace("'", '"').replace('[', '{').replace(']', '}'), len(location_array), repr(location_array).replace("'", '"').replace('[', '{').replace(']', '}')))

#print(pci_table['10de']['1f0a'])