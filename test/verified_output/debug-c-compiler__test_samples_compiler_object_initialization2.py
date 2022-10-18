
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt a; NpInt b; } object_initialization2_A;

// DECLARATIONS COMPILER SECTION
object_initialization2_A* object_initialization2_a1;
object_initialization2_A* object_initialization2_a2;
object_initialization2_A* object_initialization2_a3;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_initialization2_A* _np_0;
_np_0 = np_alloc(sizeof(object_initialization2_A));
NpInt _np_1;
_np_1 = 1;
NpInt _np_2;
_np_2 = 1;
_np_0->a = _np_1;
_np_0->b = _np_2;
object_initialization2_a1 = _np_0;
object_initialization2_A* _np_3;
_np_3 = np_alloc(sizeof(object_initialization2_A));
NpInt _np_4;
_np_4 = 2;
NpInt _np_5;
_np_5 = 1;
_np_3->a = _np_4;
_np_3->b = _np_5;
object_initialization2_a2 = _np_3;
object_initialization2_A* _np_6;
_np_6 = np_alloc(sizeof(object_initialization2_A));
NpInt _np_7;
_np_7 = 3;
NpInt _np_8;
_np_8 = 3;
_np_6->a = _np_7;
_np_6->b = _np_8;
object_initialization2_a3 = _np_6;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0