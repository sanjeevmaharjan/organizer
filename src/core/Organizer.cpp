#include "core/Organizer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <set>
#include <stdexcept>
#include <system_error>

namespace organizer {
namespace {

using ExtensionList = std::array<const char*, 15>;

constexpr ExtensionList documentExtensions{
    ".doc", ".docx", ".odt", ".pdf", ".ppt", ".pptx", ".rtf", ".txt",
    ".xls", ".xlsx", ".csv", ".md", ".tex", ".epub", ".json"
};
constexpr ExtensionList imageExtensions{
    ".bmp", ".gif", ".heic", ".jpeg", ".jpg", ".png", ".svg", ".tif",
    ".tiff", ".webp", ".ico", ".raw", ".psd", ".xcf", ".avif"
};
constexpr ExtensionList audioExtensions{
    ".aac", ".aiff", ".flac", ".m4a", ".mp3", ".ogg", ".opus", ".wav",
    ".wma", ".mid", ".midi", ".ape", ".amr", ".ac3", ".alac"
};
constexpr ExtensionList videoExtensions{
    ".3gp", ".avi", ".flv", ".m4v", ".mkv", ".mov", ".mp4", ".mpeg",
    ".mpg", ".webm", ".wmv", ".ts", ".m2ts", ".vob", ".ogv"
};
constexpr ExtensionList archiveExtensions{
    ".7z", ".bz2", ".gz", ".rar", ".tar", ".xz", ".zip", ".zst",
    ".tgz", ".cab", ".iso", ".dmg", ".deb", ".rpm", ".jar"
};
constexpr ExtensionList codeExtensions{
    ".c", ".cc", ".cpp", ".cs", ".css", ".go", ".h", ".hpp",
    ".html", ".java", ".js", ".jsx", ".kt", ".lua", ".py"
};

bool contains(const ExtensionList& extensions, const std::string& extension)
{
    return std::any_of(extensions.begin(), extensions.end(),
                       [&extension](const char* item) { return extension == item; });
}

bool pathEntryExists(const std::filesystem::path& path, std::error_code& error)
{
    const auto status = std::filesystem::symlink_status(path, error);
    if (error == std::errc::no_such_file_or_directory) {
        error.clear();
        return false;
    }
    return !error && status.type() != std::filesystem::file_type::not_found;
}

std::string lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return value;
}

std::string categoryFor(const std::filesystem::path& path)
{
    const auto extension = lowercase(path.extension().string());
    if (contains(documentExtensions, extension)) {
        return "Documents";
    }
    if (contains(imageExtensions, extension)) {
        return "Images";
    }
    if (contains(audioExtensions, extension)) {
        return "Audio";
    }
    if (contains(videoExtensions, extension)) {
        return "Video";
    }
    if (contains(archiveExtensions, extension)) {
        return "Archives";
    }
    if (contains(codeExtensions, extension)) {
        return "Code";
    }
    return "Other";
}

std::filesystem::path numberedPath(const std::filesystem::path& path, unsigned int number)
{
    const auto filename = path.stem().string() + " (" + std::to_string(number) + ")" +
                          path.extension().string();
    return path.parent_path() / filename;
}

std::filesystem::path availablePath(const std::filesystem::path& desired,
                                    const std::set<std::filesystem::path>& reserved)
{
    std::error_code error;
    const bool desiredExists = pathEntryExists(desired, error);
    if (error) {
        throw std::filesystem::filesystem_error("Unable to inspect destination",
                                                desired, error);
    }
    if (!desiredExists && reserved.count(desired) == 0) {
        return desired;
    }

    for (unsigned int number = 1;; ++number) {
        const auto candidate = numberedPath(desired, number);
        if (reserved.count(candidate) != 0) {
            continue;
        }
        const bool candidateExists = pathEntryExists(candidate, error);
        if (error) {
            throw std::filesystem::filesystem_error("Unable to inspect destination",
                                                    candidate, error);
        }
        if (!candidateExists) {
            return candidate;
        }
    }
}

std::string invalidCategoryDirectory(const std::filesystem::path& path,
                                     std::error_code& error)
{
    const auto status = std::filesystem::symlink_status(path, error);
    if (error == std::errc::no_such_file_or_directory) {
        error.clear();
        return {};
    }
    if (error) {
        return error.message();
    }
    if (status.type() == std::filesystem::file_type::not_found ||
        status.type() == std::filesystem::file_type::directory) {
        return {};
    }
    return "Category destination is not a directory: " + path.string();
}

} // namespace

std::vector<PlannedMove> Organizer::buildPlan(const std::filesystem::path& directory)
{
    std::error_code error;
    if (!std::filesystem::is_directory(directory, error)) {
        if (error) {
            throw std::filesystem::filesystem_error("Unable to access folder",
                                                    directory, error);
        }
        throw std::invalid_argument("The selected path is not a folder.");
    }

    std::vector<std::filesystem::path> files;
    std::filesystem::directory_iterator iterator(directory, error);
    if (error) {
        throw std::filesystem::filesystem_error("Unable to read folder", directory, error);
    }

    for (const std::filesystem::directory_iterator end; iterator != end; iterator.increment(error)) {
        if (error) {
            throw std::filesystem::filesystem_error("Unable to read folder", directory, error);
        }
        const auto status = iterator->symlink_status(error);
        if (error) {
            throw std::filesystem::filesystem_error("Unable to inspect file",
                                                    iterator->path(), error);
        }
        if (status.type() == std::filesystem::file_type::regular) {
            files.push_back(iterator->path());
        }
    }
    if (error) {
        throw std::filesystem::filesystem_error("Unable to read folder", directory, error);
    }

    std::sort(files.begin(), files.end());
    std::set<std::filesystem::path> reserved;
    std::vector<PlannedMove> plan;
    plan.reserve(files.size());
    for (const auto& source : files) {
        const auto category = categoryFor(source);
        const auto categoryDirectory = directory / category;
        const auto categoryError = invalidCategoryDirectory(categoryDirectory, error);
        if (!categoryError.empty()) {
            if (error) {
                throw std::filesystem::filesystem_error(
                    "Unable to inspect category destination", categoryDirectory, error);
            }
            throw std::invalid_argument(categoryError);
        }
        const auto desired = categoryDirectory / source.filename();
        const auto destination = availablePath(desired, reserved);
        reserved.insert(destination);
        plan.push_back({source, destination, category});
    }
    return plan;
}

OrganizeResult Organizer::execute(const std::vector<PlannedMove>& plan)
{
    OrganizeResult result;
    for (const auto& move : plan) {
        std::error_code error;
        const auto categoryError =
            invalidCategoryDirectory(move.destination.parent_path(), error);
        if (!categoryError.empty()) {
            const auto reason = error ? error.message() : categoryError;
            result.failed.push_back({move.source, reason});
            continue;
        }
        std::filesystem::create_directories(move.destination.parent_path(), error);
        if (error) {
            result.failed.push_back({move.source, error.message()});
            continue;
        }

        auto destination = move.destination;
        const auto desiredDestination =
            move.destination.parent_path() / move.source.filename();
        unsigned int collisionNumber = 1;
        bool copied = false;
        while (!copied) {
            std::filesystem::copy_file(move.source, destination,
                                       std::filesystem::copy_options::none, error);
            if (!error) {
                copied = true;
                break;
            }
            if (error != std::errc::file_exists) {
                result.failed.push_back({move.source, error.message()});
                break;
            }
            error.clear();
            destination = numberedPath(desiredDestination, collisionNumber++);
        }
        if (!copied) {
            continue;
        }

        if (!std::filesystem::remove(move.source, error) || error) {
            const auto reason = error ? error.message()
                                      : "The source file could not be removed.";
            result.failed.push_back({move.source, reason});
            continue;
        }
        result.completed.push_back({move.source, destination});
    }
    return result;
}

} // namespace organizer
