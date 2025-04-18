/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "platform.hpp"
#if CF_MACOS

#include "query.hpp"
#include "util.hpp"

using namespace Query;

Theme::Theme(const std::uint8_t ver, systemInfo_t& queried_themes, const std::string& theme_name_version,
             const Config& config, const bool gsettings_only)
            : m_queried_themes(queried_themes)
{
    m_theme_infos.cursor = m_theme_infos.gtk_font = m_theme_infos.cursor_size = m_theme_infos.gtk_theme_name = m_theme_infos.gtk_icon_theme 
        = MAGIC_LINE;
}

Theme::Theme(systemInfo_t& queried_themes, const Config& config, const bool gsettings_only) : m_queried_themes(queried_themes)
{
    m_theme_infos.cursor = m_theme_infos.gtk_font = m_theme_infos.cursor_size = m_theme_infos.gtk_theme_name = m_theme_infos.gtk_icon_theme 
        = MAGIC_LINE;
}

// clang-format off
std::string Theme::gtk_theme() noexcept
{ return m_theme_infos.gtk_theme_name; }

std::string Theme::gtk_icon_theme() noexcept
{ return m_theme_infos.gtk_icon_theme; }

std::string Theme::gtk_font() noexcept
{ return m_theme_infos.gtk_font; }

std::string& Theme::cursor() noexcept
{ return m_theme_infos.cursor; }

std::string& Theme::cursor_size() noexcept
{ return m_theme_infos.cursor_size; }

#endif
