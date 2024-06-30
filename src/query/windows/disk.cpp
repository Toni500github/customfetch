#include <string>
#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

Disk::Disk(const std::string_view path) {
    debug("Constructing");
}

std::string Disk::typefs() {
    return "NTFS";
}

float Disk::free_amount() {
    return 101.34f;
}

float Disk::used_amount() {
    return total_amount() - free_amount();
}

float Disk::total_amount() {
    return 480.01f;
}

#endif // CF_WINDOWS
