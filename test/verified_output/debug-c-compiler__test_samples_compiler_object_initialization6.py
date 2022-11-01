
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="__init__", .length=8}};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt x; NpInt y; } object_initialization6_A;

// DECLARATIONS COMPILER SECTION
object_initialization6_A* object_initialization6_a;
NpNone _np_0(NpContext __ctx__);
NpFunction object_initialization6_A___init__;
NpFunction _np_5;

// FUNCTION DEFINITIONS COMPILER SECTION
NpNone _np_0(NpContext __ctx__) {
object_initialization6_A* _np_1;
_np_1 = (object_initialization6_A*)__ctx__.self;
NpInt _np_2;
_np_2 = 1;
_np_1->x = _np_2;
object_initialization6_A* _np_3;
_np_3 = (object_initialization6_A*)__ctx__.self;
NpInt _np_4;
_np_4 = 1;
_np_3->y = _np_4;
return 0;
}

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
object_initialization6_A___init__.__addr__ = _np_0;
object_initialization6_A___init__.__name__ = NOT_PYTHON_STRING_CONSTANTS[1];
object_initialization6_a = np_alloc(16);
if (global_exception) {
return 1;
}
_np_5 = object_initialization6_A___init__;
NpContext _np_6;
_np_6 = _np_5.__ctx__;
_np_6.self = object_initialization6_a;
_np_5.__ctx__ = _np_6;
NpNone _np_7;
_np_7 = ((NpNone (*)(NpContext))_np_5.__addr__)(_np_5.__ctx__);
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