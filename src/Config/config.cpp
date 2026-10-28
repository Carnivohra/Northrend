#include "Config/config.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace Northrend {

Config::Config(const std::filesystem::path& path)
    : document(path) {}

void Config::load(const std::filesystem::path& path) {
    this->document.load(path);
}

void Config::save(const std::filesystem::path& path) const {
    this->document.save(path);
}

bool Config::contains(std::string_view name) const noexcept {
    return this->find(name) != nullptr;
}

std::string Config::value(
    std::string_view name,
    std::string fallback) const {
    const Wtf::Command* command = this->find(name);
    return command == nullptr ? std::move(fallback) : command->at(2);
}

void Config::set(std::string name, std::string value) {
    if (name.empty() || name.find_first_of(" \t\r\n\"") != std::string::npos) {
        throw std::invalid_argument("Invalid configuration name");
    }

    if (value.find_first_of("\r\n") != std::string::npos) {
        throw std::invalid_argument(
            "Configuration values must not contain newlines");
    }

    Wtf::Command* command = this->find(name);
    if (command != nullptr) {
        command->at(2) = std::move(value);
        return;
    }

    this->document.commands().push_back(
        { "SET", std::move(name), std::move(value) });
}

Wtf::Command* Config::find(std::string_view name) noexcept {
    Wtf::Commands& commands = this->document.commands();

    for (auto command = commands.rbegin(); command != commands.rend(); ++command) {
        if (command->size() >= 3 &&
            Config::equal_ascii(command->at(0), "SET") &&
            Config::equal_ascii(command->at(1), name)) {
            return &*command;
        }
    }

    return nullptr;
}

const Wtf::Command* Config::find(std::string_view name) const noexcept {
    const Wtf::Commands& commands = this->document.commands();

    for (auto command = commands.rbegin(); command != commands.rend(); ++command) {
        if (command->size() >= 3 &&
            Config::equal_ascii(command->at(0), "SET") &&
            Config::equal_ascii(command->at(1), name)) {
            return &*command;
        }
    }

    return nullptr;
}

bool Config::equal_ascii(
    std::string_view left,
    std::string_view right) noexcept {
    return left.size() == right.size() &&
        std::equal(
            left.begin(),
            left.end(),
            right.begin(),
            [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) ==
                    std::tolower(static_cast<unsigned char>(b));
            });
}

}
