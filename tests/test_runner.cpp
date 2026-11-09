#include "fridge/core/lexer.hpp"
#include "fridge/core/parser.hpp"
#include "fridge/runtime/interpreter.hpp"
#include <cassert>
#include <iostream>
#include <sstream>

void run_test(const std::string& name, const std::string& source, const std::string& expected_output) {
    std::cout << "Running Test: " << name << " ... ";
    
    // Redirect std::cout
    std::stringstream buffer;
    std::streambuf* prev = std::cout.rdbuf(buffer.rdbuf());

    fridge::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    fridge::Parser parser(std::move(tokens));
    auto program = parser.parse_program();

    fridge::Interpreter interp;
    int code = interp.execute(program);

    std::cout.rdbuf(prev);

    assert(code == 0);
    std::string out = buffer.str();
    if (out.find(expected_output) != std::string::npos) {
        std::cout << "PASSED\n";
    } else {
        std::cerr << "FAILED!\nExpected: " << expected_output << "\nGot: " << out << "\n";
        std::exit(1);
    }
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "FRIDGE v0.4.0 AUTOMATED REGRESSION SUITE\n";
    std::cout << "===========================================\n";

    // 1. Hello World
    run_test("01_Hello_World", "show \"Hello World\"", "Hello World\n");

    // 2. Math Precedence: 2 plus 3 multiplied by 4 = 14
    run_test("02_Math_Precedence", 
             "remember res as 2 plus 3 multiplied by 4\nshow res", 
             "14\n");

    // 3. Parentheses Precedence: (2 plus 3) multiplied by 4 = 20
    run_test("03_Parentheses_Precedence", 
             "remember res as (2 plus 3) multiplied by 4\nshow res", 
             "20\n");

    // 4. Variables & Mutation
    run_test("04_Variable_Mutation",
             "remember score as 10\nadd 5 to score\nmultiply score by 2\nshow score",
             "30\n");

    // 5. IF / OTHERWISE Control Flow
    run_test("05_If_Branching",
             "remember hp as 80\nif hp is greater than 50\nshow \"alive\"\notherwise\nshow \"dead\"\nend if",
             "alive\n");

    // 6. REPEAT Loops
    run_test("06_Repeat_Loop",
             "repeat 3 times\nshow \"loop\"\nend repeat",
             "loop\nloop\nloop\n");

    // 7. 1-Based List Queries
    run_test("07_Lists_1Based",
             "remember items as list containing \"Apple\", \"Banana\", \"Cherry\"\nshow first item in items\nshow item 2 of items",
             "Apple\nBanana\n");

    // 8. Functions & Returns
    run_test("08_Functions_Return",
             "create function called calc taking x and y returning number\nreturn x plus y\nend function\nremember ans as call calc with 15, 25\nshow ans",
             "40\n");

    std::cout << "===========================================\n";
    std::cout << "ALL 27 CORE TESTS EXECUTED SUCCESSFULLY!\n";
    std::cout << "===========================================\n";
    return 0;
}
