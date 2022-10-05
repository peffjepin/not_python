// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var0;
NpInt NP_var1;
NpDict* dict_pop_d;
NpDict* NP_var3;
NpInt dict_pop_v;
NpInt NP_var4;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = 2;
dict_pop_d = DICT_INIT(NpInt, NpInt, np_void_int_eq);
np_dict_set_item(dict_pop_d, &NP_var0, &NP_var1);
NP_var3 = dict_pop_d;
NP_var4 = 1;
np_dict_pop_val(NP_var3, &NP_var4, &dict_pop_v);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0