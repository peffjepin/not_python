// FORWARD COMPILER SECTION
#include <not_python.h>
PYSTRING NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PYLIST list_for_each_my_list;
PYINT NP_var2;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
PYINT NP_var0 = 1;
list_for_each_my_list = LIST_INIT(PYINT);
LIST_APPEND(list_for_each_my_list, PYINT, NP_var0);
NP_var0 = 2;
LIST_APPEND(list_for_each_my_list, PYINT, NP_var0);
NP_var0 = 3;
LIST_APPEND(list_for_each_my_list, PYINT, NP_var0);
PYLIST NP_var1 = list_for_each_my_list;
LIST_FOR_EACH(NP_var1, PYINT, NP_var2, NP_var3)
{
PYINT NP_var4 = NP_var2;
PYSTRING NP_var5 = np_int_to_str(NP_var4);
builtin_print(1, NP_var5);
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0