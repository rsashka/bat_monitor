## Mandatory rules of this project
- Development is carried out in C++ (the use of C is allowed only in cases where C++ is not applicable, for example, when handling hardware interrupts)
- For debug output, only macros from the `unittest.h` file defined for the BUILD_UNITTEST macro should be used. Using other debugging tools is prohibited!
- Programming for html pages in pure JavaScript without additional frameworks.
- Always add or reintroduce tests for new or changed code, even if no one asked for them.
- Always run unit testing after task completion or before commit.
- Do not create missing files if their creation is not specified in the conditions of the current task.
- The source is located in the `main` directory.
- The `test` catalog contains all tests.

## Launch command to the actions
- To configure a cmake unit test, run `cmake -S tests/ -B tests/build`.
- To build a unit test, run `make -C tests/build/`.
- To run a unit test, run `tests/build/unit-test --gtest_shuffle `.
- To focus on one "test-name", run `tests/build/unit-test` with add test option: `--gtest_filter=<test-name>`.

## CodeStyle rules
- Always use spaces for indentation, with a width of 4 spaces.
- Use camelCase for variable names and use prefix `m_` for class fields.
- Explain your reasoning before providing code.
- Focus on code readability and maintainability.
- After completing the task, the final report should include the name of the model, the initial prompt for the task, and all the summary statistics on the interaction with the model.

## Rules for the `main` directory
- The esp-idf framework and the ESP32-C3 SuperMini module are used.
- This is a multi-threaded application with the FreeRTOS API.
- Each application thread inherits the `RtosTask.h` interface.
- The class file name must match the class name.

## Rules for unit testing in the `tests` directory
- Use Google Test Framework, which is already configured to work correctly.
- Use the `tests/mocks/` directory to create mock files for use in tests.
- Access to the test Wi-Fi network, network name: `GUEST-DEV`, password: `123456789`, authorization method: `WPA2-PSK`
- Access to the device's HTML page by IP address: *192.168.4.150* and *HTTP* protocol
