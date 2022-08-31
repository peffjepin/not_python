try:
    NO_OP
except Exception:
    NO_OP
try:
    NO_OP
except Exception:
    NO_OP
finally:
    NO_OP
try:
    NO_OP
except Exception:
    NO_OP
else:
    NO_OP
try:
    NO_OP
except Exception:
    NO_OP
else:
    NO_OP
finally:
    NO_OP
try:
    NO_OP
except (Exception1, Exception2):
    NO_OP
try:
    NO_OP
except Exception as exc:
    NO_OP
try:
    NO_OP
except (Exception1, Exception2) as exc:
    NO_OP
EOF

exitcode=0