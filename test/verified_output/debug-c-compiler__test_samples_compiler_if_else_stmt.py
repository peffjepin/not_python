
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt if_else_stmt_x;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_2;
_np_2 = 1+2;
NpInt _np_1;
_np_1 = _np_2;
if (_np_1) {
if_else_stmt_x = 10;
goto _np_0;
}
if_else_stmt_x = 20;
_np_0:
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0