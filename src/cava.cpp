#include <linux/limits.h>
#if CAVA

#include <cstring>

#include "cava.hpp"
#include "util.hpp"

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
    m_frame_time_milsec = std::chrono::milliseconds((int)(1e3 / m_prm.framerate));

    // Init cava plan, audio_raw structure
    audio_raw_init(&m_audio_data, &m_audio_raw, &m_prm, m_plan);
}

#endif
