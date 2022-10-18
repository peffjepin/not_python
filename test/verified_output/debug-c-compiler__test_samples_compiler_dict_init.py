
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_init_d;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
NpInt _np_0;
NpInt _np_1;
_np_0 = 1;
_np_1 = 2;
NpDict* _np_2;
_np_2 = DICT_INIT(NpInt, NpInt, np_void_int_eq);
np_dict_set_item(_np_2, &_np_0, &_np_1);
dict_init_d = _np_2;
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0