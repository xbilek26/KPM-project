# LTE Network Simulation in NS-3 – Assignment 05

## How to run the project in ns3

### 1. Navigate to the `scratch` directory
```bash
cd /path/to/ns-3/scratch
```

### 2. Clone the repository
```bash
git clone https://github.com/xbilek26/KPM-project.git
```

### 3. Navigate back to the ns3 root directory
```bash
cd ..
```

### 4. Open in VS Code and edit
```bash
code .
```

### 4. Run the simulation
```bash
./ns3 run "scratch/KPM-project/script.cc"
```

For automated simulation run AFTER first run
```bash
python3 scratch/KPM-project/project.py
```

## Task Checklist

### Instructions for Use
- Mark each checkbox as you complete the corresponding task.
- Keep this README updated to track your progress throughout the project.

### Scenario Definition
Define a network scenario including:
- [x] At least two eNodeBs.
- [x] A minimum of five User Equipment (UE) devices.
- [x] At least one remote host (server) on the Internet.
- [x] Appropriate IP addressing and routing configuration.

### Mobility
Assign appropriate mobility models for the eNodeBs and UEs:
- [x] Geographically distribute eNodeBs and UEs to create a realistic network topology.
- [x] Use two different position allocators for the eNodeBs and the UEs.
- [x] Choose a suitable mobility model for the UEs to mimic the movement of pedestrians.

### Traffic Generation
Generate traffic using NS-3 applications:
- [x] Two UEs are exchanging files between each other.
- [x] Three UEs are streaming videos from the Internet.

### Simulation Run
- [x] Execute the simulation with different sets of parameters and collect data on network performance.
- [x] Run the simulation for a sufficient duration to observe network behavior.
- [x] Capture and analyze key metrics, such as throughput, latency, and packet loss.

### Analysis and Report
Prepare a detailed report:
- [ ] Description of the network scenario.
- [ ] Configuration settings, including LTE and application parameters.
- [ ] Results and analysis of network performance.
- [ ] Any issues encountered during the simulation and how they were resolved.
- [ ] Recommendations for improving network performance (if applicable).

## Submission Checklist
- [ ] Submit the report in a digital format, along with necessary simulation scripts and configuration files.
- [ ] Prepare a presentation (max. 10 min) highlighting the key ideas and results of the project.
