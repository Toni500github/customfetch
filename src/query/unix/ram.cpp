/*
 * Copyright 2024 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

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

static size_t get_from_text(std::string& line, u_short& iter_index, const std::uint16_t len)
{
    std::string amount = line.substr(len + 1);
    strip(amount);
    ++iter_index;
    return std::stoi(amount);
}

static RAM::RAM_t get_amount() noexcept
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
            memory_infos.free_amount = get_from_text(line, iter_index, "MemAvailable:"_len);

        else if (hasStart(line, "MemTotal:"))
            memory_infos.total_amount = get_from_text(line, iter_index, "MemTotal:"_len);

        else if (hasStart(line, "SwapFree:"))
            memory_infos.swap_free_amount = get_from_text(line, iter_index, "SwapFree:"_len);

        else if (hasStart(line, "SwapTotal:"))
            memory_infos.swap_total_amount = get_from_text(line, iter_index, "SwapTotal:"_len);

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

    memory_infos.swap_used_amount =
        memory_infos.swap_total_amount -
        memory_infos.swap_free_amount;

    return memory_infos;
}

RAM::RAM() noexcept
{
    if (!m_bInit)
    {
        m_memory_infos = get_amount();
        m_bInit        = true;
    }
}

// clang-format off
double& RAM::free_amount() noexcept
{ return m_memory_infos.free_amount; }

double& RAM::total_amount() noexcept
{ return m_memory_infos.total_amount; }

double& RAM::used_amount() noexcept
{ return m_memory_infos.used_amount; }

double& RAM::swap_total_amount() noexcept
{ return m_memory_infos.swap_total_amount; }

double& RAM::swap_used_amount() noexcept
{ return m_memory_infos.swap_used_amount; }

double& RAM::swap_free_amount() noexcept
{ return m_memory_infos.swap_free_amount; }
