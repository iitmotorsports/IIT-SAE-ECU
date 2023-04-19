import os
import re
import datetime

directory = 'output'
extension = '.log'
search_term = ' [Fault Check]    [ LOG ]'

for filename in os.listdir(directory):
    if filename.endswith(extension):
        with open(os.path.join(directory, filename), 'r', encoding="utf-8") as f:
            for line in f:
                if search_term in line:
                    match = re.match(r'\[(\d+)\]', line)
                    if match:
                        unix_time = int(match.group(1))
                        datetime_obj = datetime.datetime.fromtimestamp(unix_time)
                        line = f'[{datetime_obj.strftime("%Y-%m-%d %I:%M:%S %p")}] {line[len(match.group(0)):]}'
                    print(line, end="")
