// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyInt if_else_stmt_x;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1+2;
if (NP_var0) {
if_else_stmt_x = 10;
goto goto0;
}
if_else_stmt_x = 20;
goto0:
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0