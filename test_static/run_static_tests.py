import os
import sys
import argparse
from typing import Optional


def run_tests(force_compiler: Optional[str] = None) -> bool:
    if not os.path.isdir("build"):
        os.system("mkdir build")

    compiler_select = " "
    if force_compiler:
        compiler_select = f" -DCMAKE_C_COMPILER={force_compiler} -DCMAKE_CXX_COMPILER={force_compiler} "
        print(f"Forcing '{force_compiler}' compiler")
    success = os.system(f"cd build && cmake ..{compiler_select}&& make && ctest") == 0
    return success


def parse_args():
    """Parses the arguments given to the script"""
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', type=str, help='Compiler to use')
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    if args.compiler:
        test_status = run_tests(args.compiler)
    else:
        test_status = run_tests()

    if test_status:
        sys.exit(0)
    sys.exit(1)
