import os
import sqlite3
import matplotlib.pyplot as plt

input_dir = "data/"

results = {
    "bal": {},
    "sat": {},
    "hib": {}
}

fixed_nodes = [60, 120, 180, 240, 300]

files = os.listdir(input_dir)

for filename in files:
    if not filename.endswith(".db"):
        continue

    parts = filename.replace(".db", "").split("-")

    scenario = parts[1]
    policy = parts[2]

    nodes = int(scenario.replace("nodes", ""))

    db_path = os.path.join(input_dir, filename)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    total = 600

    cursor.execute("SELECT COUNT(*) FROM APPLICATIONS WHERE FINISH = 1")
    success = cursor.fetchone()[0]

    conn.close()

    percent_success = (success * 100.0) / total

    if nodes not in results[policy]:
        results[policy][nodes] = []

    results[policy][nodes].append(percent_success)

for policy in ["bal", "sat", "hib"]:

    scenarios = []
    averages = []

    for nodes in fixed_nodes:
        scenarios.append(nodes)

        if nodes in results[policy]:
            values = results[policy][nodes]
            avg = sum(values) / len(values)
        else:
            avg = 0 

        averages.append(avg)

    plt.figure()

    plt.plot(scenarios, averages, marker='o')

    plt.xticks(fixed_nodes)

    plt.xlabel("Número de Nós")
    plt.ylabel("% Sucesso")
    plt.title(f"Taxa de Sucesso - {policy.upper()}")

    for i, value in enumerate(averages):
        plt.text(scenarios[i], value, f"{value:.2f}",
                 ha='center', va='bottom')

    plt.grid()

    output_name = f"success_rate_{policy}.png"
    plt.savefig(output_name)

    print(f"Gerado: {output_name}")
