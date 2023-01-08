#include <libgccjit++.h>
#include <libgccjit.h>
#include <fstream>
#include <iostream>
#include <string_view>

struct BfCompiler {
    std::string filename;
    std::ifstream inputStream;
    std::intmax_t line;
    std::intmax_t column;
    int cellArraySize;

    std::size_t numOpenJumps;
    struct OpenJump {
        gccjit::block blockTest;
        gccjit::block blockBody;
        gccjit::block blockAfter;
    };
    std::vector<OpenJump> openJumps;
 
    gccjit::context context;
    gccjit::type voidType, intType, cellType, cellArrayType;
    gccjit::function funcGetchar, funcPutchar, funcMain;
    gccjit::block currentBlock;

    gccjit::rvalue valIntZero, valIntOne, valCellZero, valCellOne, valIntHalfArraySize;
    gccjit::lvalue valCellsArray, valCellsArrayIndex;

    void throwFatalError(std::string_view error)
    {
        throw std::runtime_error(std::string(this->filename) + ":" + std::to_string(this->line) + ":" + std::to_string(this->column) + ": " + std::string(error));
    }

    gccjit::function makeMainFunc()
    {
        gccjit::type charPtrPtrType = this->context.get_type(GCC_JIT_TYPE_CHAR).get_pointer().get_pointer();
        std::vector<gccjit::param> mainParams = { this->context.new_param(this->intType, "argc"), this->context.new_param(charPtrPtrType, "argv") };

        return this->context.new_function(GCC_JIT_FUNCTION_EXPORTED, this->intType, "main", mainParams, false, this->context.new_location(this->filename, this->line, this->column));
    }

    gccjit::lvalue getCurrentCell(gccjit::location location)
    {
        return this->context.new_array_access(this->valCellsArray, this->valCellsArrayIndex, location);
    }

    void compileOneCharacter(unsigned char character)
    {
        auto location = this->context.new_location(this->filename, this->line, this->column);

        switch (character) {
        case '>':
            this->currentBlock.add_comment("> -> ++cellsArrayIndex");
            this->currentBlock.add_assignment_op(this->valCellsArrayIndex, GCC_JIT_BINARY_OP_PLUS, this->valIntOne, location);
            break;
            
        case '<':
            this->currentBlock.add_comment("< -> --cellsArrayIndex");
            this->currentBlock.add_assignment_op(this->valCellsArrayIndex, GCC_JIT_BINARY_OP_MINUS, this->valIntOne, location);
            break;

        case '+':
            this->currentBlock.add_comment("+ -> ++cellsArray[cellsArrayIndex]");
            this->currentBlock.add_assignment_op(this->getCurrentCell(location), GCC_JIT_BINARY_OP_PLUS, this->valCellOne, location);
            break;

        case '-':
            this->currentBlock.add_comment("- -> --cellsArray[cellsArrayIndex]");
            this->currentBlock.add_assignment_op(this->getCurrentCell(location), GCC_JIT_BINARY_OP_MINUS, this->valCellOne, location);
            break;

        case '.':
        {
            this->currentBlock.add_comment(". -> putchar((int)cellsArray[cellsArrayIndex])", location);
            
            std::vector<gccjit::rvalue> putcharArguments = { this->context.new_cast(this->getCurrentCell(location), this->intType) };
            this->currentBlock.add_eval(this->context.new_call(this->funcPutchar, putcharArguments, location), location);
            break;
        }

        case ',':
            this->currentBlock.add_comment(", -> cellsArray[cellsArrayIndex] = (unsigned char)getchar()", location);
            this->currentBlock.add_assignment(this->getCurrentCell(location), this->context.new_cast(this->context.new_call(this->funcGetchar, location), this->cellType, location));
            break;

        case '[':
        {
            if (this->numOpenJumps == this->openJumps.size())
                this->openJumps.resize(this->openJumps.size() + 1);

            auto loopTest = this->funcMain.new_block();
            this->currentBlock.end_with_jump(loopTest, location);

            auto onZero = this->funcMain.new_block();
            auto onNonZero = this->funcMain.new_block();
            loopTest.add_comment("[");
            loopTest.end_with_conditional(this->context.new_comparison(GCC_JIT_COMPARISON_EQ, this->valCellZero, this->getCurrentCell(location)), onZero, onNonZero, location);

            this->openJumps[this->numOpenJumps++] = { loopTest, onNonZero, onZero };
            this->currentBlock = onNonZero;
            break;
        }

        case ']':
            if (this->numOpenJumps == 0)
                this->throwFatalError("could not find corresponding [ for ]");
            --this->numOpenJumps;

            this->currentBlock.add_comment("]", location);
            this->currentBlock.end_with_jump(this->openJumps[this->numOpenJumps].blockTest, location);
            this->currentBlock = this->openJumps[this->numOpenJumps].blockAfter;
            break;
        }

        if (character == '\n') {
            ++this->line;
            this->column = 0;
        } else
            ++this->column;
    }
    
    gccjit::context compile(std::string filename)
    {
        this->filename = filename;
        this->inputStream.open(this->filename.data());
        if (!this->inputStream.is_open())
            this->throwFatalError("unable to open file");

        this->line = 1;

        this->context = gccjit::context::acquire();
#ifndef IS_JIT
        this->context.set_int_option(GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 3);
#endif
        this->context.set_bool_option(GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE, 0);
        this->context.set_bool_option(GCC_JIT_BOOL_OPTION_DEBUGINFO, 1);
        this->context.set_bool_option(GCC_JIT_BOOL_OPTION_DUMP_EVERYTHING, 0);
        this->context.set_bool_option(GCC_JIT_BOOL_OPTION_KEEP_INTERMEDIATES, 0);

        this->voidType = this->context.get_type(GCC_JIT_TYPE_VOID);
        this->intType = this->context.get_type(GCC_JIT_TYPE_INT);
        this->cellType = this->context.get_type(GCC_JIT_TYPE_UNSIGNED_CHAR);
        this->cellArraySize = sizeof(int) >= 4 ? 600000000 : 30000;
        this->cellArrayType = this->context.new_array_type(this->cellType, this->cellArraySize);

        auto initialLocation = this->context.new_location(this->filename, 1, 0);

        std::vector<gccjit::param> getcharParams = {};
        this->funcGetchar = this->context.new_function(GCC_JIT_FUNCTION_IMPORTED, this->intType, "getchar", getcharParams, false, initialLocation);

        std::vector<gccjit::param> putcharParams = { this->context.new_param(this->intType, "c") };
        this->funcPutchar = this->context.new_function(GCC_JIT_FUNCTION_IMPORTED, this->voidType, "putchar", putcharParams, false, initialLocation);

        this->funcMain = this->makeMainFunc();
        this->currentBlock = this->funcMain.new_block("initial");

        this->valIntZero = this->context.zero(this->intType);
        this->valIntOne = this->context.one(this->intType);
        this->valCellZero = this->context.zero(this->cellType);
        this->valCellOne = this->context.one(this->cellType);
        this->valIntHalfArraySize = this->context.new_rvalue(this->intType, this->cellArraySize / 2);
        
        this->valCellsArray = this->context.new_global(GCC_JIT_GLOBAL_INTERNAL, this->cellArrayType, "gCellsArray", initialLocation);
        this->valCellsArrayIndex = this->funcMain.new_local(this->intType, "cellsArrayIndex", initialLocation);

        this->currentBlock.add_comment("cellsArrayIndex = " + std::to_string(this->cellArraySize / 2));
        this->currentBlock.add_assignment(this->valCellsArrayIndex, this->valIntHalfArraySize, initialLocation);

        while (true) {
            int character = this->inputStream.get();
            if (this->inputStream.eof())
                break;
            this->compileOneCharacter((unsigned char)character);
        }

        this->currentBlock.end_with_return(this->valIntZero, this->context.new_location(this->filename, this->line, this->column));
        return this->context;
    }
};

static int program(const std::vector<std::string> &args)
{
#ifndef IS_JIT
    if (args.size() != 3)
        throw std::runtime_error(args.at(0) + ": input_file output_file");

    auto inputFile = args.at(1);
    auto outputFile = args.at(2);
    auto context = BfCompiler().compile(inputFile);

    context.compile_to_file(GCC_JIT_OUTPUT_KIND_EXECUTABLE, outputFile.c_str());

    auto firstError = gcc_jit_context_get_first_error(context.get_inner_context());
    context.release();
    return firstError != nullptr;
#else
    if (args.size() != 2)
        throw std::runtime_error(args.at(0) + ": input_file");

    auto inputFile = args.at(1);
    auto context = BfCompiler().compile(inputFile);
    
    auto result = context.compile();
    
    auto mainFunc = (int (*)(int, char **))gcc_jit_result_get_code(result, "main");
    if (mainFunc == nullptr)
        throw std::runtime_error("nullptr mainFunc");

    mainFunc(0, nullptr);

    auto firstError = gcc_jit_context_get_first_error(context.get_inner_context());
    gcc_jit_result_release(result);
    context.release();
    return firstError != nullptr;
#endif
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
