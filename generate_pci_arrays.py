pciDeviceNames = {}

with open('pci.ids', 'r') as f:
    lines = f.read().split('\n')

    vendorID = None

    for line in lines:
        if line[0] == '#' or not line:
            continue

        indent_level = line.count('\t')

        match indent_level:
            case 0:
                vendorID = line[:4]
                if vendorID == 'ffff':  # Illegal Vendor ID
                    break
                pciDeviceNames[vendorID] = {}
            case 1:
                deviceID = line[1:5]
                deviceName = line[7:]

                if '[' in deviceName and ']' in deviceName:
                    deviceName = deviceName[deviceName.find('[')+1:deviceName.find(']')]

                pciDeviceNames[vendorID][deviceID] = deviceName

def dict_to_cpp_map(d: dict, nested_call : bool = False) -> str:
    output = "{"

    for i, kv in enumerate(d.items()):
        k, v = kv

        output += '{"' + k.replace('"', "'") + '"'

        output += ', '

        if isinstance(v, dict):
            output += dict_to_cpp_map(v, True) + '},\n'
        else:
            output += '"' + v.replace('"', "'") + '"}'

            if i != len(d.keys()) - 1:
                output += ', '
    
    output += '}'

    return output

def dict_to_google_sparsemap(d: dict, nested_call : bool = False) -> str:
    output = ""

    for i, kv in enumerate(d.items()):
        k, v = kv

        output += '\tmap["' + k.replace('"', "'") + '"] = '

        if isinstance(v, dict):
            output += dict_to_cpp_map(v, True) + ';\n'
        else:
            output += '"' + v.replace('"', "'") + '";\n'

    return output

with open('pci.ids.hpp', 'w+') as f:
    f.write("""#ifndef PCI_IDS_HPP
#define PCI_IDS_HPP
#include <string>
#include <sparsehash/sparse_hash_map>
#include <unordered_map>

typedef google::sparse_hash_map<std::string, std::unordered_map<std::string, std::string>> pci_unordered_map;

inline pci_unordered_map getPCIDevicesMap() {
    pci_unordered_map map;

%s
    return map;
}

inline pci_unordered_map pciDevicesMap = getPCIDevicesMap();
#endif""" % dict_to_google_sparsemap(pciDeviceNames))

print(pciDeviceNames["10de"]["2204"])
