#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <exception>
#include <algorithm>

struct BfExecutionState {
    std::ifstream codeFile;

    std::deque<unsigned char> code;
    std::size_t codePosition;
    
    std::deque<unsigned char> memory;
    decltype(memory)::iterator memoryPosition;

    // Note: If beginCodePosition == endCodePosition, then we don't know where the end of the jump is yet
    struct ConditionalJump {
        std::size_t beginCodePosition;
        std::size_t endCodePosition;
    };
    
    std::vector<ConditionalJump> conditionalJumps;
};

static void executeInstructionIncrementPtr(BfExecutionState &state)
{
    if (++state.memoryPosition == state.memory.end()) {
        state.memory.push_back(0);
        state.memoryPosition = state.memory.end() - 1;
    }
}

static void executeInstructionDecrementPtr(BfExecutionState &state)
{
    if (state.memoryPosition == state.memory.begin()) {
        state.memory.push_front(0);
        state.memoryPosition = state.memory.begin();
    } else
        --state.memoryPosition;
}

static bool findJump(BfExecutionState &state, std::size_t &position)
{
    auto conditionalJump = std::find_if(state.conditionalJumps.begin(), state.conditionalJumps.end(), [&state, &position](const BfExecutionState::ConditionalJump &x)
    {
        return x.beginCodePosition == position;
    });

    if (conditionalJump == state.conditionalJumps.end())
        throw std::runtime_error("Failed to find jump !!!");

    if (conditionalJump->beginCodePosition != conditionalJump->endCodePosition) {
        position = conditionalJump->endCodePosition;
        return true;
    }
    return false;
}

static void readToNextCloseConditionalJump(BfExecutionState &state)
{
    while (true) {
        unsigned char newInstruction;
        state.codeFile >> newInstruction;

        if (state.codeFile.eof())
            throw std::runtime_error("Found [ without matching ]");

        state.code.push_back(newInstruction);

        if (newInstruction == '[')
            state.conditionalJumps.push_back({state.code.size() - 1, state.code.size() - 1});
        else if (newInstruction == ']') {
            auto conditionalJump = std::find_if(state.conditionalJumps.rbegin(), state.conditionalJumps.rend(), [](const BfExecutionState::ConditionalJump &x)
            {
                return x.beginCodePosition == x.endCodePosition;
            });
            if (conditionalJump == state.conditionalJumps.rend())
                throw std::runtime_error("Found ] without matching [");

            conditionalJump->endCodePosition = (state.code.size() - 1);
            return;
        }
    }
}

static void executeInstructionConditionalJumpForward(BfExecutionState &state)
{
    auto conditionalJump = std::find_if(state.conditionalJumps.begin(), state.conditionalJumps.end(), [&state](const BfExecutionState::ConditionalJump &x)
    {
        return x.beginCodePosition == state.codePosition;
    });

    if (conditionalJump == state.conditionalJumps.end()) {
        state.conditionalJumps.push_back({
                state.codePosition,
                state.codePosition,
            });
        conditionalJump = state.conditionalJumps.end() - 1;
    }

    auto jumpPosition = state.codePosition;
    while (!findJump(state, jumpPosition))
        readToNextCloseConditionalJump(state);

    if (*state.memoryPosition != 0)
        return;

    state.codePosition = jumpPosition;
}

static void executeInstructionConditionalJumpBackwards(BfExecutionState &state)
{
    auto conditionalJump = std::find_if(state.conditionalJumps.begin(), state.conditionalJumps.end(), [&state](const BfExecutionState::ConditionalJump &x)
    {
        return x.endCodePosition == state.codePosition;
    });

    if (conditionalJump == state.conditionalJumps.end())
        throw std::runtime_error("Found ] without corresponding [");

    if (*state.memoryPosition != 0)
        state.codePosition = conditionalJump->beginCodePosition;
}

static void executeInstruction(BfExecutionState &state)
{
    switch (state.code[state.codePosition]) {
    case '+':
        ++*state.memoryPosition;
        break;
        
    case '-':
        --*state.memoryPosition;
        break;
        
    case '.':
        std::cout << *state.memoryPosition;
        break;

    case ',':
        *state.memoryPosition = std::cin.get();
        break;

    case '>':
        executeInstructionIncrementPtr(state);
        break;

    case '<':
        executeInstructionDecrementPtr(state);
        break;

    case '[':
        executeInstructionConditionalJumpForward(state);
        break;

    case ']':
        executeInstructionConditionalJumpBackwards(state);
        break;
    }

    ++state.codePosition;
}

static int program(std::vector<std::string> args)
{
    if (args.size() != 2)
        throw std::runtime_error("Expected 1 argument: file");

    BfExecutionState state;
    state.codeFile.open(args.at(1));
    state.codePosition = 0;
    state.memory.push_back(0);
    state.memoryPosition = state.memory.begin();

    // Execute one instruction
    while (true) {
        if (state.code.size() == state.codePosition) {
            // Need to read an instruction
            unsigned char newInstruction;

            state.codeFile >> newInstruction;
            if (state.codeFile.eof()) {
                state.codeFile.close();
                return 0;
            }

            state.code.push_back(newInstruction);
            state.codePosition = state.code.size() - 1;
        }

        executeInstruction(state);
    }
}

int main(int argc, char *argv[])
{
    try {
        const std::vector<std::string> args(argv, argv + argc);
        return program(args);
    } catch (std::exception &exc) {
        std::cerr << "Error (stdexcept): " << exc.what() << '\n';
        return 84;
    } catch (...) {
        std::cerr << "Error: Unknown exception !!!!!!\n";
        return 84;
    }
}
