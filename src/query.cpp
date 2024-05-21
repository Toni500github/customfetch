/* Implementation of the system behind querying information about the system. */
#include <query.hpp>

using namespace QuerySystem;

string QuerySystem::QuerySystemName() {
    return "Arch Linux"; // example
}

string QuerySystem::QueryGPUName() {
    return "NVIDIA GeForce GTX 1650 Super" ; // example
}