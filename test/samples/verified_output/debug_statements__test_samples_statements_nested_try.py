try:
    try:
        NO_OP
        NO_OP
    except Exception:
        NO_OP
        NO_OP
    else:
        NO_OP
        NO_OP
    finally:
        NO_OP
        NO_OP
except (Exception1, Exception2, Exception3) as exc:
    NO_OP
else:
    try:
        NO_OP
    except Exception:
        NO_OP
finally:
    NO_OP
EOF

exitcode=0