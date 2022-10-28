
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt try1_x;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpUnsigned _np_2;
_np_2 = current_excepts;
current_excepts = 0;
current_excepts = current_excepts | 4;
try1_x = 1;
if (global_exception) {
goto _np_1;
}
if (!global_exception) {
goto _np_0;
}
_np_1:
Exception* _np_3;
_np_3 = get_exception();
NpUnsigned _np_4;
_np_4 = 0;
_np_4 = _np_4 | 4;
NpUnsigned _np_6;
_np_6 = _np_3->type;
NpUnsigned _np_5;
_np_5 = _np_6 & _np_4;
if (_np_5) {
try1_x = 2;
goto _np_0;
}
_np_0:
try1_x = 3;
current_excepts = _np_2;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0