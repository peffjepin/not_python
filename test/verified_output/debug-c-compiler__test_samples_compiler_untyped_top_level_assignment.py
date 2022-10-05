// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt untyped_top_level_assignment_x;
NpFloat untyped_top_level_assignment_y;
NpFloat untyped_top_level_assignment_z;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
untyped_top_level_assignment_x = 1;
untyped_top_level_assignment_y = 2.0;
untyped_top_level_assignment_z = untyped_top_level_assignment_y;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0