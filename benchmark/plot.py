import pandas as pd
import json

benchmarks = json.load(open("bench_out.json"))["benchmarks"]

classes = set()
datasets = set()
for benchmark in benchmarks:
    name = benchmark["name"]
    classes.add(name.split("_")[0])
    datasets.add(name.split("<")[1].split(">")[0])


def repl(m):
    return m.group("one")


df = pd.DataFrame(benchmarks)
for class_name in classes:
    d = pd.DataFrame()
    for dataset_name in datasets:
        # get results
        s = df[
            df["name"].str.contains(f"{class_name}_.*<{dataset_name}.*", regex=True)
        ][["name", "cpu_time"]]
        # format function names
        s["name"] = s["name"].str.replace(
            f"{class_name}_(?P<one>.*)<{dataset_name}.*", repl, regex=True
        )
        s["name"] = s["name"].str.replace("_", "::")
        s = s.loc[s["name"].str.lower().sort_values(ascending=False).index]
        s = s.rename(columns={"cpu_time": dataset_name})
        if d.empty:
            d = s
        else:
            d[dataset_name] = s[dataset_name].values
    ax = d.plot.barh(x="name")
    ax.set_title(f"{class_name}")
    ax.set_xlabel("")
    ax.set_xscale("log")
    ax.set_xlim([1e-6, 1e3])
    ax.set_xticks([1e-6, 1e-3, 1, 1e3])
    ax.set_xticklabels(["1 ns", "1 us", "1 ms", "1 s"])
    ax.set_ylabel("")
    fig = ax.get_figure()
    fig.set_size_inches(12, 10)
    fig.savefig(f"{class_name}.png", bbox_inches="tight", dpi=100)
