#include "query.hpp"
#include "util.hpp"

#include <array>
#include <fstream>

using namespace Query;

enum {
    USED = 0,
    AVAILABLE,
    TOTAL,

    SHMEM = 0,
    FREE,
    BUFFER,
    CACHED,
    SRECLAIMABLE
};

// minimaze the while loop iteration once we have all the values we needed
// less cpu cicles and saving ms of time

static size_t get_from_text(std::string& line, u_short& iter_index) {
    std::string amount = line.substr(line.find(':')+1);
    strip(amount);
    ++iter_index;
    return std::stoi(amount);
}

static std::array<size_t, 3> get_amount() {
    constexpr std::string_view meminfo_path = "/proc/meminfo";
    std::array<size_t, 3> memory_infos;
    //std::array<size_t, 5> extra_mem_info;
    std::ifstream file(meminfo_path.data());
    if (!file.is_open()) {
        error("Could not open {}", meminfo_path);
        return {0};
    }
    
    std::string line;
    u_short iter_index = 0;
    while (std::getline(file, line) && iter_index < 2) {
        if (line.find("MemAvailable:") != std::string::npos)
            memory_infos.at(AVAILABLE) = get_from_text(line, iter_index);

        if (line.find("MemTotal:") != std::string::npos)
            memory_infos.at(TOTAL) = get_from_text(line, iter_index);

        /*if (line.find("Shmem:") != std::string::npos)
            extra_mem_info.at(SHMEM) = get_from_text(line);
        
        if (line.find("MemFree:") != std::string::npos)
            extra_mem_info.at(FREE) = get_from_text(line);
        
        if (line.find("Buffers:") != std::string::npos)
            extra_mem_info.at(BUFFER) = get_from_text(line);
        
        if (line.find("Cached:") != std::string::npos)
            extra_mem_info.at(CACHED) = get_from_text(line); 
        
        if (line.find("SReclaimable:") != std::string::npos)
            extra_mem_info.at(SRECLAIMABLE)  = get_from_text(line);*/
    }
    
    // https://github.com/dylanaraps/neofetch/wiki/Frequently-Asked-Questions#linux-is-neofetchs-memory-output-correct
    memory_infos.at(USED) = memory_infos.at(TOTAL) - memory_infos.at(AVAILABLE); // + extra_mem_info.at(SHMEM) - extra_mem_info.at(FREE) - extra_mem_info.at(BUFFER) - extra_mem_info.at(CACHED) - extra_mem_info.at(SRECLAIMABLE);    

    return memory_infos;
}

RAM::RAM() {
    m_memory_infos = get_amount();
}

size_t RAM::free_amount()  { 
    return m_memory_infos.at(AVAILABLE) / 1024; 
}

size_t RAM::used_amount()  { 
    return m_memory_infos.at(USED) / 1024; 
}

size_t RAM::total_amount() { 
    return m_memory_infos.at(TOTAL) / 1024; 
}
