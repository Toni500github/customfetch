#include <fstream>

#include "query.hpp"
#include "util.hpp"

using namespace Query;

/*enum {
    SHMEM = 0,
    FREE,
    BUFFER,
    CACHED,
    SRECLAIMABLE
};*/

static size_t get_from_text(std::string& line, u_short& iter_index)
{
    std::string amount = line.substr(line.find(':') + 1);
    strip(amount);
    ++iter_index;
    return std::stoi(amount);
}

static RAM::RAM_t get_amount()
{
    debug("calling in RAM {}", __PRETTY_FUNCTION__);
    constexpr std::string_view meminfo_path = "/proc/meminfo";
    RAM::RAM_t                 memory_infos;

    // std::array<size_t, 5> extra_mem_info;
    std::ifstream file(meminfo_path.data());
    if (!file.is_open())
    {
        error("Could not open {}\nFailed to get RAM infos", meminfo_path);
        return memory_infos;
    }

    std::string    line;
    static u_short iter_index = 0;
    while (std::getline(file, line) && iter_index < 4)
    {
        if (hasStart(line, "MemAvailable:"))
            memory_infos.free_amount = get_from_text(line, iter_index);

        if (hasStart(line, "MemTotal:"))
            memory_infos.total_amount = get_from_text(line, iter_index);

        if (hasStart(line, "SwapFree:"))
            memory_infos.swap_free_amount = get_from_text(line, iter_index);

        if (hasStart(line, "SwapTotal:"))
            memory_infos.swap_total_amount = get_from_text(line, iter_index);

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
    memory_infos.used_amount =
        memory_infos.total_amount -
        memory_infos.free_amount;  // + extra_mem_info.at(SHMEM) - extra_mem_info.at(FREE) - extra_mem_info.at(BUFFER) -
                                   // extra_mem_info.at(CACHED) - extra_mem_info.at(SRECLAIMABLE);

    return memory_infos;
}

RAM::RAM()
{
    debug("Constructing {}", __func__);
    if (!m_bInit)
    {
        m_memory_infos = get_amount();
        m_bInit        = true;
    }
}

// clang-format off
float& RAM::free_amount()
{ return m_memory_infos.free_amount; }

float& RAM::total_amount()
{ return m_memory_infos.total_amount; }

float& RAM::used_amount()
{ return m_memory_infos.used_amount; }

float& RAM::swap_total_amount()
{ return m_memory_infos.swap_total_amount; }

float& RAM::swap_free_amount()
{ return m_memory_infos.swap_free_amount; }
