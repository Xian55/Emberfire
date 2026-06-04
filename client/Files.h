#pragma once
//
// Files.h  (reconstructed parent-level support header)
//
// Filesystem helpers in namespace Util. Reconstructed from client call sites:
//
//   Util::getFileList(dir, out)          -> fills vector<string>& with file paths under 'dir'
//   Util::readLinesFromFile(path, out)   -> appends each line of 'path' to vector<string>&
//   Util::readTextFile(path)             -> returns full file contents as std::string
//
// getFileList is observed producing paths like "scripts\\animation//<file>" which the
// caller then strips with strReplaceAll -- so entries are prefixed with the directory.
//
// Compilable standalone under MSVC x86, C++17.
//
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Util
{
    // Recursively (and at top level) collect files under 'directory'.
    // Entries are returned as "<directory>//<filename>" to match the original
    // client behaviour (callers strip the "<directory>//" prefix afterwards).
    inline void getFileList(const std::string& directory, std::vector<std::string>& out)
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        if (!fs::exists(directory, ec))
            return;

        for (auto it = fs::recursive_directory_iterator(directory, ec);
             it != fs::recursive_directory_iterator(); it.increment(ec))
        {
            if (ec)
                break;
            if (!it->is_regular_file(ec))
                continue;

            // Preserve subdirectories (e.g. content/portraits/portraits.zip) — emitting
            // only filename() collapsed nested paths to "content//portraits.zip", which
            // zip_open could not find, so subdir zips (portraits) were never registered.
            std::error_code rec;
            std::string rel = fs::relative(it->path(), directory, rec).string();
            if (rec || rel.empty())
                rel = it->path().filename().string();
            out.push_back(directory + "//" + rel);
        }
    }

    // Append each line of the file to 'out'. Trailing CR is stripped.
    inline void readLinesFromFile(const std::string& path, std::vector<std::string>& out)
    {
        std::ifstream file(path);
        if (!file.is_open())
            return;

        std::string line;
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            out.push_back(line);
        }
    }

    // Return the whole file as a string (empty string if not found).
    inline std::string readTextFile(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return std::string();

        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
}
