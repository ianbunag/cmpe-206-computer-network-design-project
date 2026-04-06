#include "ns3/core-module.h"

using namespace ns3;

void runExample() {
    NS_LOG_COMPONENT_DEFINE("example");

    std::string message = "";
    std::cout << "Enter a custom message for the simulation (Welcome!): ";
    std::getline(std::cin, message);
    if (message.empty()) {
        message = "Welcome!";
    }

    NS_LOG_UNCOND(message);
}
