#include <fstream>
#include <query.hpp>
#include <algorithm>
#include <unistd.h>

using namespace Query;

string_view System::name() {
    return this->sysInfos.sysname;
}

string System::OS_Name() {
    string_view sysName = this->name(); // Query::System::name()

    if (sysName == "Linux") {
        string os_pretty_name;
        path os_release_path = "/etc/os-release";
        std::ifstream os_release_file(os_release_path);
        if (!os_release_file.is_open())
            die("Could not open {}", os_release_path.c_str());

        string line;
        while (std::getline(os_release_file, line)) {
            if(line.find("PRETTY_NAME=") == 0) {
                os_pretty_name = line.substr(12);
                os_pretty_name.erase(std::remove(os_pretty_name.begin(), os_pretty_name.end(), '\"' ), os_pretty_name.end());
                os_release_file.close();
                return os_pretty_name;
            }
        }

    }

    return "System not supported yet";

}

string_view System::GPUName() {
    return "NVIDIA GeForce GTX 1650 Super" ; // example
}
