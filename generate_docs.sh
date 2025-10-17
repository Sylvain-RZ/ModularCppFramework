#!/bin/bash
# Generate Doxygen documentation for ModularCppFramework

set -e

echo "=========================================="
echo "  ModularCppFramework Documentation"
echo "=========================================="
echo ""

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "ERROR: Doxygen is not installed."
    echo "Please install it with:"
    echo "  sudo apt-get install doxygen        # Ubuntu/Debian"
    echo "  sudo dnf install doxygen            # Fedora"
    echo "  brew install doxygen                # macOS"
    exit 1
fi

echo "✓ Doxygen found: $(doxygen --version)"
echo ""

# Create output directory if it doesn't exist
mkdir -p docs/doxygen

echo "Generating documentation..."
doxygen Doxyfile

if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "  Documentation generated successfully!"
    echo "=========================================="
    echo ""
    echo "📖 Open the documentation:"
    echo "   HTML: docs/doxygen/html/index.html"
    echo ""
    echo "To view in browser:"
    echo "   firefox docs/doxygen/html/index.html"
    echo "   google-chrome docs/doxygen/html/index.html"
    echo "   xdg-open docs/doxygen/html/index.html"
    echo ""
else
    echo ""
    echo "ERROR: Documentation generation failed!"
    exit 1
fi
