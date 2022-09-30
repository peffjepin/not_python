// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyList* list_for_each_my_list;
PyList* NP_var1;
PyInt NP_var2;
PyInt NP_var4;
PyString NP_var5;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_for_each_my_list = LIST_INIT(PyInt, (PySortFunction)pyint_sort_fn, (PySortFunction)pyint_sort_fn_rev, (PyCompareFunction)void_int_eq);
list_append(list_for_each_my_list, &NP_var0);
NP_var0 = 2;
list_append(list_for_each_my_list, &NP_var0);
NP_var0 = 3;
list_append(list_for_each_my_list, &NP_var0);
NP_var1 = list_for_each_my_list;
for(size_t i=0; i < NP_var1->count && (list_get_item(NP_var1, i, &NP_var2), 1);i++)
{
NP_var4 = NP_var2;
NP_var5 = np_int_to_str(NP_var4);
builtin_print(1, NP_var5);
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0