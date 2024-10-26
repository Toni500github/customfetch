#if CAVA

#include "cava.hpp"

#include <linux/limits.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

#include "util.hpp"
#include "fmt/base.h"

Cava::Cava()
{
    char cfg_path[PATH_MAX] = "/home/toni/.config/cava/config";

    m_error.length = 0;
    if (!load_config(cfg_path, &m_prm, false, &m_error))
        die("Error loading config: {}", m_error.message);

    // Override cava parameters by the user config
    m_prm.inAtty = 0;
    m_prm.output = output_method::OUTPUT_RAW;
    strcpy(m_prm.data_format, "ascii");
    strcpy(m_prm.raw_target, "/dev/stdout");

    m_prm.bar_width   = 2;
    m_prm.bar_spacing = 0;
    m_prm.bar_height  = 32;
    m_prm.bar_width   = 1;
    m_prm.orientation = ORIENT_TOP;
    m_prm.xaxis       = xaxis_scale::NONE;
    m_prm.mono_opt    = AVERAGE;
    m_prm.autobars    = 0;
    m_prm.gravity     = 0;
    m_prm.integral    = 1;

    // Make cava parameters configuration
    m_plan = new cava_plan{};

    m_audio_raw.height     = m_prm.ascii_range;
    m_audio_data.format    = -1;
    m_audio_data.source    = new char[1 + strlen(m_prm.audio_source)];
    m_audio_data.source[0] = '\0';
    strcpy(m_audio_data.source, m_prm.audio_source);

    m_audio_data.rate            = 0;
    m_audio_data.samples_counter = 0;
    m_audio_data.channels        = 2;
    m_audio_data.IEEE_FLOAT      = 0;

    m_audio_data.input_buffer_size = BUFFER_SIZE * m_audio_data.channels;
    m_audio_data.cava_buffer_size  = m_audio_data.input_buffer_size * 8;

    m_audio_data.cava_in = new double[m_audio_data.cava_buffer_size]{ 0.0 };

    m_audio_data.terminate   = 0;
    m_audio_data.suspendFlag = false;
    m_input_source           = get_input(&m_audio_data, &m_prm);

    if (!m_input_source)
        die("cava API didn't provide input audio source method");

    // Calculate delay for Update() thread
    m_frame_time_milsec = std::chrono::milliseconds(static_cast<int>(1e3 / m_prm.framerate));

    // Init cava plan, audio_raw structure
    audio_raw_init(&m_audio_data, &m_audio_raw, &m_prm, m_plan);
    m_audio_raw.previous_frame[0] = -1;
}

Cava::~Cava()
{
    if (m_plan)
        delete m_plan;

    audio_raw_clean(&m_audio_raw);
}

void upThreadDelay(std::chrono::milliseconds& delay, std::chrono::seconds& delta)
{
    if (delta == std::chrono::seconds{ 0 })
    {
        delta += std::chrono::seconds{ 1 };
        delay += delta;
    }
}

void downThreadDelay(std::chrono::milliseconds& delay, std::chrono::seconds& delta)
{
    if (delta > std::chrono::seconds{ 0 })
    {
        delay -= delta;
        delta -= std::chrono::seconds{ 1 };
    }
}

std::string getIcon(const uint16_t percentage, const uint16_t max)
{
    constexpr std::array<std::string_view, 8> blocks = { "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█" };
    const auto idx = std::clamp(percentage / ((max == 0 ? 100 : max) / blocks.size()), 0LU, blocks.size() - 1);

    return blocks[idx].data();
}

void Cava::update()
{
    if (m_audio_data.suspendFlag)
        return;

    m_silence = false;

    for (int i = 0; i < m_audio_data.input_buffer_size; ++i)
    {
        if (m_audio_data.cava_in[i])
        {
            m_silence       = false;
            m_sleep_counter = 0;
            break;
        }
    }

    if (m_silence)// && m_prm.sleep_timer)
    {
        if (m_sleep_counter <= (int)(std::chrono::milliseconds(m_prm.sleep_timer * 1s) / m_frame_time_milsec))
        {
            ++m_sleep_counter;
            m_silence = false;
        }
    }

    if (!m_silence)
    {
        cava_execute(m_audio_data.cava_in, m_audio_data.samples_counter, m_audio_raw.cava_out, m_plan);

        downThreadDelay(m_frame_time_milsec, m_suspend_silence_delay);
        // Process: execute cava
        pthread_mutex_lock(&m_audio_data.lock);
        cava_execute(m_audio_data.cava_in, m_audio_data.samples_counter, m_audio_raw.cava_out, m_plan);
        if (m_audio_data.samples_counter > 0)
            m_audio_data.samples_counter = 0;
        pthread_mutex_unlock(&m_audio_data.lock);

        // Do transformation under raw data
        audio_raw_fetch(&m_audio_raw, &m_prm, &m_rePaint, m_plan);

        if (m_rePaint == 1)
        {
            m_text.clear();

            for (int i = 0; i < m_audio_raw.number_of_bars; ++i)
            {
                m_audio_raw.previous_frame[i] = m_audio_raw.bars[i];
                m_text.append(
                    getIcon((m_audio_raw.bars[i] > m_prm.ascii_range) ? m_prm.ascii_range : m_audio_raw.bars[i],
                            m_prm.ascii_range + 1));
                if (m_prm.bar_delim != 0)
                    m_text.push_back(m_prm.bar_delim);
            }

            fmt::print("{}", m_text);
        }
    }
}

#endif
