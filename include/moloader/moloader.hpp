#pragma once
#include <filesystem>
#include <vector>

namespace moloader {
bool load(const std::filesystem::path& path);
const char* gettext(const char* str);
std::string getstring(const std::string& str); 
} // namespace moloader
