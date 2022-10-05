// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt try1_x;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
uint64_t NP_var0 = current_excepts;
current_excepts = VALUE_ERROR;
try1_x = 1;
if (global_exception) goto goto0;
if (!global_exception) goto goto1;
goto0:
Exception* NP_var1 = get_exception();
if (NP_var1->type & (VALUE_ERROR)) {
try1_x = 2;
}
goto1:
try1_x = 3;
current_excepts = NP_var0;
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0