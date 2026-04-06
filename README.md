# CMPE-206 Computer Network Design Project
https://www.nsnam.org

## Owners
- [Bhimsen Thapa Chhetri](https://github.com/bhimsenthapa1)
- [John Ian Buñag](https://github.com/ianbunag)

## Quick Start

### Clone ns-3 with git
```bash
git clone https://gitlab.com/nsnam/ns-3-dev.git
cd ns-3-dev
git checkout -b ns-3.45-branch ns-3.45
```

### Clean build
```bash
./ns3 clean
```

### Build
```bash
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

### Verify build
```bash
./ns3 run first
```

### Clone simulations repository into scratch directory
```bash
cd scratch
git clone git@github.com:ianbunag/cmpe-206-computer-network-design-project.git
```

### Run a simulation
```bash
./ns3 run --cwd=scratch/cmpe-206-computer-network-design-project 'project --simulation=example'
```

### Change the log level
```bash
NS_LOG=example=level_info ./ns3 run --cwd=scratch/cmpe-206-computer-network-design-project 'project --simulation=example'
```

## Adding a new simulation
1. Create a new file in `simulations` directory with the name of your simulation, e.g. `my-simulation.cc`.
2. Add your simulation code to the file with a unique function name, e.g. `void runMySimulation()`.
3. Set the log component name within the function, e.g. `NS_LOG_COMPONENT_DEFINE("my-simulation");`.
4. Add a new case to the `project` function in `project.cc` to run your simulation when the `--simulation` argument is set to the name of your simulation, e.g. `my-simulation`.
5. Build and run your simulation using the command above, replacing `example` with the name of your simulation, e.g. `my-simulation`.
