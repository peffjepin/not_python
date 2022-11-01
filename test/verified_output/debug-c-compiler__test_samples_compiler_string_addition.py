
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="Hello,", .length=6},
{.data=" World.", .length=7}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpString string_addition_a;
NpString string_addition_b;
NpString string_addition_c;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
string_addition_a = NOT_PYTHON_STRING_CONSTANTS[1];
string_addition_b = NOT_PYTHON_STRING_CONSTANTS[2];
NpString _np_0;
_np_0 = string_addition_a;
NpString _np_1;
_np_1 = string_addition_b;
string_addition_c = np_str_add(_np_0, _np_1);
if (global_exception) {
return 1;
}
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0