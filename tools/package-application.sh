#!/bin/bash

# package-application.sh - Application Packaging Tool for ModularCppFramework
# This script helps package MCF applications into distributable archives

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BUILD_DIR="build"
BUILD_TYPE="Release"
PACKAGE_TARGET=""
CLEAN_BUILD=false
SHOW_HELP=false
VERBOSE=false
OUTPUT_DIR=""
EXTRACT_PACKAGE=false
TEST_PACKAGE=false

# Print colored message (to stderr to avoid interfering with return values)
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1" >&2
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" >&2
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" >&2
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

# Print usage information
print_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Package MCF applications into distributable archives.

OPTIONS:
    -t, --target TARGET       Package target name (required)
                              Examples: package-my_app, package-mcf-examples

    -b, --build-dir DIR       Build directory (default: build)

    -c, --config CONFIG       Build configuration type (default: Release)
                              Options: Debug, Release, RelWithDebInfo, MinSizeRel

    -o, --output DIR          Copy package to output directory after build

    -j, --jobs N              Number of parallel build jobs (default: auto-detect)

    --clean                   Clean build directory before building

    --extract                 Extract package after building for verification

    --test                    Run basic tests on the extracted package

    -v, --verbose             Enable verbose output

    -h, --help                Show this help message

EXAMPLES:
    # Package a single application
    $0 -t package-my_app

    # Package with clean build
    $0 -t package-my_app --clean

    # Package all MCF examples
    $0 -t package-mcf-examples -c Release

    # Package, extract, and test
    $0 -t package-my_app --extract --test

    # Package and copy to distribution directory
    $0 -t package-my_app -o /tmp/dist

PACKAGE TARGETS:
    To see available package targets, run:
        cmake --build $BUILD_DIR --target help | grep package-

WORKFLOW:
    1. Configure CMake (if needed)
    2. Build the application and dependencies
    3. Run the package target
    4. Optionally extract and test the package
    5. Optionally copy to output directory

SEE ALSO:
    - docs/sdk/APPLICATION_PACKAGING.md - Complete packaging guide
    - cmake/MCFPackaging.cmake - Packaging functions reference

EOF
}

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -t|--target)
                PACKAGE_TARGET="$2"
                shift 2
                ;;
            -b|--build-dir)
                BUILD_DIR="$2"
                shift 2
                ;;
            -c|--config)
                BUILD_TYPE="$2"
                shift 2
                ;;
            -o|--output)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            -j|--jobs)
                BUILD_JOBS="$2"
                shift 2
                ;;
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            --extract)
                EXTRACT_PACKAGE=true
                shift
                ;;
            --test)
                TEST_PACKAGE=true
                EXTRACT_PACKAGE=true  # Must extract to test
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -h|--help)
                SHOW_HELP=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                print_usage
                exit 1
                ;;
        esac
    done
}

# Validate arguments
validate_arguments() {
    if [[ "$SHOW_HELP" == true ]]; then
        print_usage
        exit 0
    fi

    if [[ -z "$PACKAGE_TARGET" ]]; then
        print_error "Package target is required. Use -t or --target option."
        print_usage
        exit 1
    fi

    # Ensure target starts with "package-"
    if [[ ! "$PACKAGE_TARGET" =~ ^package- ]]; then
        print_warning "Target name should start with 'package-'. Prepending it automatically."
        PACKAGE_TARGET="package-$PACKAGE_TARGET"
    fi

    # Validate build type
    case "$BUILD_TYPE" in
        Debug|Release|RelWithDebInfo|MinSizeRel)
            ;;
        *)
            print_error "Invalid build type: $BUILD_TYPE"
            print_error "Valid options: Debug, Release, RelWithDebInfo, MinSizeRel"
            exit 1
            ;;
    esac

    # Auto-detect number of CPU cores if not specified
    if [[ -z "$BUILD_JOBS" ]]; then
        if command -v nproc &> /dev/null; then
            BUILD_JOBS=$(nproc)
        else
            BUILD_JOBS=4
        fi
    fi
}

# Clean build directory
clean_build_directory() {
    if [[ "$CLEAN_BUILD" == true ]]; then
        print_info "Cleaning build directory: $BUILD_DIR"
        if [[ -d "$BUILD_DIR" ]]; then
            rm -rf "$BUILD_DIR"
            print_success "Build directory cleaned"
        else
            print_warning "Build directory does not exist, skipping clean"
        fi
    fi
}

# Configure CMake
configure_cmake() {
    print_info "Configuring CMake..."

    if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]] || [[ "$CLEAN_BUILD" == true ]]; then
        mkdir -p "$BUILD_DIR"

        local CMAKE_CMD="cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=$BUILD_TYPE"

        if [[ "$VERBOSE" == true ]]; then
            CMAKE_CMD="$CMAKE_CMD -DCMAKE_VERBOSE_MAKEFILE=ON"
        fi

        print_info "Running: $CMAKE_CMD"
        eval $CMAKE_CMD

        print_success "CMake configuration complete"
    else
        print_info "Using existing CMake configuration"
    fi
}

# Build the project
build_project() {
    print_info "Building project dependencies..."

    local BUILD_CMD="cmake --build $BUILD_DIR --config $BUILD_TYPE -j $BUILD_JOBS"

    if [[ "$VERBOSE" == true ]]; then
        BUILD_CMD="$BUILD_CMD --verbose"
    fi

    print_info "Running: $BUILD_CMD"
    eval $BUILD_CMD

    print_success "Build complete"
}

# Create package
create_package() {
    print_info "Creating package: $PACKAGE_TARGET"

    local PACKAGE_CMD="cmake --build $BUILD_DIR --target $PACKAGE_TARGET"

    if [[ "$VERBOSE" == true ]]; then
        PACKAGE_CMD="$PACKAGE_CMD --verbose"
    fi

    # Execute package command
    if [[ "$VERBOSE" == true ]]; then
        eval $PACKAGE_CMD
    else
        eval $PACKAGE_CMD > /dev/null 2>&1
    fi

    if [[ $? -ne 0 ]]; then
        print_error "Package creation failed"
        return 1
    fi

    # Find the generated package
    local PACKAGE_FILE=$(find "$BUILD_DIR" -maxdepth 1 -type f \( -name "*.tar.gz" -o -name "*.zip" \) | head -n 1)

    if [[ -z "$PACKAGE_FILE" ]]; then
        print_error "Package file not found in $BUILD_DIR"
        return 1
    fi

    print_success "Package created: $(basename "$PACKAGE_FILE")"
    echo "$PACKAGE_FILE"
}

# Extract package for testing
extract_package() {
    local PACKAGE_FILE="$1"

    if [[ "$EXTRACT_PACKAGE" == true ]]; then
        print_info "Extracting package for verification..."

        local EXTRACT_DIR="/tmp/mcf-package-test-$$"
        mkdir -p "$EXTRACT_DIR"

        if [[ "$PACKAGE_FILE" == *.tar.gz ]]; then
            tar -xzf "$PACKAGE_FILE" -C "$EXTRACT_DIR"
        elif [[ "$PACKAGE_FILE" == *.zip ]]; then
            unzip -q "$PACKAGE_FILE" -d "$EXTRACT_DIR"
        else
            print_error "Unknown package format: $PACKAGE_FILE"
            return 1
        fi

        print_success "Package extracted to: $EXTRACT_DIR"

        # List contents
        print_info "Package contents:"
        if command -v tree &> /dev/null; then
            tree "$EXTRACT_DIR" -L 3 >&2
        else
            ls -lR "$EXTRACT_DIR" >&2
        fi

        echo "$EXTRACT_DIR"
    fi
}

# Test package
test_package() {
    local EXTRACT_DIR="$1"

    if [[ "$TEST_PACKAGE" == true ]] && [[ -n "$EXTRACT_DIR" ]]; then
        print_info "Running basic package tests..."

        # Find the actual package directory (first subdirectory)
        local PACKAGE_DIR=$(find "$EXTRACT_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)

        if [[ -z "$PACKAGE_DIR" ]]; then
            print_error "Could not find package directory in $EXTRACT_DIR"
            return 1
        fi

        print_info "Package directory: $PACKAGE_DIR"

        # Check directory structure
        print_info "Checking directory structure..."

        local ERRORS=0

        if [[ ! -d "$PACKAGE_DIR/bin" ]]; then
            print_warning "Missing bin/ directory"
            ERRORS=$((ERRORS + 1))
        else
            print_success "bin/ directory found"
            local BIN_COUNT=$(find "$PACKAGE_DIR/bin" -type f -executable 2>/dev/null | wc -l)
            print_info "  Executables: $BIN_COUNT"
        fi

        if [[ -d "$PACKAGE_DIR/plugins" ]]; then
            print_success "plugins/ directory found"
            local PLUGIN_COUNT=$(find "$PACKAGE_DIR/plugins" -type f \( -name "*.so" -o -name "*.dll" -o -name "*.dylib" \) 2>/dev/null | wc -l)
            print_info "  Plugins: $PLUGIN_COUNT"
        fi

        if [[ -d "$PACKAGE_DIR/config" ]]; then
            print_success "config/ directory found"
            local CONFIG_COUNT=$(find "$PACKAGE_DIR/config" -type f 2>/dev/null | wc -l)
            print_info "  Config files: $CONFIG_COUNT"
        fi

        if [[ -f "$PACKAGE_DIR/README.txt" ]]; then
            print_success "README.txt found"
            print_info "README contents:"
            echo "----------------------------------------"
            head -n 20 "$PACKAGE_DIR/README.txt"
            echo "----------------------------------------"
        else
            print_warning "Missing README.txt"
        fi

        # Try to get executable info
        print_info "Checking executables..."
        for exe in "$PACKAGE_DIR"/bin/*; do
            if [[ -f "$exe" ]] && [[ -x "$exe" ]]; then
                print_info "  $(basename "$exe")"

                # Check dependencies (Linux only)
                if command -v ldd &> /dev/null; then
                    if [[ "$VERBOSE" == true ]]; then
                        echo "    Dependencies:"
                        ldd "$exe" | head -n 10 | sed 's/^/      /'
                    fi
                fi
            fi
        done

        if [[ $ERRORS -eq 0 ]]; then
            print_success "All package tests passed"
        else
            print_warning "Package tests completed with $ERRORS warnings"
        fi

        print_info "Extracted package location: $PACKAGE_DIR"
        print_info "You can manually test by running: cd $PACKAGE_DIR && ./bin/<executable>"
    fi
}

# Copy package to output directory
copy_to_output() {
    local PACKAGE_FILE="$1"

    if [[ -n "$OUTPUT_DIR" ]]; then
        print_info "Copying package to output directory..."

        mkdir -p "$OUTPUT_DIR"
        cp "$PACKAGE_FILE" "$OUTPUT_DIR/"

        local OUTPUT_FILE="$OUTPUT_DIR/$(basename "$PACKAGE_FILE")"
        print_success "Package copied to: $OUTPUT_FILE"
    fi
}

# Main function
main() {
    parse_arguments "$@"
    validate_arguments

    print_info "MCF Application Packaging Tool"
    print_info "==============================="
    print_info "Package Target: $PACKAGE_TARGET"
    print_info "Build Directory: $BUILD_DIR"
    print_info "Build Type: $BUILD_TYPE"
    print_info "Build Jobs: $BUILD_JOBS"
    echo ""

    # Execute packaging workflow
    clean_build_directory
    configure_cmake
    build_project

    local PACKAGE_FILE=$(create_package)
    if [[ -z "$PACKAGE_FILE" ]]; then
        print_error "Packaging failed"
        exit 1
    fi

    local EXTRACT_DIR=$(extract_package "$PACKAGE_FILE")
    test_package "$EXTRACT_DIR"
    copy_to_output "$PACKAGE_FILE"

    echo ""
    print_success "Packaging complete!"
    print_info "Package location: $PACKAGE_FILE"

    if [[ -n "$OUTPUT_DIR" ]]; then
        print_info "Output location: $OUTPUT_DIR/$(basename "$PACKAGE_FILE")"
    fi

    if [[ -n "$EXTRACT_DIR" ]]; then
        print_info "Extracted for testing: $EXTRACT_DIR"
    fi
}

# Run main function
main "$@"
