#include "simulations/example.cc"
#include "simulations/first.cc"

#include "ns3/core-module.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    std::string simulation = "";
    CommandLine cmd;

    cmd.AddValue("simulation", "Simulation name", simulation);
    cmd.Parse(argc, argv);

    if (simulation == "example") {
        runExample();
    } else if (simulation == "first") {
        runFirst();
    } else {
        NS_LOG_UNCOND("Simulation not found");
    }

    return 0;
}
