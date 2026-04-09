# Simulations

## buffer-simulation

This simulation has the objective of analyzing how the buffer size affects data flow. Metrics such as throughput, latency, and packet loss are collected to understand the impact of different buffer sizes on network performance. See results and line charts in https://docs.google.com/spreadsheets/d/e/2PACX-1vT0bVNkllmD_IhFfsIunuudCD6aoQU-WdAWtDOV2CYuQgbxYoIm0zfzc7FAntmd4YDP94ovQChUXKoh/pubhtml.

The primary parameter of this simulation is the router buffer size, which is set to 2, 10, 50 (baseline), 100, and 500.

Performance is analyzed across three traffic modes - TCP, UDP, and mixed.

The topology is a standard dumbbell topology with 2 sources and 2 destinations separated by 2 routers. The sources and destinations are connected to the routers with 100 Mbps links and 1ms delay, while the routers are connected to each other with a 10 Mbps link and 10ms delay.

In the TCP mode, sources send as much data as possible with no maximum byte limit.

In the UDP mode, sources send at a constant rate of 10 Mbps and packet size of 1 Kb.

The mixed mode represents a typical real world traffic distribution by allocating 80% of the bandwidth to TCP and 20% to UDP. In this setup, one source sends TCP packets at a fixed rate of 8 Mbps and the other sends UDP packets at 2 Mbps, and both flows use a standard packet size of 1 Kb.

In the following topology, blue nodes represent TCP sources and destinations, red nodes represent UDP sources and destinations, and green nodes represent routers.

**TCP mode topology**

![TCP mode topology](./images/buffer-simulation-tcp.png)

**UDP mode topology**

![UDP mode topology](./images/buffer-simulation-udp.png)

**Mixed mode topology**

![Mixed mode topology](./images/buffer-simulation-mixed.png)
