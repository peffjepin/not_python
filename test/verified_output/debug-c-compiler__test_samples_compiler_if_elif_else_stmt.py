
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt if_elif_else_stmt_x;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_1;
_np_1 = 1+2;
if (_np_1) {
if_elif_else_stmt_x = 1;
goto _np_0;
}
NpInt _np_2;
_np_2 = 2+3;
if (_np_2) {
if_elif_else_stmt_x = 2;
goto _np_0;
}
NpInt _np_3;
_np_3 = 3+4;
if (_np_3) {
if_elif_else_stmt_x = 3;
goto _np_0;
}
if_elif_else_stmt_x = 4;
_np_0:
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0