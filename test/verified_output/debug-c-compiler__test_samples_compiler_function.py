
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="function", .length=8}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFloat _np_0(NpContext __ctx__, NpInt x, NpInt y);
NpFunction function_function;

// FUNCTION DEFINITIONS COMPILER SECTION
NpFloat _np_0(NpContext __ctx__, NpInt x, NpInt y) {
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
function_function.__addr__ = _np_0;
function_function.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0