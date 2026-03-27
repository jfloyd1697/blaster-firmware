# generate_weapon_json.py
import os
import json

input_folder = "..\\assets"       # folder with WAV files
json_output = "weapon_profiles.json"

weapon_profiles = []

for root, dirs, files in os.walk(input_folder):
    for file in files:
        if file.lower().endswith(".wav"):
            wav_path = os.path.join(root, file)
            rel_dir = os.path.relpath(root, input_folder)

            profile = {
                "name": os.path.splitext(file)[0],
                "file": os.path.relpath(wav_path, input_folder).replace("\\", "/"),
                "category": rel_dir.replace("\\", "/") if rel_dir != "." else "default",
            }
            weapon_profiles.append(profile)

# Save JSON
with open(json_output, "w") as f:
    json.dump(weapon_profiles, f, indent=4)

print(f"Generated JSON profiles: {json_output}")