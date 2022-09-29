def my_function(value: str):
    print(value)


fnref: Function[[str], str] = my_function
fnref("hello")
