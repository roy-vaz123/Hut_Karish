#include "ScanFiles.h"
#include <iostream>

int main() {
    ScanFiles::PortPidMap db = ScanFiles::initializeDB();

    std::cout << "Port to PID mapping:\n";
    for (const auto& [port, pid] : db) {
        std::cout << "Port " << port << " → PID " << pid << '\n';
    }

    return 0;
}
