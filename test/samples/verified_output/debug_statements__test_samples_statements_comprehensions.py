EXPR: 
(i
 for i in [1, 2, 3])
EXPR: 
[i
 for i in [1, 2, 3]]
EXPR: 
{k: v
 for (k, v) in [(1, 2), (3, 4), (5, 6)]}
EXPR: 
((x + y)
 for x in [1, 2, 3]
 for y in [4, 5, 6])
EXPR: 
[(x + y)
 for x in [1, 2, 3]
 for y in [4, 5, 6]]
EXPR: 
{(x + y): (y - x)
 for x in [1, 2, 3]
 for y in [4, 5, 6]}
EXPR: 
(i
 for i in (range __call__ (10))
 if (i < 5))
EOF

exitcode=0