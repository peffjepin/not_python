
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="string1", .length=7},
{.data="string2", .length=7}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpString string_literals_string1;
NpString string_literals_string2;
NpString string_literals_string2_2;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
string_literals_string1 = NOT_PYTHON_STRING_CONSTANTS[1];
string_literals_string2 = NOT_PYTHON_STRING_CONSTANTS[2];
string_literals_string2_2 = NOT_PYTHON_STRING_CONSTANTS[2];
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0