% { shopt -s nullglob; for file in sections/*.tex; do echo "\\input{$file}"; done; }
