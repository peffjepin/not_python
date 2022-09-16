// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYLIST my_list;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
PYINT NP_var0 = 1;
my_list = LIST_INIT(PYINT);
LIST_APPEND(my_list, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(my_list, PYINT, NP_var0);
NP_var0 = 3;
LIST_APPEND(my_list, PYINT, NP_var0);
PYLIST NP_var1 = my_list;
LIST_FOR_EACH(NP_var1, PYINT, value, NP_var2)
{
PYINT NP_var3 = value;
builtin_print(1, np_int_to_str(NP_var3));
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0