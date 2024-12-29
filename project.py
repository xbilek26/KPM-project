#!/usr/bin/env python3

import subprocess
import os
from concurrent.futures import ProcessPoolExecutor

prefixes = [f"sim-{x}" for x in range(10)]

scenarios = [
        {"backBoneSpeed": "10Gbps", "backBoneDelay": "5ms", "uesDataRate": "5Mbps", "videoDataRate": "10Mbps", "outputPrefix": prefixes[0]},
        {"backBoneSpeed": "20Gbps", "backBoneDelay": "2ms", "uesDataRate": "1Mbps", "videoDataRate": "5Mbps", "outputPrefix": prefixes[1]},
        {"backBoneSpeed": "10Gbps", "backBoneDelay": "10ms", "uesDataRate": "10Mbps", "videoDataRate": "20Mbps", "outputPrefix": prefixes[2]},
        {"backBoneSpeed": "1Gbps", "backBoneDelay": "50ms", "uesDataRate": "2Mbps", "videoDataRate": "5Mbps", "outputPrefix": prefixes[3]},
        {"backBoneSpeed": "5Gbps", "backBoneDelay": "5ms", "uesDataRate": "500Kbps", "videoDataRate": "15Mbps", "outputPrefix": prefixes[4]},
        {"backBoneSpeed": "50Gbps", "backBoneDelay": "1ms", "uesDataRate": "1Mbps", "videoDataRate": "2Mbps", "outputPrefix": prefixes[5]},
        {"backBoneSpeed": "500Mbps", "backBoneDelay": "100ms", "uesDataRate": "5Mbps", "videoDataRate": "10Mbps", "outputPrefix": prefixes[6]},
        {"backBoneSpeed": "5Gbps", "backBoneDelay": "10ms", "uesDataRate": "3Mbps", "videoDataRate": "8Mbps", "outputPrefix": prefixes[7]},
        {"backBoneSpeed": "1Gbps", "backBoneDelay": "200ms", "uesDataRate": "1Mbps", "videoDataRate": "2Mbps", "outputPrefix": prefixes[8]},
        {"backBoneSpeed": "10Gbps", "backBoneDelay": "10ms", "uesDataRate": "5Mbps", "videoDataRate": "10Mbps", "outputPrefix": prefixes[9]},
    ]

parameters = []
for scenario in scenarios:
    parameters.append(' '.join([f'--{key}={value}' for key, value in scenario.items()]))


commands = [f'./ns3 run scratch/KPM-project/script.cc -- {parameter}' for parameter in parameters]

cpus = int(os.sysconf("SC_NPROCESSORS_ONLN")) - 1



def my_parallel_command(command):
    print(command)
    subprocess.run(command, shell=True)

with ProcessPoolExecutor(max_workers = cpus) as executor:
    futures = executor.map(my_parallel_command, commands)
    pass

subprocess.run("mkdir output", shell=True)
for i in range(len(prefixes)):
    with open(f"output/{prefixes[i]}.cmd.txt", "w") as file:
        file.write(commands[i])

subprocess.run(f"cp -rf {' '.join([f'{x}**' for x in prefixes])} output", shell=True)