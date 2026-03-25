import os
import sqlite3
import matplotlib.pyplot as plt
import numpy as np

input_dir = "data/"

# Estrutura: policy -> nodes -> timestamp -> lista de valores
data = {}

for filename in os.listdir(input_dir):
    parts = filename.replace(".db", "").split("-")

    nodes = int(parts[1].replace("nodes", ""))
    policy = parts[2]

    db_path = os.path.join(input_dir, filename)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    cursor.execute("SELECT TIMESTAMP, AVG_POWER FROM BATTERY_MONITORING ORDER BY TIMESTAMP")
    rows = cursor.fetchall()

    conn.close()

    if policy not in data:
        data[policy] = {}

    if nodes not in data[policy]:
        data[policy][nodes] = {}

    for timestamp, power in rows:
        if timestamp not in data[policy][nodes]:
            data[policy][nodes][timestamp] = []

        data[policy][nodes][timestamp].append(power)

avg_data = {}

for policy in data:
    avg_data[policy] = {}

    for nodes in data[policy]:
        avg_data[policy][nodes] = []

        for timestamp in sorted(data[policy][nodes].keys()):
            values = data[policy][nodes][timestamp]
            mean = sum(values) / len(values)

            avg_data[policy][nodes].append((timestamp, mean))

for policy in avg_data:

    plt.figure()

    for nodes in sorted(avg_data[policy].keys()):
        times = [0]          # ⬅️ começa com hora 0
        power = [100.0]      # ⬅️ bateria cheia

        for timestamp, mean in avg_data[policy][nodes]:
            hour = int(timestamp / 3600)
            times.append(hour)
            power.append(mean)

        plt.plot(times, power, marker='o', label=f"{nodes} nodes")

        #for x, y in zip(times, power):
        #    plt.text(x, y, f"{y:.1f}", fontsize=7, ha='center', va='bottom')

    plt.xticks(range(0, 25))

    plt.xlabel("Tempo (horas)")
    plt.ylabel("Bateria média (%)")
    plt.title(f"Evolução da bateria - {policy}")
    plt.legend()
    plt.grid()
    plt.ylim(0, 100)
    plt.xlim(0, 25)


    plt.savefig(f"battery_{policy}.png")
    plt.close()