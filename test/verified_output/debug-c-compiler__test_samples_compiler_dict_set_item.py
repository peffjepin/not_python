
// FORWARD COMPILER SECTION
#include <not_python.h>
NpString NOT_PYTHON_STRING_CONSTANTS[] = {
{.data="", .length=0},
{.data="ten", .length=3},
{.data="twenty", .length=6}};

// TYPEDEFS COMPILER SECTION

// DECLARATIONS COMPILER SECTION
NpDict* dict_set_item_d;

// FUNCTION DEFINITIONS COMPILER SECTION

// INIT MODULE FUNCTION COMPILER SECTION
static void init_module(void) {
dict_set_item_d = np_dict_init(8, 24, np_void_int_eq);
NpDict* _np_0;
_np_0 = dict_set_item_d;
NpInt _np_1;
_np_1 = 10;
NpString _np_2;
_np_2 = NOT_PYTHON_STRING_CONSTANTS[1];
np_dict_set_item(_np_0, &_np_1, &_np_2);
NpDict* _np_3;
_np_3 = dict_set_item_d;
NpInt _np_4;
_np_4 = 20;
NpString _np_5;
_np_5 = NOT_PYTHON_STRING_CONSTANTS[2];
np_dict_set_item(_np_3, &_np_4, &_np_5);
}
// MAIN FUNCTION COMPILER SECTION
int main(void) {
init_module();
return 0;
}
exitcode=0