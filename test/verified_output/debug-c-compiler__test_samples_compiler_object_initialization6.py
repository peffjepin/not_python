// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION
typedef struct {PYINT x, PYINT y} A;

// VARIABLE DECLARATIONS COMPILER SECTION
A* a;

// FUNCTION DECLARATIONS COMPILER SECTION
void __init__(A* self);

// FUNCTION DEFINITIONS COMPILER SECTION
void __init__(A* self) {
A* NP_var0 = self;
PYINT NP_var0->x = 1;
A* NP_var1 = self;
PYINT NP_var1->y = 1;
}

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
a = np_alloc(sizeof(A));
__init__(a);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0