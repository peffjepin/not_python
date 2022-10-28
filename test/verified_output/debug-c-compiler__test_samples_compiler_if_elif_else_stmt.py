
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt if_elif_else_stmt_x;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_1;
_np_1 = 1;
NpInt _np_2;
_np_2 = 2;
NpInt _np_3;
_np_3 = _np_1 + _np_2;
if (_np_3) {
if_elif_else_stmt_x = 1;
goto _np_0;
}
NpInt _np_4;
_np_4 = 2;
NpInt _np_5;
_np_5 = 3;
NpInt _np_6;
_np_6 = _np_4 + _np_5;
if (_np_6) {
if_elif_else_stmt_x = 2;
goto _np_0;
}
NpInt _np_7;
_np_7 = 3;
NpInt _np_8;
_np_8 = 4;
NpInt _np_9;
_np_9 = _np_7 + _np_8;
if (_np_9) {
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