
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_pop_d;
NpInt dict_pop_v;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static int init_module(void) {
NpInt _np_0;
_np_0 = 1;
NpInt _np_1;
_np_1 = 2;
dict_pop_d = np_dict_init(8, 8, np_void_int_eq);
if (global_exception) {
return 1;
}
np_dict_set_item(dict_pop_d, &_np_0, &_np_1);
if (global_exception) {
return 1;
}
NpDict* _np_2;
_np_2 = dict_pop_d;
NpInt _np_3;
_np_3 = 1;
np_dict_pop_val(_np_2, &_np_3, &dict_pop_v);
if (global_exception) {
return 1;
}
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0