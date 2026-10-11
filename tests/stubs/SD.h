#pragma once
struct StubAudioSd {
    bool exists(const char*) const { return true; }
};
inline StubAudioSd SD;
