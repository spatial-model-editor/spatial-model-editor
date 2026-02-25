if __name__ == "__main__":
    import os
    import doctest
    import sys

    # Ensure doctest plotting examples use a headless backend in CI.
    os.environ.setdefault("MPLBACKEND", "Agg")
    import sme

    result = doctest.testmod(sme)
    sys.exit(1 if result.failed > 0 else 0)
