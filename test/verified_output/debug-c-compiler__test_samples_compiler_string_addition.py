// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="Hello,", .length=6},
{.data=" World.", .length=7}};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpString string_addition_a;
NpString string_addition_b;
NpString string_addition_c;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
string_addition_a = NOT_PYTHON_STRING_CONSTANTS[0];
string_addition_b = NOT_PYTHON_STRING_CONSTANTS[1];
string_addition_c = np_str_add(string_addition_a, string_addition_b);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0