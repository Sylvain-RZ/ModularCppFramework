#!/bin/bash
# create-application.sh
# Helper script to generate a new MCF application using CMake

set -e

# Default values
APP_NAME=""
APP_VERSION="1.0.0"
APP_AUTHOR="MCF Developer"
APP_DESCRIPTION="Application built with ModularCppFramework"
APP_MODULES=""
APP_PLUGINS=""
APP_REALTIME=0
APP_EVENT_DRIVEN=0
APP_CONFIG=0
APP_OUTPUT_DIR=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Usage function
usage() {
    local exit_code=${1:-1}
    echo -e "${BLUE}MCF Application Generator${NC}"
    echo ""
    echo "Usage: $0 -n <app_name> [options]"
    echo ""
    echo "Required:"
    echo "  -n, --name NAME           Application name (e.g., MyGame)"
    echo ""
    echo "Optional:"
    echo "  -v, --version VERSION     Application version (default: 1.0.0)"
    echo "  -a, --author AUTHOR       Application author (default: MCF Developer)"
    echo "  -d, --description DESC    Application description"
    echo "  -m, --modules MODULES     Comma-separated modules (logger,networking,profiling,realtime)"
    echo "  -p, --plugins PLUGINS     Comma-separated plugins to load"
    echo "  -r, --realtime            Add realtime update loop"
    echo "  -e, --event-driven        Add event-driven architecture"
    echo "  -c, --config              Generate config.json file"
    echo "  -o, --output DIR          Output directory (default: ./<app_name>)"
    echo "  -h, --help                Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 -n MyGame"
    echo "  $0 -n MyGame -r -c -m logger,profiling"
    echo "  $0 -n NetworkApp -v 2.0.0 -a 'Net Team' -m networking -e -c"
    echo ""
    exit $exit_code
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--name)
            APP_NAME="$2"
            shift 2
            ;;
        -v|--version)
            APP_VERSION="$2"
            shift 2
            ;;
        -a|--author)
            APP_AUTHOR="$2"
            shift 2
            ;;
        -d|--description)
            APP_DESCRIPTION="$2"
            shift 2
            ;;
        -m|--modules)
            APP_MODULES="$2"
            shift 2
            ;;
        -p|--plugins)
            APP_PLUGINS="$2"
            shift 2
            ;;
        -r|--realtime)
            APP_REALTIME=1
            shift
            ;;
        -e|--event-driven)
            APP_EVENT_DRIVEN=1
            shift
            ;;
        -c|--config)
            APP_CONFIG=1
            shift
            ;;
        -o|--output)
            APP_OUTPUT_DIR="$2"
            shift 2
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
if [ -z "$APP_NAME" ]; then
    echo -e "${RED}Error: Application name is required${NC}"
    usage 1
fi

# Set default output directory
if [ -z "$APP_OUTPUT_DIR" ]; then
    APP_OUTPUT_DIR="$APP_NAME"
fi

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_DIR="$PROJECT_ROOT/cmake"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Display configuration
echo -e "${BLUE}=== MCF Application Generator ===${NC}"
echo ""
echo -e "${YELLOW}Configuration:${NC}"
echo "  Name:        $APP_NAME"
echo "  Version:     $APP_VERSION"
echo "  Author:      $APP_AUTHOR"
echo "  Description: $APP_DESCRIPTION"
if [ -n "$APP_MODULES" ]; then
    echo "  Modules:     $APP_MODULES"
fi
if [ -n "$APP_PLUGINS" ]; then
    echo "  Plugins:     $APP_PLUGINS"
fi
echo "  Features:"
if [ $APP_REALTIME -eq 1 ]; then
    echo "    - Realtime update loop"
fi
if [ $APP_EVENT_DRIVEN -eq 1 ]; then
    echo "    - Event-driven architecture"
fi
if [ $APP_CONFIG -eq 1 ]; then
    echo "    - Configuration file (config.json)"
fi
if [ $APP_REALTIME -eq 0 ] && [ $APP_EVENT_DRIVEN -eq 0 ] && [ $APP_CONFIG -eq 0 ]; then
    echo "    - Basic application"
fi
echo "  Output:      $APP_OUTPUT_DIR"
echo ""

# Create temporary CMake script
TEMP_CMAKE="$PROJECT_ROOT/build/generate_application.cmake"
mkdir -p "$PROJECT_ROOT/build"

# Convert comma-separated modules to CMake list
MODULE_LIST=""
if [ -n "$APP_MODULES" ]; then
    IFS=',' read -ra MODULES_ARRAY <<< "$APP_MODULES"
    for module in "${MODULES_ARRAY[@]}"; do
        module=$(echo "$module" | xargs) # trim whitespace
        MODULE_LIST="$MODULE_LIST $module"
    done
fi

# Convert comma-separated plugins to CMake list
PLUGIN_LIST=""
if [ -n "$APP_PLUGINS" ]; then
    IFS=',' read -ra PLUGINS_ARRAY <<< "$APP_PLUGINS"
    for plugin in "${PLUGINS_ARRAY[@]}"; do
        plugin=$(echo "$plugin" | xargs) # trim whitespace
        PLUGIN_LIST="$PLUGIN_LIST $plugin"
    done
fi

cat > "$TEMP_CMAKE" << EOF
# Include application generator
include("$CMAKE_DIR/MCFApplicationGenerator.cmake")

# Generate application
set(APP_ARGS NAME "$APP_NAME" VERSION "$APP_VERSION" AUTHOR "$APP_AUTHOR" DESCRIPTION "$APP_DESCRIPTION" OUTPUT_DIR "$APP_OUTPUT_DIR")

if("$MODULE_LIST" STREQUAL "")
    # No modules
else()
    list(APPEND APP_ARGS MODULES $MODULE_LIST)
endif()

if("$PLUGIN_LIST" STREQUAL "")
    # No plugins
else()
    list(APPEND APP_ARGS PLUGINS $PLUGIN_LIST)
endif()

if($APP_REALTIME)
    list(APPEND APP_ARGS REALTIME)
endif()

if($APP_EVENT_DRIVEN)
    list(APPEND APP_ARGS EVENT_DRIVEN)
endif()

if($APP_CONFIG)
    list(APPEND APP_ARGS CONFIG)
endif()

mcf_generate_application(\${APP_ARGS})
EOF

# Run CMake to generate application
echo -e "${GREEN}Generating application...${NC}"
cd "$PROJECT_ROOT"
cmake -P "$TEMP_CMAKE"

# Cleanup
rm -f "$TEMP_CMAKE"

echo ""
echo -e "${GREEN}✓ Application generated successfully!${NC}"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "  1. Navigate to the application directory:"
echo -e "     ${BLUE}cd $APP_OUTPUT_DIR${NC}"
echo ""
echo "  2. Create build directory and configure:"
echo -e "     ${BLUE}mkdir build && cd build${NC}"
echo -e "     ${BLUE}cmake ..${NC}"
echo ""
echo "  3. Build the application:"
echo -e "     ${BLUE}make -j\$(nproc)${NC}"
echo ""
echo "  4. Run the application:"
echo -e "     ${BLUE}./bin/$(echo $APP_NAME | tr '[:upper:]' '[:lower:]')${NC}"
echo ""
