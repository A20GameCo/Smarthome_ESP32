import os
import sys
import argparse
from typing import Optional


def run_tests(force_compiler: Optional[str] = None, clean: bool = False) -> bool:
    if clean:
        if os.path.isdir("build"):
            print("Removing 'build' directory")
            os.system("rm -rf build")

    if not os.path.isdir("build"):
        os.system("mkdir build")

    compiler_select = " "
    if force_compiler:
        compiler_select = f" -DCMAKE_C_COMPILER={force_compiler} -DCMAKE_CXX_COMPILER={force_compiler} "
        print(f"Forcing '{force_compiler}' compiler")
    # success = os.system(f"cd build && cmake .. -DUSE_GCOV=yes {compiler_select}&& make") == 0
    success = os.system(f"cd build && cmake ..{compiler_select}&& make") == 0
    if success:
        # success = os.system("make all && make tests && make coverage")
        xml_option = "--output-junit ../test_reports/desktop_tests.xml"  # TODO: not happenin
        log_option = "--output-log ../test_reports/desktop_tests.log"
        success = os.system(f"ctest --test-dir build {xml_option} {log_option}")
    return success


def parse_args():
    """Parses the arguments given to the script"""
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', type=str, help='Compiler to use')
    parser.add_argument('--clean', action="store_true", help='Cleans the build dir before compiling', default=False)
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    if args.compiler:
        test_status = run_tests(args.compiler, clean=args.clean)
    else:
        test_status = run_tests(clean=args.clean)

    if test_status:
        sys.exit(0)
    sys.exit(1)
