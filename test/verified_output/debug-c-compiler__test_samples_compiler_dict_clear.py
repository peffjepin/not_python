// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
NpInt NP_var0;
NpInt NP_var1;
NpDict* dict_clear_d;
NpDict* NP_var3;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = 2;
dict_clear_d = DICT_INIT(NpInt, NpInt, np_void_int_eq);
np_dict_set_item(dict_clear_d, &NP_var0, &NP_var1);
NP_var0 = 3;
NP_var1 = 4;
np_dict_set_item(dict_clear_d, &NP_var0, &NP_var1);
NP_var3 = dict_clear_d;
np_dict_clear(NP_var3);
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0