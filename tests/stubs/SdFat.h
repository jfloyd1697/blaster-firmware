#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

constexpr int O_RDONLY = 0;

class File32 {
public:
    File32() = default;
    explicit File32(std::shared_ptr<std::vector<std::uint8_t>> content)
        : m_content(std::move(content)) {}

    explicit operator bool() const { return m_content != nullptr; }

    int read(std::uint8_t* destination, std::size_t bytes) {
        if (!m_content) return 0;
        const std::size_t remaining = m_content->size() - m_position;
        const std::size_t count = std::min(remaining, bytes);
        if (count > 0) std::memcpy(destination, m_content->data() + m_position, count);
        m_position += count;
        return static_cast<int>(count);
    }

    bool available() const { return m_content && m_position < m_content->size(); }
    std::uint32_t curPosition() const { return static_cast<std::uint32_t>(m_position); }

    bool seekSet(std::uint32_t position) {
        if (!m_content || position > m_content->size()) return false;
        m_position = position;
        return true;
    }

    void close() { m_content.reset(); m_position = 0; }

private:
    std::shared_ptr<std::vector<std::uint8_t>> m_content;
    std::size_t m_position = 0;
};

class StubSd {
public:
    int openCount = 0;
    std::unordered_map<std::string, std::shared_ptr<std::vector<std::uint8_t>>> files;

    void addFile(const std::string& path, const std::vector<std::uint8_t>& data) {
        files[path] = std::make_shared<std::vector<std::uint8_t>>(data);
    }

    File32 open(const char* path, int) {
        ++openCount;
        auto it = files.find(path);
        return it == files.end() ? File32{} : File32{it->second};
    }
};
