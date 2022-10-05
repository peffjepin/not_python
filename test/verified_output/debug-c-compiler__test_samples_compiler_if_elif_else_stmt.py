// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var0;
NpInt if_elif_else_stmt_x;
NpInt NP_var1;
NpInt NP_var2;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1+2;
if (NP_var0) {
if_elif_else_stmt_x = 1;
goto goto0;
}
NP_var1 = 2+3;
if (NP_var1) {
if_elif_else_stmt_x = 2;
goto goto0;
}
NP_var2 = 3+4;
if (NP_var2) {
if_elif_else_stmt_x = 3;
goto goto0;
}
if_elif_else_stmt_x = 4;
goto0:
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0