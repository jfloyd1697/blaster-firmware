# batch_mp3_to_wav.py
import os
import subprocess

# -----------------------------
# Config
# -----------------------------
input_folder = "mp3_files"   # folder containing your mp3s
output_folder = "..\\assets"     # folder to output WAVs

# Create output folder if it doesn't exist
os.makedirs(output_folder, exist_ok=True)

# -----------------------------
# Conversion loop using ffmpeg
# -----------------------------
for filename in os.listdir(input_folder):
    if filename.lower().endswith(".mp3"):
        mp3_path = os.path.join(input_folder, filename)
        wav_filename = os.path.splitext(filename)[0] + ".wav"
        wav_path = os.path.join(output_folder, wav_filename)

        # ffmpeg command: convert to 16-bit PCM, 44100Hz
        command = [
            "ffmpeg",
            "-y",                     # overwrite if exists
            "-i", mp3_path,           # input file
            "-ar", "44100",           # sample rate
            "-ac", "2",               # stereo channels
            "-sample_fmt", "s16",     # 16-bit PCM
            wav_path
        ]

        subprocess.run(command, check=True)
        print(f"Converted {filename} → {wav_filename}")

print("All MP3s converted to WAV!")