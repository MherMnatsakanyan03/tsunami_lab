import matplotlib.pyplot as plt
import re
import os
import json

current_dir = os.path.dirname(__file__)
data_folder = '../docs/project-report/source/_static/content/M2_Data'
path = os.path.join(current_dir, data_folder)
variable = "total write time"

def read_data(file_path):
    total_time_data = None  # Temporäre Variable zur Speicherung der Gesamt-'write time'
    with open(file_path, 'r') as file:
        for line in file:
            if variable in line:  # Überprüfen Sie, ob es sich um die Gesamt-'write time' handelt
                total_time_data = line.split(variable)[1].strip()  # Speichern der Gesamt-'write time'
    total_time_ms = convert_time_to_seconds(total_time_data) if total_time_data else None
    return total_time_ms




# Funktion zur Umrechnung der Zeit in Sekunden
def convert_time_to_seconds(time_str):
    time_units = {'s': 1, 'ms': 0.001, 'us': 0.000001, "ns": 0.000000001}
    total_time_seconds = 0
    for time_part in re.findall(r'(\d+)(\w+)', time_str):
        time_value, unit = time_part
        total_time_seconds += int(time_value) * time_units[unit]
    return total_time_seconds

def extract_number_from_filename(filename):
    match = re.search(r'\d+', filename)
    if match:
        return int(match.group())
    return None


# Datenstrukturen für die Ergebnisse
data = {
    'cpu_parallel': [],
    'cpu_serial': [],
    'opencl_parallel': [],
    'opencl_serial': []
}

# Dateinamen und zugehörige Schlüssel im 'data' Dictionary
files_keys = [
    #('cpu_tohoku_500_write_parallel', 'cpu_parallel'),
    #('cpu_tohoku_500_write_serial', 'cpu_serial'),
    ('opencl_tohoku_500_write_parallel', 'opencl_parallel'),
    ('opencl_tohoku_500_write_serial', 'opencl_serial'),
    ('cpu_tohoku_1000_write_parallel', 'cpu_parallel'),
    ('cpu_tohoku_1000_write_serial', 'cpu_serial'),
    ('opencl_tohoku_1000_write_parallel', 'opencl_parallel'),
    ('opencl_tohoku_1000_write_serial', 'opencl_serial'),
    ('cpu_tohoku_2000_write_parallel', 'cpu_parallel'),
    ('cpu_tohoku_2000_write_serial', 'cpu_serial'),
    ('opencl_tohoku_2000_write_parallel', 'opencl_parallel'),
    ('opencl_tohoku_2000_write_serial', 'opencl_serial'),
    ('cpu_tohoku_4000_write_parallel', 'cpu_parallel'),
    ('cpu_tohoku_4000_write_serial', 'cpu_serial'),
    ('opencl_tohoku_4000_write_parallel', 'opencl_parallel'),
    ('opencl_tohoku_4000_write_serial', 'opencl_serial'),
    ('cpu_tohoku_8000_write_parallel', 'cpu_parallel'),
    ('cpu_tohoku_8000_write_serial', 'cpu_serial'),
    ('opencl_tohoku_8000_write_parallel', 'opencl_parallel'),
    ('opencl_tohoku_8000_write_serial', 'opencl_serial'),
]

## Datenstrukturen für die Ergebnisse
#data = {
#    'cpu_non_write': [],
#    'opencl_non_write': [],
#}
#
## Dateinamen und zugehörige Schlüssel im 'data' Dictionary
#files_keys = [
#    ('cpu_tohoku_500_non_write_parallel', 'cpu_non_write'),
#    ('cpu_tohoku_1000_non_write_parallel', 'cpu_non_write'),
#    ('cpu_tohoku_2000_non_write_parallel', 'cpu_non_write'),
#    ('cpu_tohoku_4000_non_write_parallel', 'cpu_non_write'),
#    ('cpu_tohoku_8000_non_write_parallel', 'cpu_non_write'),
#    ('opencl_tohoku_500_non_write_parallel', 'opencl_non_write'),
#    ('opencl_tohoku_1000_non_write_parallel', 'opencl_non_write'),
#    ('opencl_tohoku_2000_non_write_parallel', 'opencl_non_write'),
#    ('opencl_tohoku_4000_non_write_parallel', 'opencl_non_write'),
#    ('opencl_tohoku_8000_non_write_parallel', 'opencl_non_write'),
#]

# Dateien einlesen und Daten verarbeiten
for file_name, key in files_keys:
    size = extract_number_from_filename(file_name)
    if size is not None:
        file_path = os.path.join(path, file_name)
        time_ms = read_data(file_path)
        if time_ms is not None:
            data[key].append((size, time_ms))



json_data = json.dumps(data)
print(json_data)
