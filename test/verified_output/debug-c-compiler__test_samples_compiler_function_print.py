
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="Hello, world", .length=12}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpString _np_1;
_np_1 = NOT_PYTHON_STRING_CONSTANTS[0];
void* _np_0;
_np_0 = builtin_print(1, _np_1);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0