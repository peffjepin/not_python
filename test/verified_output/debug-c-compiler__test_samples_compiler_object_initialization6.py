
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION
typedef struct { NpInt x; NpInt y; } object_initialization6_A;

// DECLARATIONS COMPILER SECTION
object_initialization6_A* object_initialization6_a;
NpFunction object_initialization6_A___init__;

// FUNCTION DEFINITIONS COMPILER SECTION
void* _np_0(NpContext __ctx__) {
object_initialization6_A* self;
self = ((object_initialization6_A*)__ctx__.self);
object_initialization6_A* _np_1;
_np_1 = self;
_np_1->x = 1;
object_initialization6_A* _np_2;
_np_2 = self;
_np_2->y = 1;
return NULL;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
object_initialization6_A___init__.addr = &_np_0;
object_initialization6_A* _np_3;
_np_3 = np_alloc(sizeof(object_initialization6_A));
NpContext _np_5;
_np_5 = object_initialization6_A___init__.ctx;
_np_5.self = _np_3;
void* _np_4;
_np_4 = ((void* (*)(NpContext ctx))object_initialization6_A___init__.addr)(_np_5);
object_initialization6_a = _np_3;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0