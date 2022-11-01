
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt a; NpInt b; } object_initialization2_A;

// DECLARATIONS COMPILER SECTION
object_initialization2_A* object_initialization2_a1;
object_initialization2_A* object_initialization2_a2;
object_initialization2_A* object_initialization2_a3;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
object_initialization2_a1 = np_alloc(16);
if (global_exception) {
return 1;
}
NpInt _np_0;
_np_0 = 1;
NpInt _np_1;
_np_1 = 1;
object_initialization2_a1->a = _np_0;
object_initialization2_a1->b = _np_1;
object_initialization2_a2 = np_alloc(16);
if (global_exception) {
return 1;
}
NpInt _np_2;
_np_2 = 2;
NpInt _np_3;
_np_3 = 1;
object_initialization2_a2->a = _np_2;
object_initialization2_a2->b = _np_3;
object_initialization2_a3 = np_alloc(16);
if (global_exception) {
return 1;
}
NpInt _np_4;
_np_4 = 3;
NpInt _np_5;
_np_5 = 3;
object_initialization2_a3->a = _np_4;
object_initialization2_a3->b = _np_5;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0