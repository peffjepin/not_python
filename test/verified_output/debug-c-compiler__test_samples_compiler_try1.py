
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt try1_x;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
uint64_t _np_1;
_np_1 = current_excepts;
current_excepts = VALUE_ERROR;
try1_x = 1;
if (global_exception) {
goto _np_2;
}
if (!global_exception) {
goto _np_0;
}
_np_2:
Exception* _np_3;
_np_3 = get_exception();
if (_np_3->type & (VALUE_ERROR)) {
try1_x = 2;
goto _np_0;
}
_np_0:
try1_x = 3;
current_excepts = _np_1;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0