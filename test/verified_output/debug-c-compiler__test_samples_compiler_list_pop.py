// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var0;
NpList* list_pop_l;
NpList* NP_var2;
NpInt list_pop_v1;
NpList* NP_var4;
NpInt NP_var5;
NpInt list_pop_v2;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
list_pop_l = LIST_INIT(NpInt, (NpSortFunction)np_int_sort_fn, (NpSortFunction)np_int_sort_fn_rev, (NpCompareFunction)np_void_int_eq);
np_list_append(list_pop_l, &NP_var0);
NP_var0 = 2;
np_list_append(list_pop_l, &NP_var0);
NP_var0 = 3;
np_list_append(list_pop_l, &NP_var0);
NP_var2 = list_pop_l;
np_list_pop(NP_var2, -1, &list_pop_v1);
NP_var4 = list_pop_l;
NP_var5 = 0;
np_list_pop(NP_var4, NP_var5, &list_pop_v2);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0