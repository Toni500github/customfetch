#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

RAM::RAM() {
    debug("Constructing RAM");
}

size_t RAM::free_amount() {
    return 104442;
}

size_t RAM::total_amount() {
    return 15882;
}

size_t RAM::used_amount() {
    return total_amount() - free_amount();
}

#endif
