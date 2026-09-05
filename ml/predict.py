"""Compatibility entry for the JSON-only forecast worker; never opens SQLite."""

from .service import main

if __name__ == '__main__':
  main()
