
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="my_function", .length=11}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__);
NpFunction void_function_my_function;

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__) {
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
void_function_my_function.__addr__ = _np_0;
void_function_my_function.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0