#ifndef _CAVA_HPP
#define _CAVA_HPP

#if CAVA

#include <string>
#include <chrono>

extern "C" {
#include "cava/common.h"
}
using namespace std::literals::chrono_literals;

class Cava
{
public:
    Cava();
    ~Cava();
    void update();
private:
    struct error_s m_error{};          // cava errors
    struct config_params m_prm{};      // cava parameters
    struct audio_raw m_audio_raw{};    // cava handled raw audio data(is based on audio_data)
    struct audio_data m_audio_data{};  // cava audio data
    struct cava_plan* m_plan;          //{new cava_plan{}};

    // Cava API to read audio source
    ptr m_input_source;
    // Delay to handle audio source
    std::chrono::milliseconds m_frame_time_milsec{1s};
    // Text to display
    std::string m_text;
    int m_rePaint = 1;
    
    std::chrono::seconds m_fetch_input_delay{4};
    std::chrono::seconds m_suspend_silence_delay{4};

    bool m_silence = false;
    bool m_hide_on_silence = false;

    std::string m_format_silent;

    int m_sleep_counter = 0;
};

#endif

#endif
