#pragma once

#include <string>

const std::string arch();
const std::string host();
const std::string host_name();
const std::string host_version();
const std::string host_vendor();

inline std::FILE *os_release;
const std::string os_name();
const std::string os_pretty_name();
const std::string os_name_id();
