// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt while_x;
PyBool NP_var0;
PyInt NP_var1;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
while_x = 0;
for(;;) {
NP_var0 = while_x<10;
if (!NP_var0) break;
NP_var1 = 1;
while_x = while_x+NP_var1;
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0