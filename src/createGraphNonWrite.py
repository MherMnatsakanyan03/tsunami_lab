import matplotlib.pyplot as plt
import re
import os

current_dir = os.path.dirname(__file__)
data_folder = '../docs/project-report/source/_static/content/M2_Data'
path = os.path.join(current_dir, data_folder)

def read_data(file_path):
    total_time_data = None  # Temporäre Variable zur Speicherung der Gesamt-'write time'
    with open(file_path, 'r') as file:
        for line in file:
            if 'total write time:' in line:  # Überprüfen Sie, ob es sich um die Gesamt-'write time' handelt
                total_time_data = line.split('total write time:')[1].strip()  # Speichern der Gesamt-'write time'
    total_time_ms = convert_time_to_ms(total_time_data) if total_time_data else None
    return total_time_ms




# Funktion zur Umrechnung der Zeit in Millisekunden
def convert_time_to_ms(time_str):
    time_units = {'s': 1000 * 1000, 'ms': 1000, 'us': 1}
    total_time_ms = 0
    for time_part in re.findall(r'(\d+)(\w+)', time_str):
        time_value, unit = time_part
        total_time_ms += int(time_value) * time_units[unit]
    return total_time_ms

def extract_number_from_filename(filename):
    match = re.search(r'\d+', filename)
    if match:
        return int(match.group())
    return None


# Datenstrukturen für die Ergebnisse
data = {
    'cpu_non_write': [],
    'opencl_non_write': []
}

# Dateinamen und zugehörige Schlüssel im 'data' Dictionary
files_keys = [
    ('cpu_tohoku_500_non_write_parallel', 'cpu_non_write'),
    ('cpu_tohoku_1000_non_write_parallel', 'cpu_non_write'),
    ('cpu_tohoku_2000_non_write_parallel', 'cpu_non_write'),
    ('cpu_tohoku_4000_non_write_parallel', 'cpu_non_write'),
    ('cpu_tohoku_8000_non_write_parallel', 'cpu_non_write'),
    ('opencl_tohoku_500_non_write_parallel', 'opencl_non_write'),
    ('opencl_tohoku_1000_non_write_parallel', 'opencl_non_write'),
    ('opencl_tohoku_2000_non_write_parallel', 'opencl_non_write'),
    ('opencl_tohoku_4000_non_write_parallel', 'opencl_non_write'),
    ('opencl_tohoku_8000_non_write_parallel', 'opencl_non_write'),
   
    
    
]

# Dateien einlesen und Daten verarbeiten
for file_name, key in files_keys:
    size = extract_number_from_filename(file_name)
    if size is not None:
        file_path = os.path.join(path, file_name)
        time_ms = read_data(file_path)
        if time_ms is not None:
            data[key].append((size, time_ms))


# Daten visualisieren
for key, values in data.items():
    sizes = [item[0] for item in values]
    times = [item[1] for item in values]
    plt.plot(sizes, times, marker='o', label=key)  # Hinzufügen des 'marker' Parameters

plt.xlabel('Data Size')
plt.ylabel('Time (ms)')
plt.title('Write Time Comparison')
plt.legend()
plt.show()