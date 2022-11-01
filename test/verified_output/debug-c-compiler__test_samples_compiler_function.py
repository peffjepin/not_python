
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpFloat _np_0(NpContext __ctx__, NpInt x, NpInt y);
NpFunction function_function;

// FUNCTION DEFINITIONS COMPILER SECTION
NpFloat _np_0(NpContext __ctx__, NpInt x, NpInt y) {
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
function_function.addr = _np_0;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0