
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt x; NpInt y; } object_initialization_A;

// DECLARATIONS COMPILER SECTION
object_initialization_A* object_initialization_a1;
object_initialization_A* object_initialization_a2;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
if (global_exception) {
return 1;
}
object_initialization_a1 = np_alloc(16);
NpInt _np_0;
_np_0 = 1;
NpInt _np_1;
_np_1 = 1;
object_initialization_a1->x = _np_0;
object_initialization_a1->y = _np_1;
if (global_exception) {
return 1;
}
object_initialization_a2 = np_alloc(16);
NpInt _np_2;
_np_2 = 2;
NpInt _np_3;
_np_3 = 2;
object_initialization_a2->x = _np_3;
object_initialization_a2->y = _np_2;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0