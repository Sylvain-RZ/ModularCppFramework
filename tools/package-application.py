#!/usr/bin/env python3
"""
package-application.py - Cross-platform application packaging tool for ModularCppFramework
Python wrapper that works on Windows, Linux, and macOS
"""

import sys
import os
import argparse
import subprocess
import platform
import shutil
from pathlib import Path


def get_cpu_count():
    """Get number of CPU cores"""
    try:
        import multiprocessing
        return multiprocessing.cpu_count()
    except:
        return 4


def print_info(msg):
    """Print info message"""
    print(f"[INFO] {msg}", file=sys.stderr)


def print_success(msg):
    """Print success message"""
    print(f"[SUCCESS] {msg}", file=sys.stderr)


def print_warning(msg):
    """Print warning message"""
    print(f"[WARNING] {msg}", file=sys.stderr)


def print_error(msg):
    """Print error message"""
    print(f"[ERROR] {msg}", file=sys.stderr)


def create_parser():
    """Create argument parser"""
    parser = argparse.ArgumentParser(
        description='Package MCF applications into distributable archives',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Package a single application
  %(prog)s -t package-my_app

  # Package with clean build
  %(prog)s -t package-my_app --clean

  # Package all MCF examples
  %(prog)s -t package-mcf-examples -c Release

  # Package, extract, and test
  %(prog)s -t package-my_app --extract --test

  # Package and copy to distribution directory
  %(prog)s -t package-my_app -o /tmp/dist
        """)

    parser.add_argument('-t', '--target', required=True,
                        help='Package target name (e.g., package-my_app, package-mcf-examples)')
    parser.add_argument('-b', '--build-dir', default='build',
                        help='Build directory (default: build)')
    parser.add_argument('-c', '--config', default='Release',
                        choices=['Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel'],
                        help='Build configuration type (default: Release)')
    parser.add_argument('-o', '--output', default='',
                        help='Copy package to output directory after build')
    parser.add_argument('-j', '--jobs', type=int, default=0,
                        help='Number of parallel build jobs (default: auto-detect)')
    parser.add_argument('--clean', action='store_true',
                        help='Clean build directory before building')
    parser.add_argument('--extract', action='store_true',
                        help='Extract package after building for verification')
    parser.add_argument('--test', action='store_true',
                        help='Run basic tests on the extracted package')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Enable verbose output')

    return parser


def clean_build_directory(build_dir):
    """Clean build directory"""
    print_info(f"Cleaning build directory: {build_dir}")
    if build_dir.exists():
        shutil.rmtree(build_dir)
        print_success("Build directory cleaned")
    else:
        print_warning("Build directory does not exist, skipping clean")


def configure_cmake(build_dir, config, verbose):
    """Configure CMake"""
    print_info("Configuring CMake...")

    cache_file = build_dir / 'CMakeCache.txt'
    if not cache_file.exists():
        build_dir.mkdir(parents=True, exist_ok=True)

        cmake_cmd = ['cmake', '-B', str(build_dir), f'-DCMAKE_BUILD_TYPE={config}']

        if verbose:
            cmake_cmd.append('-DCMAKE_VERBOSE_MAKEFILE=ON')

        print_info(f"Running: {' '.join(cmake_cmd)}")
        result = subprocess.run(cmake_cmd, check=True)

        print_success("CMake configuration complete")
    else:
        print_info("Using existing CMake configuration")


def build_project(build_dir, config, jobs, verbose):
    """Build the project"""
    print_info("Building project dependencies...")

    build_cmd = ['cmake', '--build', str(build_dir), '--config', config, '-j', str(jobs)]

    if verbose:
        build_cmd.append('--verbose')

    print_info(f"Running: {' '.join(build_cmd)}")
    result = subprocess.run(build_cmd, check=True)

    print_success("Build complete")


def create_package(build_dir, target, verbose):
    """Create package"""
    print_info(f"Creating package: {target}")

    package_cmd = ['cmake', '--build', str(build_dir), '--target', target]

    if verbose:
        package_cmd.append('--verbose')

    # Execute package command
    if verbose:
        result = subprocess.run(package_cmd, check=True)
    else:
        result = subprocess.run(package_cmd, check=True,
                                stdout=subprocess.DEVNULL,
                                stderr=subprocess.DEVNULL)

    # Find the generated package
    package_patterns = ['*.tar.gz', '*.zip']
    package_file = None

    for pattern in package_patterns:
        packages = list(build_dir.glob(pattern))
        if packages:
            package_file = packages[0]
            break

    if not package_file:
        print_error(f"Package file not found in {build_dir}")
        return None

    print_success(f"Package created: {package_file.name}")
    return package_file


def extract_package(package_file):
    """Extract package for testing"""
    print_info("Extracting package for verification...")

    import tempfile
    extract_dir = Path(tempfile.mkdtemp(prefix='mcf-package-test-'))

    if package_file.suffix == '.gz':
        import tarfile
        with tarfile.open(package_file, 'r:gz') as tar:
            tar.extractall(extract_dir)
    elif package_file.suffix == '.zip':
        import zipfile
        with zipfile.ZipFile(package_file, 'r') as zip_ref:
            zip_ref.extractall(extract_dir)
    else:
        print_error(f"Unknown package format: {package_file}")
        return None

    print_success(f"Package extracted to: {extract_dir}")

    # List contents
    print_info("Package contents:")
    for item in extract_dir.rglob('*'):
        if item.is_file():
            rel_path = item.relative_to(extract_dir)
            print(f"  {rel_path}", file=sys.stderr)

    return extract_dir


def test_package(extract_dir):
    """Test package"""
    print_info("Running basic package tests...")

    # Find the actual package directory (first subdirectory)
    subdirs = [d for d in extract_dir.iterdir() if d.is_dir()]
    if not subdirs:
        print_error(f"Could not find package directory in {extract_dir}")
        return False

    package_dir = subdirs[0]
    print_info(f"Package directory: {package_dir}")

    # Check directory structure
    print_info("Checking directory structure...")

    errors = 0

    bin_dir = package_dir / 'bin'
    if not bin_dir.exists():
        print_warning("Missing bin/ directory")
        errors += 1
    else:
        print_success("bin/ directory found")
        executables = list(bin_dir.glob('*'))
        if platform.system() == 'Windows':
            executables = [e for e in executables if e.suffix == '.exe']
        else:
            executables = [e for e in executables if os.access(e, os.X_OK)]
        print_info(f"  Executables: {len(executables)}")

    plugins_dir = package_dir / 'plugins'
    if plugins_dir.exists():
        print_success("plugins/ directory found")
        plugin_patterns = ['*.so', '*.dll', '*.dylib']
        plugins = []
        for pattern in plugin_patterns:
            plugins.extend(plugins_dir.glob(pattern))
        print_info(f"  Plugins: {len(plugins)}")

    config_dir = package_dir / 'config'
    if config_dir.exists():
        print_success("config/ directory found")
        configs = list(config_dir.rglob('*'))
        configs = [c for c in configs if c.is_file()]
        print_info(f"  Config files: {len(configs)}")

    readme = package_dir / 'README.txt'
    if readme.exists():
        print_success("README.txt found")
        print_info("README contents:")
        print("----------------------------------------")
        with open(readme, 'r') as f:
            lines = f.readlines()[:20]
            print(''.join(lines))
        print("----------------------------------------")
    else:
        print_warning("Missing README.txt")

    if errors == 0:
        print_success("All package tests passed")
    else:
        print_warning(f"Package tests completed with {errors} warnings")

    print_info(f"Extracted package location: {package_dir}")
    print_info(f"You can manually test by running: cd {package_dir} && ./bin/<executable>")

    return errors == 0


def copy_to_output(package_file, output_dir):
    """Copy package to output directory"""
    print_info("Copying package to output directory...")

    output_dir.mkdir(parents=True, exist_ok=True)
    output_file = output_dir / package_file.name
    shutil.copy2(package_file, output_file)

    print_success(f"Package copied to: {output_file}")


def main():
    """Main entry point"""
    parser = create_parser()
    args = parser.parse_args()

    # Ensure target starts with "package-"
    if not args.target.startswith('package-'):
        print_warning("Target name should start with 'package-'. Prepending it automatically.")
        args.target = f"package-{args.target}"

    # Auto-detect number of CPU cores
    if args.jobs == 0:
        args.jobs = get_cpu_count()

    # If --test is specified, also extract
    if args.test:
        args.extract = True

    print_info("MCF Application Packaging Tool")
    print_info("=" * 31)
    print_info(f"Package Target: {args.target}")
    print_info(f"Build Directory: {args.build_dir}")
    print_info(f"Build Type: {args.config}")
    print_info(f"Build Jobs: {args.jobs}")
    print("")

    build_dir = Path(args.build_dir)
    output_dir = Path(args.output) if args.output else None

    try:
        # Execute packaging workflow
        if args.clean:
            clean_build_directory(build_dir)

        configure_cmake(build_dir, args.config, args.verbose)
        build_project(build_dir, args.config, args.jobs, args.verbose)

        package_file = create_package(build_dir, args.target, args.verbose)
        if not package_file:
            print_error("Packaging failed")
            return 1

        extract_dir = None
        if args.extract:
            extract_dir = extract_package(package_file)

        if args.test and extract_dir:
            test_package(extract_dir)

        if output_dir:
            copy_to_output(package_file, output_dir)

        print("")
        print_success("Packaging complete!")
        print_info(f"Package location: {package_file}")

        if output_dir:
            print_info(f"Output location: {output_dir / package_file.name}")

        if extract_dir:
            print_info(f"Extracted for testing: {extract_dir}")

        return 0

    except subprocess.CalledProcessError as e:
        print_error(f"Command failed: {e}")
        return 1
    except FileNotFoundError as e:
        print_error(f"File not found: {e}")
        return 1
    except Exception as e:
        print_error(f"Unexpected error: {e}")
        return 1


if __name__ == '__main__':
    sys.exit(main())
