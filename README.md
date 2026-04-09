# CMPE-206 Computer Network Design Project

Computer Network Design Project to analyze the performance of TCP and UDP traffic under different conditions using the ns-3 network simulator.

See [simulations.md](./simulations.md) for descriptions of the simulations and results.

## Owners
- [Bhimsen Thapa Chhetri](https://github.com/bhimsenthapa1)
- [John Ian Buñag](https://github.com/ianbunag)

## Install ns-3

https://www.nsnam.org

### Clone ns-3 with git
```bash
git clone https://gitlab.com/nsnam/ns-3-dev.git
cd ns-3-dev
git checkout -b ns-3.45-branch ns-3.45
```

### Clean ns-3 build
```bash
./ns3 clean
```

### Build ns-3
```bash
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

### Verify ns-3 build
```bash
./ns3 run first
```

## Install netanim

### Clone netanim outside the ns-3 directory with git
```bash
git clone https://gitlab.com/nsnam/netanim.git
cd netanim
```

### Build netanim
```bash
mkdir build && cd build
cmake .. && cmake --build .
```

## Install Wireshark

https://www.wireshark.org/download.html

## Clone simulations repository

All commands below are run in the simulations' directory. ns-3 and netanim binaries are referenced with relative paths.

### Structure
- animations - contains netanim animation files
- monitors - contains flow monitor XML files
- simulations - contains the C++ source code for the simulations
- project.cc - contains the main function that runs the simulations based on command line arguments

### Clone cmpe-206-computer-network-design-project into scratch directory with git
```bash
cd ns-3-dev/scratch
git clone git@github.com:ianbunag/cmpe-206-computer-network-design-project.git
cd cmpe-206-computer-network-design-project
```

### Run a simulation
```bash
./../../ns3 run 'project --simulation=example'
```

> See simulations directory for available simulations.

### View animations
```bash
../../../netanim/build/netanim
```

> Open the .xml file in the `animations` directory corresponding to your simulation to view the animation.

### Gather metrics

#### Throughput, Mean Delay, and Packet Loss Ratio
```bash
python3 ../../src/flow-monitor/examples/flowmon-parse-results.py monitors/<filename>.xml
```

> Jitter sum can be located in the flow monitor XML file.

#### Retransmission and duplicate ACK counts

1. Open Wireshark
2. Open the .pcap file in the `monitors` directory corresponding to your simulation.
3. Click Analyze > Expert Information to view the retransmission and duplicate ACK counts.

## Adding a new simulation
1. Create a new file in `simulations` directory with the name of your simulation, e.g. `my-simulation.cc`.
2. Add your simulation code to the file with a unique function name, e.g. `void runMySimulation()`.
3. Set the log component name within the function, e.g. `NS_LOG_COMPONENT_DEFINE("my-simulation");`.
4. Add a new case to the `project` function in `project.cc` to run your simulation when the `--simulation` argument is set to the name of your simulation, e.g. `my-simulation`.
5. Build and run your simulation using the command above, replacing `example` with the name of your simulation, e.g. `my-simulation`.
