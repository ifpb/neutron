import os
import sqlite3
import csv
import matplotlib.pyplot as plt
import numpy as np

input_dir = "data/"
output_dir = "results/"

os.makedirs(output_dir, exist_ok=True)

results = {}

files = os.listdir(input_dir)

for filename in files:

    if not filename.endswith(".db"):
        continue

    db = os.path.join(input_dir, filename)

    parts = filename.replace(".db", "").split("-")

    scenario = parts[1]
    policy = parts[2]
    exec_id = int(parts[3])

    print(f"Lendo: {scenario} | {policy} | execução {exec_id}")

    conn = sqlite3.connect(db)
    cursor = conn.cursor()

    cursor.execute("SELECT COUNT(*) FROM WORKERS")
    total_workers = cursor.fetchone()[0]

    cursor.execute("SELECT COUNT(DISTINCT ID_WORKER) FROM WORKERS_APPLICATIONS")
    used_count = cursor.fetchone()[0]

    conn.close()

    pused = round((used_count * 100.0) / total_workers, 2)

    print(
        f"  workers={total_workers} "
        f"| usados={used_count} "
        f"| taxa={pused}%"
    )

    if scenario not in results:
        results[scenario] = {}

    if exec_id not in results[scenario]:
        results[scenario][exec_id] = {
            "bal": None,
            "sat": None,
            "hib": None
        }

    results[scenario][exec_id][policy] = pused

for scenario in results:

    output_path = os.path.join(output_dir, f"pused_{scenario}.csv")

    with open(output_path, "w", newline="") as f:

        writer = csv.writer(f)
        writer.writerow(["puBal", "puSat", "puHib"])

        for exec_id in sorted(results[scenario].keys()):

            bal = results[scenario][exec_id]["bal"]
            sat = results[scenario][exec_id]["sat"]
            hib = results[scenario][exec_id]["hib"]

            writer.writerow([bal, sat, hib])

    print(f"CSV gerado: {output_path}")

summary = {
    "bal": {},
    "sat": {},
    "hib": {}
}

for scenario in results:

    for policy in ["bal", "sat", "hib"]:

        values = []

        for exec_id in results[scenario]:

            value = results[scenario][exec_id][policy]

            if value is not None:
                values.append(value)

        avg = sum(values) / len(values)

        nodes = int(scenario.replace("nodes", ""))

        summary[policy][nodes] = avg

        print(f"MÉDIA | {scenario} | {policy} = {avg:.2f}%")

for policy in ["bal", "sat", "hib"]:

    plt.figure(figsize=(8, 5))

    x = sorted(summary[policy].keys())
    y = [summary[policy][nodes] for nodes in x]

    plt.plot(x, y, marker='o')

    for xi, yi in zip(x, y):
        plt.text(xi, yi, f"{yi:.2f}", fontsize=10,
                 ha='center', va='bottom')

    plt.xlabel("Número de Nós")
    plt.ylabel("% de Utilização Histórica")
    plt.title(f"Taxa de Utilização - {policy.upper()}")

    plt.ylim(0, 100)

    plt.xticks([60, 120, 180, 240, 300])

    plt.grid(True)

    output_graph = os.path.join(output_dir, f"pused_{policy}.png")

    plt.savefig(output_graph)
    plt.close()

    print(f"Gráfico gerado: {output_graph}")