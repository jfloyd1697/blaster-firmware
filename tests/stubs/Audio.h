#pragma once
inline float lastToneAmplitude = 0;
inline float lastToneFrequency = 0;
inline int mixerConnectionCount = 0;
inline void AudioMemory(int) {}
class AudioSynthWaveformSine {
public:
    void amplitude(float amplitude) { lastToneAmplitude = amplitude; }
    void frequency(float frequency) { lastToneFrequency = frequency; }
};
class AudioPlaySdWav {
public:
    bool isPlaying() const { return playing; }
    bool play(const char*) { playing = true; return true; }
    void stop() { playing = false; }
private:
    bool playing = false;
};
class AudioMixer4 { public: void gain(int, float) {} };
class AudioOutputI2S {};
class AudioControlSGTL5000 {
public:
    bool enable() { return true; }
    void volume(float) {}
};
class AudioConnection {
public:
    template<class T, class U>
    AudioConnection(T&, int, U&, int) { ++mixerConnectionCount; }
};
