(i for i in [1, 2, 3])
[i for i in [1, 2, 3]]
{k: v for (k, v) in [(1, 2), (3, 4), (5, 6)]}
(x + y for x in [1, 2, 3] for y in [4, 5, 6])
[x + y for x in [1, 2, 3] for y in [4, 5, 6]]
{x + y: y - x for x in [1, 2, 3] for y in [4, 5, 6]}
(i for i in range(10) if i < 5)
(i if i == 1 else 2*i for i in range(10) if i < 5)
