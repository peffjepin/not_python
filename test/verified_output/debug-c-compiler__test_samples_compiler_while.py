
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt while_x;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
while_x = 0;
while(1) {
NpInt _np_1;
_np_1 = while_x;
NpInt _np_2;
_np_2 = 10;
NpBool _np_3;
_np_3 = _np_1 < _np_2;
if (!_np_3) {
break;
}
NpInt _np_4;
_np_4 = 1;
while_x = while_x + _np_4;
_np_0:
}
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0