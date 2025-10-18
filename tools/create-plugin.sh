#!/bin/bash
# create-plugin.sh
# Helper script to generate a new MCF plugin using CMake

set -e

# Default values
PLUGIN_NAME=""
PLUGIN_VERSION="1.0.0"
PLUGIN_AUTHOR="MCF Developer"
PLUGIN_DESCRIPTION="Plugin for ModularCppFramework"
PLUGIN_PRIORITY=100
PLUGIN_REALTIME=0
PLUGIN_EVENT_DRIVEN=0
PLUGIN_OUTPUT_DIR=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Usage function
usage() {
    local exit_code=${1:-1}
    echo -e "${BLUE}MCF Plugin Generator${NC}"
    echo ""
    echo "Usage: $0 -n <plugin_name> [options]"
    echo ""
    echo "Required:"
    echo "  -n, --name NAME           Plugin name (e.g., AudioPlugin)"
    echo ""
    echo "Optional:"
    echo "  -v, --version VERSION     Plugin version (default: 1.0.0)"
    echo "  -a, --author AUTHOR       Plugin author (default: MCF Developer)"
    echo "  -d, --description DESC    Plugin description"
    echo "  -p, --priority PRIORITY   Load priority (default: 100)"
    echo "  -o, --output DIR          Output directory (default: plugins/)"
    echo "  -r, --realtime            Add IRealtimeUpdatable interface"
    echo "  -e, --event-driven        Add IEventDriven interface"
    echo "  -h, --help                Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 -n MyPlugin"
    echo "  $0 -n AudioPlugin -v 2.0.0 -a 'Audio Team' -r"
    echo "  $0 -n NetworkPlugin -r -e -p 200"
    echo ""
    exit $exit_code
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--name)
            PLUGIN_NAME="$2"
            shift 2
            ;;
        -v|--version)
            PLUGIN_VERSION="$2"
            shift 2
            ;;
        -a|--author)
            PLUGIN_AUTHOR="$2"
            shift 2
            ;;
        -d|--description)
            PLUGIN_DESCRIPTION="$2"
            shift 2
            ;;
        -p|--priority)
            PLUGIN_PRIORITY="$2"
            shift 2
            ;;
        -o|--output)
            PLUGIN_OUTPUT_DIR="$2"
            shift 2
            ;;
        -r|--realtime)
            PLUGIN_REALTIME=1
            shift
            ;;
        -e|--event-driven)
            PLUGIN_EVENT_DRIVEN=1
            shift
            ;;
        -h|--help)
            usage 0
            ;;
        *)
            echo -e "${RED}Error: Unknown option $1${NC}"
            usage 1
            ;;
    esac
done

# Validate required arguments
if [ -z "$PLUGIN_NAME" ]; then
    echo -e "${RED}Error: Plugin name is required${NC}"
    usage 1
fi

# Get script directory (tools/ is now at project root level)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_DIR="$PROJECT_ROOT/cmake"

# Build CMake command
CMAKE_CMD="cmake -S \"$PROJECT_ROOT\" -B \"$PROJECT_ROOT/build\" -DGENERATE_PLUGIN=ON"
CMAKE_CMD="$CMAKE_CMD -DPLUGIN_NAME=\"$PLUGIN_NAME\""
CMAKE_CMD="$CMAKE_CMD -DPLUGIN_VERSION=\"$PLUGIN_VERSION\""
CMAKE_CMD="$CMAKE_CMD -DPLUGIN_AUTHOR=\"$PLUGIN_AUTHOR\""
CMAKE_CMD="$CMAKE_CMD -DPLUGIN_DESCRIPTION=\"$PLUGIN_DESCRIPTION\""
CMAKE_CMD="$CMAKE_CMD -DPLUGIN_PRIORITY=$PLUGIN_PRIORITY"

if [ $PLUGIN_REALTIME -eq 1 ]; then
    CMAKE_CMD="$CMAKE_CMD -DPLUGIN_REALTIME=ON"
fi

if [ $PLUGIN_EVENT_DRIVEN -eq 1 ]; then
    CMAKE_CMD="$CMAKE_CMD -DPLUGIN_EVENT_DRIVEN=ON"
fi

# Display configuration
echo -e "${BLUE}=== MCF Plugin Generator ===${NC}"
echo ""
echo -e "${YELLOW}Configuration:${NC}"
echo "  Name:        $PLUGIN_NAME"
echo "  Version:     $PLUGIN_VERSION"
echo "  Author:      $PLUGIN_AUTHOR"
echo "  Description: $PLUGIN_DESCRIPTION"
echo "  Priority:    $PLUGIN_PRIORITY"
echo "  Interfaces:"
if [ $PLUGIN_REALTIME -eq 1 ]; then
    echo "    - IRealtimeUpdatable"
fi
if [ $PLUGIN_EVENT_DRIVEN -eq 1 ]; then
    echo "    - IEventDriven"
fi
if [ $PLUGIN_REALTIME -eq 0 ] && [ $PLUGIN_EVENT_DRIVEN -eq 0 ]; then
    echo "    - IPlugin only"
fi
echo ""

# Create temporary CMakeLists.txt for generation
TEMP_CMAKE="$PROJECT_ROOT/build/generate_plugin.cmake"
mkdir -p "$PROJECT_ROOT/build"

cat > "$TEMP_CMAKE" << EOF
# Include plugin generator
include("$CMAKE_DIR/MCFPluginGenerator.cmake")

# Generate plugin
set(PLUGIN_ARGS NAME "$PLUGIN_NAME" VERSION "$PLUGIN_VERSION" AUTHOR "$PLUGIN_AUTHOR" DESCRIPTION "$PLUGIN_DESCRIPTION" PRIORITY $PLUGIN_PRIORITY)

if(NOT "$PLUGIN_OUTPUT_DIR" STREQUAL "")
    list(APPEND PLUGIN_ARGS OUTPUT_DIR "$PLUGIN_OUTPUT_DIR/$PLUGIN_NAME")
endif()

if($PLUGIN_REALTIME)
    list(APPEND PLUGIN_ARGS REALTIME)
endif()

if($PLUGIN_EVENT_DRIVEN)
    list(APPEND PLUGIN_ARGS EVENT_DRIVEN)
endif()

mcf_generate_plugin(\${PLUGIN_ARGS})
EOF

# Run CMake to generate plugin
echo -e "${GREEN}Generating plugin...${NC}"
cd "$PROJECT_ROOT"
cmake -P "$TEMP_CMAKE"

# Cleanup
rm -f "$TEMP_CMAKE"

echo ""
echo -e "${GREEN}✓ Plugin generated successfully!${NC}"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "  1. Add the following line to plugins/CMakeLists.txt:"
echo -e "     ${BLUE}add_subdirectory($PLUGIN_NAME)${NC}"
echo ""
echo "  2. Implement your plugin logic in:"
echo -e "     ${BLUE}plugins/$PLUGIN_NAME/$PLUGIN_NAME.cpp${NC}"
echo ""
echo "  3. Build the project:"
echo -e "     ${BLUE}cd build && cmake .. && make -j\$(nproc)${NC}"
echo ""
