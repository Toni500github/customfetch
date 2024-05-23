#include "query.hpp"

using namespace Query;

System::System() {
    uname(&this->sysInfos);
}
