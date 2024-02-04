import subprocess
import os

# Pfad zum Zielverzeichnis
#target_directory = "/Users/ibyton/Desktop/Uni/tsunami_lab/docs/project-report/source/_static/content/M2_Data"

current_dir = os.path.dirname(__file__)
data_folder = '../docs/project-report/source/_static/content/M2_Data'
target_directory = os.path.join(current_dir, data_folder)

# Funktion, um einen Befehl auszuführen und den Output in einer Datei zu speichern
def run_command_and_save_output(command, output_file):
    try:
        # Vollständiger Pfad zur Output-Datei
        full_output_path = os.path.join(target_directory, output_file)

        # Ausführen des Befehls und Speichern des Outputs in der Datei
        with open(full_output_path, "w") as file:
            result = subprocess.run(command, check=True, text=True, stdout=file, stderr=subprocess.STDOUT)
            print(f"Befehl erfolgreich ausgeführt: {' '.join(command)}")
    except subprocess.CalledProcessError as e:
        # Fehlermeldung, falls der Befehl nicht erfolgreich ausgeführt wurde
        print(f"Ein Fehler ist aufgetreten bei Befehl: {' '.join(command)} - {e}")
    except FileNotFoundError:
        # Fehlermeldung, falls der Befehl nicht gefunden wurde
        print(f"Befehl nicht gefunden: {' '.join(command)}")

# Liste der Befehle
commands = [
    ['scons'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p1', '-w1', '500'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p1', '-w1', '1000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p1', '-w1', '2000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p1', '-w1', '4000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p1', '-w1', '8000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p1', '-w0', '1000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p1', '-w0', '2000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p1', '-w0', '4000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p1', '-w0', '8000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p1', '-w1', '500'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p1', '-w1', '1000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p1', '-w1', '2000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p1', '-w1', '4000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p1', '-w1', '8000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p1', '-w0', '500'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p1', '-w0', '1000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p1', '-w0', '2000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p1', '-w0', '4000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p1', '-w0', '8000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p0', '-w0', '500'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p0', '-w0', '1000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p0', '-w0', '2000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p0', '-w0', '4000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o1', '-p0', '-w0', '8000'],
    ['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p0', '-w0', '500'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p0', '-w0', '1000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p0', '-w0', '2000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p0', '-w0', '4000'],
    #['./build/tsunami_lab', '-d', '2d', '-s', 'tsunami2d', '-o0', '-p0', '-w0', '8000']
]


# Erstellen des Zielverzeichnisses, falls es noch nicht existiert
if not os.path.exists(target_directory):
    os.makedirs(target_directory)

for cmd in commands:
    # Dateinamen-Parameter extrahieren
    if cmd[0] == 'scons':
        output_filename = 'scons_output'
    else:
        o_value = 'opencl' if '-o1' in cmd else 'cpu'
        w_value = 'write' if '-w0' in cmd else 'non_write'
        p_value = 'parallel' if '-p1' in cmd else 'serial'
        
        number = cmd[-1]

        # Output-Dateinamen erstellen
        output_filename = f"{o_value}_tohoku_{number}_{w_value}_{p_value}"
    
    # Befehl ausführen und Output in der Datei speichern
    run_command_and_save_output(cmd, output_filename)
