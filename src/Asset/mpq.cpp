#include "Asset/mpq.hpp"

#include <StormLib.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace Northrend {

MpqArchive::MpqArchive(const std::filesystem::path& path)
    : path(path) {
    const std::string archivePath = this->path.string();
    HANDLE archive = nullptr;

    if (!SFileOpenArchive(
            archivePath.c_str(),
            0,
            MPQ_OPEN_READ_ONLY,
            &archive)) {
        throw std::runtime_error(
            "Failed to open MPQ archive: " +
            archivePath +
            " (StormLib error " +
            std::to_string(SErrGetLastError()) +
            ')');
    }

    this->handle = archive;
}

MpqArchive::~MpqArchive() {
    if (this->handle) {
        SFileCloseArchive(this->handle);
    }
}

bool MpqArchive::contains(const AssetId& asset) const {
    const std::string name = entry_name(asset);
    const std::scoped_lock lock(this->access);

    return SFileHasFile(this->handle, name.c_str());
}

AssetData MpqArchive::read(const AssetId& asset) const {
    const std::string name = entry_name(asset);
    const std::scoped_lock lock(this->access);
    HANDLE openedFile = nullptr;

    if (!SFileOpenFileEx(
            this->handle,
            name.c_str(),
            SFILE_OPEN_FROM_MPQ,
            &openedFile)) {
        throw std::runtime_error(
            "Failed to open MPQ asset: " +
            std::string(asset.path()) +
            " in " +
            this->path.string() +
            " (StormLib error " +
            std::to_string(SErrGetLastError()) +
            ')');
    }

    const auto closeFile = [](void* file) noexcept {
        SFileCloseFile(file);
    };
    const std::unique_ptr<void, decltype(closeFile)> file(
        openedFile,
        closeFile);

    DWORD sizeHigh = 0;
    SErrSetLastError(ERROR_SUCCESS);
    const DWORD sizeLow =
        SFileGetFileSize(file.get(), &sizeHigh);

    if (sizeLow == SFILE_INVALID_SIZE &&
        SErrGetLastError() != ERROR_SUCCESS) {
        throw std::runtime_error(
            "Failed to determine MPQ asset size: " +
            std::string(asset.path()) +
            " (StormLib error " +
            std::to_string(SErrGetLastError()) +
            ')');
    }

    const std::uint64_t size =
        (static_cast<std::uint64_t>(sizeHigh) << 32U) |
        sizeLow;

    if (size > std::numeric_limits<std::size_t>::max()) {
        throw std::length_error(
            "MPQ asset exceeds the platform address space: " +
            std::string(asset.path()));
    }

    std::vector<std::byte> bytes(
        static_cast<std::size_t>(size));
    std::size_t offset = 0;

    while (offset < bytes.size()) {
        const std::size_t remaining = bytes.size() - offset;
        const DWORD requested = static_cast<DWORD>(
            std::min<std::size_t>(
                remaining,
                std::numeric_limits<DWORD>::max()));
        DWORD bytesRead = 0;

        if (!SFileReadFile(
                file.get(),
                bytes.data() + offset,
                requested,
                &bytesRead,
                nullptr) ||
            bytesRead != requested) {
            throw std::runtime_error(
                "Failed to read MPQ asset: " +
                std::string(asset.path()) +
                " (StormLib error " +
                std::to_string(SErrGetLastError()) +
                ')');
        }

        offset += bytesRead;
    }

    return AssetData(std::move(bytes));
}

std::string MpqArchive::entry_name(const AssetId& asset) {
    std::string name(asset.path());
    std::ranges::replace(name, '/', '\\');
    return name;
}

}
