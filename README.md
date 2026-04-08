# CMPE-206 Computer Network Design Project
https://www.nsnam.org

## Owners
- [Bhimsen Thapa Chhetri](https://github.com/bhimsenthapa1)
- [John Ian Buñag](https://github.com/ianbunag)

## Install ns3

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

### View animations
```bash
../../../netanim/build/netanim
```

> Note: Open the .xml file in the `animations` directory corresponding to your simulation to view the animation.

### Analyze the results
```bash
python3 ../../src/flow-monitor/examples/flowmon-parse-results.py monitors/<filename>.xml
```

## Adding a new simulation
1. Create a new file in `simulations` directory with the name of your simulation, e.g. `my-simulation.cc`.
2. Add your simulation code to the file with a unique function name, e.g. `void runMySimulation()`.
3. Set the log component name within the function, e.g. `NS_LOG_COMPONENT_DEFINE("my-simulation");`.
4. Add a new case to the `project` function in `project.cc` to run your simulation when the `--simulation` argument is set to the name of your simulation, e.g. `my-simulation`.
5. Build and run your simulation using the command above, replacing `example` with the name of your simulation, e.g. `my-simulation`.
