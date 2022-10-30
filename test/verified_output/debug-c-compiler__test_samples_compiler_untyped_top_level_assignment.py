
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpInt untyped_top_level_assignment_x;
NpFloat untyped_top_level_assignment_y;
NpFloat untyped_top_level_assignment_z;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
untyped_top_level_assignment_x = 1;
untyped_top_level_assignment_y = 2.000000;
untyped_top_level_assignment_z = untyped_top_level_assignment_y;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0