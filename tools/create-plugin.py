#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
create-plugin.py - Cross-platform plugin generator for ModularCppFramework
Python wrapper that works on Windows, Linux, and macOS
"""

import sys
import os
import argparse
import subprocess
import platform
from pathlib import Path

# Set UTF-8 encoding for Windows console
if platform.system() == 'Windows':
    import codecs
    if sys.stdout.encoding != 'utf-8':
        sys.stdout.reconfigure(encoding='utf-8')
    if sys.stderr.encoding != 'utf-8':
        sys.stderr.reconfigure(encoding='utf-8')


def get_project_root():
    """Get the project root directory"""
    script_dir = Path(__file__).parent.absolute()
    return script_dir.parent


def create_parser():
    """Create argument parser"""
    parser = argparse.ArgumentParser(
        description='MCF Plugin Generator - Create new plugins',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s -n MyPlugin
  %(prog)s -n AudioPlugin -v 2.0.0 -a "Audio Team" -r
  %(prog)s -n NetworkPlugin -r -e -p 200
        """)

    parser.add_argument('-n', '--name', required=True,
                        help='Plugin name (required, e.g., AudioPlugin)')
    parser.add_argument('-v', '--version', default='1.0.0',
                        help='Plugin version (default: 1.0.0)')
    parser.add_argument('-a', '--author', default='MCF Developer',
                        help='Plugin author (default: MCF Developer)')
    parser.add_argument('-d', '--description', default='Plugin for ModularCppFramework',
                        help='Plugin description')
    parser.add_argument('-p', '--priority', type=int, default=100,
                        help='Load priority (default: 100)')
    parser.add_argument('-o', '--output', default='',
                        help='Output directory (default: plugins/)')
    parser.add_argument('-r', '--realtime', action='store_true',
                        help='Add IRealtimeUpdatable interface')
    parser.add_argument('-e', '--event-driven', action='store_true',
                        help='Add IEventDriven interface')

    return parser


def run_cmake_generator(args, project_root):
    """Run CMake plugin generator"""

    # Build CMake command
    cmake_cmd = [
        'cmake',
        '-S', str(project_root),
        '-B', str(project_root / 'build'),
        '-DGENERATE_PLUGIN=ON',
        f'-DPLUGIN_NAME={args.name}',
        f'-DPLUGIN_VERSION={args.version}',
        f'-DPLUGIN_AUTHOR={args.author}',
        f'-DPLUGIN_DESCRIPTION={args.description}',
        f'-DPLUGIN_PRIORITY={args.priority}'
    ]

    if args.realtime:
        cmake_cmd.append('-DPLUGIN_REALTIME=ON')

    if args.event_driven:
        cmake_cmd.append('-DPLUGIN_EVENT_DRIVEN=ON')

    # Display configuration
    print("=== MCF Plugin Generator ===")
    print()
    print("Configuration:")
    print(f"  Name:        {args.name}")
    print(f"  Version:     {args.version}")
    print(f"  Author:      {args.author}")
    print(f"  Description: {args.description}")
    print(f"  Priority:    {args.priority}")
    print("  Interfaces:")
    if args.realtime:
        print("    - IRealtimeUpdatable")
    if args.event_driven:
        print("    - IEventDriven")
    if not args.realtime and not args.event_driven:
        print("    - IPlugin only")
    print()

    # Create temporary CMake script
    build_dir = project_root / 'build'
    build_dir.mkdir(exist_ok=True)

    temp_cmake = build_dir / 'generate_plugin.cmake'
    cmake_dir = project_root / 'cmake'

    # Convert paths to POSIX format for CMake (forward slashes work on all platforms)
    cmake_dir_posix = cmake_dir.as_posix()

    output_dir_line = ''
    if args.output:
        # Convert output path to POSIX format for CMake
        output_path = Path(args.output)
        output_path_posix = output_path.as_posix()
        output_dir_line = f'    list(APPEND PLUGIN_ARGS OUTPUT_DIR "{output_path_posix}/{args.name}")\n'

    realtime_line = ''
    if args.realtime:
        realtime_line = '    list(APPEND PLUGIN_ARGS REALTIME)\n'

    event_line = ''
    if args.event_driven:
        event_line = '    list(APPEND PLUGIN_ARGS EVENT_DRIVEN)\n'

    cmake_script = f'''# Include plugin generator
include("{cmake_dir_posix}/MCFPluginGenerator.cmake")

# Generate plugin
set(PLUGIN_ARGS NAME "{args.name}" VERSION "{args.version}" AUTHOR "{args.author}" DESCRIPTION "{args.description}" PRIORITY {args.priority})

{output_dir_line}
{realtime_line}
{event_line}
mcf_generate_plugin(${{PLUGIN_ARGS}})
'''

    temp_cmake.write_text(cmake_script)

    # Run CMake to generate plugin
    print("Generating plugin...")
    try:
        result = subprocess.run(['cmake', '-P', str(temp_cmake)],
                                cwd=str(project_root),
                                check=True,
                                capture_output=False)

        # Cleanup
        temp_cmake.unlink()

        print()
        print("✓ Plugin generated successfully!")
        print()
        print("Next steps:")
        print(f"  1. Add the following line to plugins/CMakeLists.txt:")
        print(f"     add_subdirectory({args.name})")
        print()
        print(f"  2. Implement your plugin logic in:")
        print(f"     plugins/{args.name}/{args.name}.cpp")
        print()
        print(f"  3. Build the project:")
        if platform.system() == 'Windows':
            print(f"     cd build && cmake .. && cmake --build .")
        else:
            print(f"     cd build && cmake .. && make -j$(nproc)")
        print()

        return 0

    except subprocess.CalledProcessError as e:
        print(f"Error: Plugin generation failed: {e}", file=sys.stderr)
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
