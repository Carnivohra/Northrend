#include "Config/wtf.hpp"

#include <cctype>
#include <fstream>
#include <stdexcept>

namespace Northrend {

Wtf::Wtf(const std::filesystem::path& path) {
    this->load(path);
}

void Wtf::load(const std::filesystem::path& path) {
    this->content.clear();

    std::ifstream file(path);

    if (!file) {
        if (!std::filesystem::exists(path)) {
            return;
        }

        throw std::runtime_error("Could not open WTF file: " + path.string());
    }

    std::string line;
    std::size_t number = 0;

    while (std::getline(file, line)) {
        ++number;

        try {
            Command command = Wtf::parse(line);

            if (!command.empty()) {
                this->content.emplace_back(std::move(command));
            }
        } catch (const std::invalid_argument& error) {
            throw std::runtime_error("Could not parse WTF file " + path.string() + " at line " +
                std::to_string(number) + ": " + error.what());
        }
    }

    if (file.bad()) {
        throw std::runtime_error("Could not read WTF file: " + path.string());
    }
}

void Wtf::save(const std::filesystem::path& path) const {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream file(path, std::ios::trunc);

    if (!file) {
        throw std::runtime_error("Could not open WTF file for writing: " + path.string());
    }

    for (const Command& command : this->content) {
        Wtf::write(file, command);
    }

    if (!file) {
        throw std::runtime_error("Could not write WTF file: " + path.string());
    }
}

Wtf::Commands& Wtf::commands() noexcept {
    return this->content;
}

const Wtf::Commands& Wtf::commands() const noexcept {
    return this->content;
}

Wtf::Command Wtf::parse(std::string_view line) {
    Command command;
    std::size_t position = 0;

    while (position < line.size()) {
        while (position < line.size() && std::isspace(static_cast<unsigned char>(line[position]))) {
            ++position;
        }

        if (position == line.size()) {
            break;
        }

        std::string argument;

        if (line[position] != '"') {
            const std::size_t begin = position;

            while (position < line.size() && !std::isspace(static_cast<unsigned char>(line[position]))) {
                ++position;
            }

            command.emplace_back(line.substr(begin, position - begin));
            continue;
        }

        ++position;
        bool closed = false;

        while (position < line.size()) {
            const char character = line[position++];

            if (character == '"') {
                closed = true;
                break;
            }

            if (character == '\\' && position < line.size() && (line[position] == '\\' || line[position] == '"')) {
                argument.push_back(line[position++]);
                continue;
            }

            argument.push_back(character);
        }

        if (!closed) {
            throw std::invalid_argument("unterminated quoted argument");
        }

        if (position < line.size() && !std::isspace(static_cast<unsigned char>(line[position]))) {
            throw std::invalid_argument("quoted arguments must be separated by whitespace");
        }

        command.emplace_back(std::move(argument));
    }

    return command;
}

void Wtf::write(std::ostream& stream, const Command& command) {
    for (std::size_t index = 0; index < command.size(); ++index) {
        if (index != 0) {
            stream.put(' ');
        }

        const std::string& argument = command[index];
        const bool quoted = argument.empty() || argument.find_first_of(" \t\r\n\"") != std::string::npos;

        if (!quoted) {
            stream << argument;
            continue;
        }

        stream.put('"');
        for (const char character : argument) {
            if (character == '\\' || character == '"') {
                stream.put('\\');
            }

            stream.put(character);
        }
        stream.put('"');
    }

    stream.put('\n');
}

}
