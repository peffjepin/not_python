class A:
    x: float

    def __int__(self, badparam: int) -> int:
        return badparam
