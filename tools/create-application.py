#!/usr/bin/env python3
"""
create-application.py - Cross-platform application generator for ModularCppFramework
Python wrapper that works on Windows, Linux, and macOS
"""

import sys
import os
import argparse
import subprocess
import platform
from pathlib import Path


def get_project_root():
    """Get the project root directory"""
    script_dir = Path(__file__).parent.absolute()
    return script_dir.parent


def create_parser():
    """Create argument parser"""
    parser = argparse.ArgumentParser(
        description='MCF Application Generator - Create new applications',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s -n MyGame
  %(prog)s -n MyGame -r -c -m logger,profiling
  %(prog)s -n NetworkApp -v 2.0.0 -a "Net Team" -m networking -e -c
        """)

    parser.add_argument('-n', '--name', required=True,
                        help='Application name (required, e.g., MyGame)')
    parser.add_argument('-v', '--version', default='1.0.0',
                        help='Application version (default: 1.0.0)')
    parser.add_argument('-a', '--author', default='MCF Developer',
                        help='Application author (default: MCF Developer)')
    parser.add_argument('-d', '--description', default='Application built with ModularCppFramework',
                        help='Application description')
    parser.add_argument('-m', '--modules', default='',
                        help='Comma-separated modules (logger,networking,profiling,realtime)')
    parser.add_argument('-p', '--plugins', default='',
                        help='Comma-separated plugins to load')
    parser.add_argument('-r', '--realtime', action='store_true',
                        help='Add realtime update loop')
    parser.add_argument('-e', '--event-driven', action='store_true',
                        help='Add event-driven architecture')
    parser.add_argument('-c', '--config', action='store_true',
                        help='Generate config.json file')
    parser.add_argument('-o', '--output', default='',
                        help='Output directory (default: ./<app_name>)')

    return parser


def run_cmake_generator(args, project_root):
    """Run CMake application generator"""

    # Set default output directory
    output_dir = args.output if args.output else args.name

    # Display configuration
    print("=== MCF Application Generator ===")
    print()
    print("Configuration:")
    print(f"  Name:        {args.name}")
    print(f"  Version:     {args.version}")
    print(f"  Author:      {args.author}")
    print(f"  Description: {args.description}")
    if args.modules:
        print(f"  Modules:     {args.modules}")
    if args.plugins:
        print(f"  Plugins:     {args.plugins}")
    print("  Features:")
    if args.realtime:
        print("    - Realtime update loop")
    if args.event_driven:
        print("    - Event-driven architecture")
    if args.config:
        print("    - Configuration file (config.json)")
    if not args.realtime and not args.event_driven and not args.config:
        print("    - Basic application")
    print(f"  Output:      {output_dir}")
    print()

    # Create temporary CMake script
    build_dir = project_root / 'build'
    build_dir.mkdir(exist_ok=True)

    temp_cmake = build_dir / 'generate_application.cmake'
    cmake_dir = project_root / 'cmake'

    # Convert paths to POSIX format for CMake (forward slashes work on all platforms)
    cmake_dir_posix = cmake_dir.as_posix()
    output_dir_posix = Path(output_dir).as_posix()

    # Convert comma-separated modules to CMake list
    module_list = ''
    if args.modules:
        modules = [m.strip() for m in args.modules.split(',')]
        module_list = ' '.join(modules)

    # Convert comma-separated plugins to CMake list
    plugin_list = ''
    if args.plugins:
        plugins = [p.strip() for p in args.plugins.split(',')]
        plugin_list = ' '.join(plugins)

    modules_line = ''
    if module_list:
        modules_line = f'    list(APPEND APP_ARGS MODULES {module_list})\n'

    plugins_line = ''
    if plugin_list:
        plugins_line = f'    list(APPEND APP_ARGS PLUGINS {plugin_list})\n'

    realtime_line = ''
    if args.realtime:
        realtime_line = '    list(APPEND APP_ARGS REALTIME)\n'

    event_line = ''
    if args.event_driven:
        event_line = '    list(APPEND APP_ARGS EVENT_DRIVEN)\n'

    config_line = ''
    if args.config:
        config_line = '    list(APPEND APP_ARGS CONFIG)\n'

    cmake_script = f'''# Include application generator
include("{cmake_dir_posix}/MCFApplicationGenerator.cmake")

# Generate application
set(APP_ARGS NAME "{args.name}" VERSION "{args.version}" AUTHOR "{args.author}" DESCRIPTION "{args.description}" OUTPUT_DIR "{output_dir_posix}")

{modules_line}
{plugins_line}
{realtime_line}
{event_line}
{config_line}
mcf_generate_application(${{APP_ARGS}})
'''

    temp_cmake.write_text(cmake_script)

    # Run CMake to generate application
    print("Generating application...")
    try:
        result = subprocess.run(['cmake', '-P', str(temp_cmake)],
                                cwd=str(project_root),
                                check=True,
                                capture_output=False)

        # Cleanup
        temp_cmake.unlink()

        print()
        print("[SUCCESS] Application generated successfully!")
        print()
        print("Next steps:")
        print(f"  1. Navigate to the application directory:")
        print(f"     cd {output_dir}")
        print()
        print(f"  2. Create build directory and configure:")
        print(f"     mkdir build && cd build")
        print(f"     cmake ..")
        print()
        print(f"  3. Build the application:")
        if platform.system() == 'Windows':
            print(f"     cmake --build .")
        else:
            print(f"     make -j$(nproc)")
        print()
        print(f"  4. Run the application:")
        app_name_lower = args.name.lower()
        if platform.system() == 'Windows':
            print(f"     bin\\{app_name_lower}.exe")
        else:
            print(f"     ./bin/{app_name_lower}")
        print()

        return 0

    except subprocess.CalledProcessError as e:
        print(f"Error: Application generation failed: {e}", file=sys.stderr)
        return 1
    except FileNotFoundError:
        print("Error: CMake not found. Please install CMake.", file=sys.stderr)
        return 1


def main():
    """Main entry point"""
    parser = create_parser()
    args = parser.parse_args()

    # Get project root
    project_root = get_project_root()

    # Run generator
    return run_cmake_generator(args, project_root)


if __name__ == '__main__':
    sys.exit(main())
