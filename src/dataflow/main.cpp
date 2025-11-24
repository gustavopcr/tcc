#include "dataflow/dataflow.hpp"
#include <vector>
#include <iostream>

int main() {
    try {
        // 1. Create Dataflow machine and load graph
        // Ensure the path is correct relative to where you run the executable
        Dataflow df = create_dataflow("src/dataflow/program.txt");

        // 2. Define inputs
        // Graph expects: Node 10 (a), Node 20 (b)
        // Calculation: (5 + 3) * (5 - 3) = 8 * 2 = 16
        std::vector<uint64_t> inputs = {5, 3};

        // 3. Run simulation
        df.run(inputs);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}