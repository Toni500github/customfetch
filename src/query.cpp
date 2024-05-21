/* Implementation of the system behind querying information about the system. */
#include <query.hpp>
#include <unistd.h>

QuerySystem::QuerySystem() {
    uname(&query_sys.osInfo);
}

string QuerySystem::SystemName() {
    return this->osInfo.domainname; // example
}

string QuerySystem::GPUName() {
    return "NVIDIA GeForce GTX 1650 Super" ; // example
}
