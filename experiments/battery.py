import os
import sqlite3
import matplotlib.pyplot as plt
import numpy as np

input_dir = "data/"
output_dir = "results/"

data = {}

for filename in os.listdir(input_dir):

    if not filename.endswith(".db"):
        continue

    parts = filename.replace(".db", "").split("-")

    nodes = int(parts[1].replace("nodes", ""))
    policy = parts[2]

    db_path = os.path.join(input_dir, filename)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    cursor.execute("""
        SELECT TIMESTAMP, AVG_POWER
        FROM BATTERY_MONITORING
        ORDER BY TIMESTAMP
    """)

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

for policy in data:

    plt.figure(figsize=(12, 6))

    for nodes in sorted(data[policy].keys()):

        times = [0]
        means = [100.0]
        ci_upper = [100.0]
        ci_lower = [100.0]

        for timestamp in sorted(data[policy][nodes].keys()):

            values = data[policy][nodes][timestamp]

            mean = np.mean(values)

            std = np.std(values)

            ci = 1.96 * (std / np.sqrt(len(values)))

            hour = int(timestamp / 3600)
            print(f"{policy} | {nodes} nós | hora {hour} | média={mean:.2f} | IC={ci:.4f}")
            times.append(hour)
            means.append(mean)

            ci_upper.append(mean + ci)
            ci_lower.append(mean - ci)

        plt.plot(times, means, marker='o', label=f"{nodes} nodes")

        plt.fill_between(
            times,
            ci_lower,
            ci_upper,
            alpha=0.4
        )

    plt.xticks(range(0, 25))

    plt.xlabel("Tempo (horas)")
    plt.ylabel("Bateria média (%)")
    plt.title(f"Evolução da bateria - {policy.upper()}")

    plt.legend()

    plt.grid()

    plt.ylim(0, 100)
    plt.xlim(0, 24)
    
    output_graph = os.path.join(output_dir, f"battery_ci_{policy}.png")

    plt.savefig(output_graph)
    plt.close()

    print(f"Gráfico gerado: {output_graph}")