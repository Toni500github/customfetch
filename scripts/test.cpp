#include <algorithm>
#include <iostream>
#include "pci.ids.hpp"

int main() {
    size_t index = pci_vendors_location_array[std::distance(pci_vendors_array.begin(), std::find(pci_vendors_array.begin(), pci_vendors_array.end(), "10de"))];

    std::cout << index << std::endl;

    return 0;
}