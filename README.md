# CMPE-206 Computer Network Design Project

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

### (Optional) Create python virtual environment with conda
```bash
conda create --name cmpe-206-computer-network-design-project
```

### (Optional) Activate python virtual environment with conda
```bash
conda activate cmpe-206-computer-network-design-project
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
./ns3 test
```

### Clone simulations repository into scratch directory
```bash
cd scratch
git clone git@github.com:ianbunag/cmpe-206-computer-network-design-project.git
```
