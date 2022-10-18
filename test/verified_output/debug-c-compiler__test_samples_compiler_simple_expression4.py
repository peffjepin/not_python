
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFloat simple_expression4_a;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_2;
_np_2 = 2*3;
NpInt _np_1;
_np_1 = 1+_np_2;
NpInt _np_0;
_np_0 = _np_1;
NpInt _np_5;
_np_5 = 3*2;
NpFloat _np_4;
_np_4 = (NpFloat)_np_5/(NpFloat)4;
NpFloat _np_3;
_np_3 = _np_4;
simple_expression4_a = _np_0-_np_3;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0