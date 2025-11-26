#!/usr/bin/env python3
import os
import re
import csv
from collections import defaultdict

# Configuration
folders = ["contrast-mpi-omp", "contrast-mpi", "contrast-omp"]
folders = ["contrast-omp"]
output_csv = "resumen_tiempos.csv"

# Regex to parse filename: tiempo_contrast-mpi-omp_1n_1p_iter1.txt
filename_pattern = re.compile(r"tiempo_(.+)_(\d+)n_(\d+)p_iter(\d+)\.txt")

# Keys to extract in order
keys = [
    "Grey processing",
    "Grey communication",
    "Grey write",
    "HSL processing",
    "HSL communication",
    "HSL write",
    "YUV processing",
    "YUV communication",
    "YUV write",
    "Total execution"
]

# Data structure: data[(folder, nodes, nproc)] = [ [val1, val2...], [val1, val2...] ]
data = defaultdict(list)

print("Procesando archivos de resultados...")

for folder in folders:
    results_dir = os.path.join(folder, "resultados")
    if not os.path.exists(results_dir):
        print(f"Directorio no encontrado: {results_dir}")
        continue
        
    for filename in os.listdir(results_dir):
        match = filename_pattern.match(filename)
        if match:
            carpeta = match.group(1)
            nodos = int(match.group(2))
            nproc = int(match.group(3))
            # iteracion = match.group(4) # Not needed for averaging
            
            filepath = os.path.join(results_dir, filename)
            
            try:
                with open(filepath, 'r') as f:
                    content = f.read()
                
                # Extract values for this run
                run_values = []
                all_found = True
                for key in keys:
                    # Regex: "107.886 (ms) \t taken for Grey processing"
                    # We look for the number at the start of the line containing the key
                    # or just before "(ms)"
                    pattern = re.compile(r"([\d\.]+)\s+\(ms\)\s+taken for " + re.escape(key))
                    m = pattern.search(content)
                    if m:
                        run_values.append(float(m.group(1)))
                    else:
                        # If a value is missing, we might have a failed run
                        # But let's append 0.0 or handle it. 
                        # For now, we assume valid output if file exists.
                        # If 'Total execution' is missing, the run probably crashed.
                        run_values.append(0.0)
                        if key == "Total execution":
                            all_found = False
                
                if all_found:
                    data[(carpeta, nodos, nproc)].append(run_values)
            except Exception as e:
                print(f"Error leyendo {filepath}: {e}")

# Write to CSV
print(f"Escribiendo resultados en {output_csv}...")

with open(output_csv, 'w', newline='') as csvfile:
    writer = csv.writer(csvfile)
    # Header (optional, but useful)
    header = ["Carpeta", "Nodos", "NProc"] + keys
    writer.writerow(header)
    
    # Sort keys for consistent output
    # Sort by Folder, then Nodes, then NProc
    sorted_keys = sorted(data.keys(), key=lambda x: (x[0], x[1], x[2]))
    
    for key in sorted_keys:
        carpeta, nodos, nproc = key
        runs = data[key]
        
        if not runs:
            continue
            
        # Calculate averages
        num_runs = len(runs)
        num_metrics = len(keys)
        averages = [0.0] * num_metrics
        
        for run in runs:
            for i in range(num_metrics):
                averages[i] += run[i]
        
        averages = [x / num_runs for x in averages]
        
        # Format as string with 3 decimal places
        averages_str = [f"{x:.3f}" for x in averages]
        
        writer.writerow([carpeta, nodos, nproc] + averages_str)

print("¡Hecho!")
