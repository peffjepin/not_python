def my_function(x: int, y: int, z: int = 3, w: int = 4) -> int:
    return x + y + z + w


a = my_function(1, 2, w=5*4+1)
