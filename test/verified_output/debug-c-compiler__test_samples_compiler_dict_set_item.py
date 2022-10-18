
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="ten", .length=3},
{.data="twenty", .length=6}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_set_item_d;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
dict_set_item_d = DICT_INIT(NpInt, NpString, np_void_int_eq);
NpDict* _np_0;
_np_0 = dict_set_item_d;
NpInt _np_2;
_np_2 = 10;
NpInt _np_1;
_np_1 = _np_2;
NpString _np_3;
_np_3 = NOT_PYTHON_STRING_CONSTANTS[0];
np_dict_set_item(_np_0, &_np_1, &_np_3);
NpDict* _np_4;
_np_4 = dict_set_item_d;
NpInt _np_6;
_np_6 = 20;
NpInt _np_5;
_np_5 = _np_6;
NpString _np_7;
_np_7 = NOT_PYTHON_STRING_CONSTANTS[1];
np_dict_set_item(_np_4, &_np_5, &_np_7);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0