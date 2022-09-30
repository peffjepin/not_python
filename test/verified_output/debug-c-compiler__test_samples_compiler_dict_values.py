// FORWARD COMPILER SECTION
#include <not_python.h>
PyString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// STRUCT DECLARATIONS COMPILER SECTION

// VARIABLE DECLARATIONS COMPILER SECTION
PyInt NP_var0;
PyInt NP_var1;
PyDict* dict_values_d;
PyDict* NP_var4;
PyIter NP_var2;
PyInt NP_var5;

// FUNCTION DECLARATIONS COMPILER SECTION

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NP_var0 = 1;
NP_var1 = 2;
dict_values_d = DICT_INIT(PyInt, PyInt, void_int_eq);
dict_set_item(dict_values_d, &NP_var0, &NP_var1);
NP_var0 = 3;
NP_var1 = 4;
dict_set_item(dict_values_d, &NP_var0, &NP_var1);
NP_var4 = dict_values_d;
NP_var2 = dict_iter_vals(NP_var4);
void* NP_var6 = NULL;
while ((NP_var6 = NP_var2.next(NP_var2.iter),
NP_var5 = (NP_var6) ? *(PyInt*)(NP_var6) : NP_var5,
NP_var6)){
}
}

// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
}
exitcode=0