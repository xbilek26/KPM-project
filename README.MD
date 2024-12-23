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

### 4. Run the simulation
```bash
./ns3 run "scratch/KPM-project/script.cc"
```

## Objective
- [ ] Gain hands-on experience in simulating an LTE network using NS-3.
- [ ] Create a comprehensive network scenario, configure LTE parameters, and analyze network performance.

## Task Checklist

### 2.1 Scenario Definition
Define a network scenario including:
- [ ] At least two eNodeBs.
- [ ] A minimum of five User Equipment (UE) devices.
- [ ] At least one remote host (server) on the Internet.
- [ ] Appropriate IP addressing and routing configuration.

### 2.2 Mobility
Assign appropriate mobility models for the eNodeBs and UEs:
- [ ] Geographically distribute eNodeBs and UEs to create a realistic network topology.
- [ ] Use two different position allocators for the eNodeBs and the UEs.
- [ ] Choose a suitable mobility model for the UEs to mimic the movement of pedestrians.

### 2.3 Traffic Generation
Generate traffic using NS-3 applications:
- [ ] Two UEs are exchanging files between each other.
- [ ] Three UEs are streaming videos from the Internet.

### 2.4 Simulation Run
- [ ] Execute the simulation with different sets of parameters and collect data on network performance.
- [ ] Run the simulation for a sufficient duration to observe network behavior.
- [ ] Capture and analyze key metrics, such as throughput, latency, and packet loss.

### 2.5 Analysis and Report
Prepare a detailed report:
- [ ] Description of the network scenario.
- [ ] Configuration settings, including LTE and application parameters.
- [ ] Results and analysis of network performance.
- [ ] Any issues encountered during the simulation and how they were resolved.
- [ ] Recommendations for improving network performance (if applicable).

## Submission Checklist
- [ ] Submit the report in a digital format, along with necessary simulation scripts and configuration files.
- [ ] Prepare a presentation (max. 10 min) highlighting the key ideas and results of the project.

---

### Instructions for Use
- Mark each checkbox as you complete the corresponding task.
- Keep this README updated to track your progress throughout the project.
