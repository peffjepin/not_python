// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="string1", .length=7},
{.data="string2", .length=7}};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpString string_literals_string1;
NpString string_literals_string2;
NpString string_literals_string2_2;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
string_literals_string1 = NOT_PYTHON_STRING_CONSTANTS[0];
string_literals_string2 = NOT_PYTHON_STRING_CONSTANTS[1];
string_literals_string2_2 = NOT_PYTHON_STRING_CONSTANTS[1];
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0