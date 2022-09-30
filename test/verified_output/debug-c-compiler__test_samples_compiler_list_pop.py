// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyList* list_pop_l;
PyList* NP_var2;
PyInt list_pop_v1;
PyList* NP_var4;
PyInt NP_var5;
PyInt list_pop_v2;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_pop_l = LIST_INIT(PyInt, (PySortFunction)pyint_sort_fn, (PySortFunction)pyint_sort_fn_rev, (PyCompareFunction)void_int_eq);
list_append(list_pop_l, &NP_var0);
NP_var0 = 2;
list_append(list_pop_l, &NP_var0);
NP_var0 = 3;
list_append(list_pop_l, &NP_var0);
NP_var2 = list_pop_l;
list_pop(NP_var2, -1, &list_pop_v1);
NP_var4 = list_pop_l;
NP_var5 = 0;
list_pop(NP_var4, NP_var5, &list_pop_v2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0