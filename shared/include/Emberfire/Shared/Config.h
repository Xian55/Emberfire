#pragma once
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdlib>

// INI config singleton accessed via `sConfig`. Interface reconstructed from client usage
// (Application.cpp loadCfg, ContentMgr/Connector/Options). Values may be quoted ("...") in the file.
class Config {
public:
    static Config* instance() { static Config c; return &c; }

    void setSource(const char* path, bool localPath = true) { (void)localPath; m_path = path ? path : ""; load(); }

    std::string getString(const char* section, const char* key, const char* def = "") {
        const std::string* v = find(section, key); return v ? *v : std::string(def);
    }
    int getInt(const char* section, const char* key, int def = 0) {
        const std::string* v = find(section, key); return v ? std::atoi(v->c_str()) : def;
    }
    bool getBool(const char* section, const char* key, bool def = false) {
        const std::string* v = find(section, key);
        if (!v) return def;
        return *v == "1" || *v == "true" || *v == "True" || *v == "TRUE";
    }
    void setInt(const char* section, const char* key, int value) { m_data[section][key] = std::to_string(value); save(); }
    void setString(const char* section, const char* key, const char* value) { m_data[section][key] = value ? value : ""; save(); }

    // Options cache: map an int id -> (section,key) for fast repeated lookups (Application.cpp).
    void registerCache(int id, const char* section, const char* key) { m_cache[id] = { section, key }; }
    int  getCache(int id) {
        auto it = m_cache.find(id);
        if (it == m_cache.end()) return 0;
        return getInt(it->second.first.c_str(), it->second.second.c_str(), 0);
    }

private:
    Config() = default;

    static std::string trim(const std::string& s) {
        std::size_t a = s.find_first_not_of(" \t\r\n");
        std::size_t b = s.find_last_not_of(" \t\r\n");
        if (a == std::string::npos) return "";
        std::string r = s.substr(a, b - a + 1);
        if (r.size() >= 2 && r.front() == '"' && r.back() == '"') r = r.substr(1, r.size() - 2);
        return r;
    }
    const std::string* find(const std::string& section, const std::string& key) {
        auto s = m_data.find(section);
        if (s == m_data.end()) return nullptr;
        auto k = s->second.find(key);
        return k == s->second.end() ? nullptr : &k->second;
    }
    void load() {
        m_data.clear();
        std::ifstream f(m_path);
        if (!f) return;
        std::string line, section;
        while (std::getline(f, line)) {
            std::string t = trim(line);
            if (t.empty() || t[0] == '#' || t[0] == ';') continue;
            if (t.front() == '[' && t.back() == ']') { section = t.substr(1, t.size() - 2); continue; }
            std::size_t eq = t.find('=');
            if (eq == std::string::npos) continue;
            m_data[section][trim(t.substr(0, eq))] = trim(t.substr(eq + 1));
        }
    }
    void save() {
        if (m_path.empty()) return;
        std::ofstream f(m_path);
        if (!f) return;
        for (auto& sec : m_data) {
            f << "[" << sec.first << "]\n";
            for (auto& kv : sec.second) f << kv.first << "=" << kv.second << "\n";
            f << "\n";
        }
    }
    std::string m_path;
    std::map<std::string, std::map<std::string, std::string>> m_data;
    std::map<int, std::pair<std::string, std::string>> m_cache;
};

#define sConfig Config::instance()
