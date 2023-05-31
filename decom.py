import os
import subprocess

directory = 'intake'
new_directory = 'logs'
out_directory = 'output'

# Loop through each file in the directory
for filename in os.listdir(directory):
    # Check if the file is a regular file
    if os.path.isfile(os.path.join(directory, filename)):
        # Construct the system command
        command = f'convert.py -f {os.path.join(directory, filename)} -o {os.path.join(new_directory, filename)} -i intake/map.json'
        # Run the system command using subprocess
        subprocess.run(command, shell=True)

        command = f'interpretPaste.py -g -p {os.path.join(new_directory, filename)} -o {os.path.join(out_directory, filename)}'
        subprocess.run(command, shell=True)
