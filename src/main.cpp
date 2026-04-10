#include "counter.hpp"
#include "utils.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    try {
        const AppConfig config = parseArguments(argc, argv);

        if (config.showHelp) {
            printUsage(argv[0]);
            return 0;
        }

        ObjectCounter app(config);
        return app.run();
    } catch (const std::exception& exception) {
        std::cerr << "Error: " << exception.what() << "\n\n";
        printUsage(argv[0]);
        return 1;
    }
}
