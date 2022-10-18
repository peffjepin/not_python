
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt x; NpInt y; } object_initialization_A;

// DECLARATIONS COMPILER SECTION
object_initialization_A* object_initialization_a1;
object_initialization_A* object_initialization_a2;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_initialization_A* _np_0;
_np_0 = np_alloc(sizeof(object_initialization_A));
NpInt _np_1;
_np_1 = 1;
NpInt _np_2;
_np_2 = 1;
_np_0->x = _np_1;
_np_0->y = _np_2;
object_initialization_a1 = _np_0;
object_initialization_A* _np_3;
_np_3 = np_alloc(sizeof(object_initialization_A));
NpInt _np_4;
_np_4 = 2;
NpInt _np_5;
_np_5 = 2;
_np_3->x = _np_5;
_np_3->y = _np_4;
object_initialization_a2 = _np_3;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0