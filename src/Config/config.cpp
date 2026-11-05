#include "Config/config.hpp"

#include "Asset/Format/wtf.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace Northrend {

Config::Config(const std::filesystem::path& path) {
    this->load(path);
}

void Config::load(const std::filesystem::path& path) {
    const Wtf document(path);
    this->entries.clear();
    this->entries.reserve(document.commands().size());

    for (const Wtf::Command& command : document.commands()) {
        if (command.size() >= 3 && Config::equal_ascii(command[0], "SET")) {
            this->set(command[1], command[2]);
        }
    }
}

void Config::save(const std::filesystem::path& path) const {
    Wtf document;
    Wtf::Commands& commands = document.commands();
    commands.reserve(this->entries.size());

    for (const Entry& entry : this->entries) {
        commands.push_back({"SET", entry.name, entry.value});
    }

    document.save(path);
}

bool Config::contains(std::string_view name) const noexcept {
    return this->find(name) != nullptr;
}

std::string Config::value(std::string_view name, std::string fallback) const {
    const Entry* entry = this->find(name);
    return entry == nullptr ? std::move(fallback) : entry->value;
}

void Config::set(std::string name, std::string value) {
    if (name.empty() || name.find_first_of(" \t\r\n\"") != std::string::npos) {
        throw std::invalid_argument("Invalid configuration name");
    }

    if (value.find_first_of("\r\n") != std::string::npos) {
        throw std::invalid_argument("Configuration values must not contain newlines");
    }

    Entry* entry = this->find(name);

    if (entry != nullptr) {
        entry->value = std::move(value);
        return;
    }

    this->entries.push_back(Entry {.name = std::move(name), .value = std::move(value)});
}

Config::Entry* Config::find(std::string_view name) noexcept {
    for (Entry& entry : this->entries) {
        if (Config::equal_ascii(entry.name, name)) {
            return &entry;
        }
    }

    return nullptr;
}

const Config::Entry* Config::find(std::string_view name) const noexcept {
    for (const Entry& entry : this->entries) {
        if (Config::equal_ascii(entry.name, name)) {
            return &entry;
        }
    }

    return nullptr;
}

bool Config::equal_ascii(std::string_view left, std::string_view right) noexcept {
    return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
}

}
