#!/bin/bash
# Generate Doxygen documentation for gr-packet-protocols

set -e

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCS_DIR="$PROJECT_ROOT/docs"
DOXYFILE="$DOCS_DIR/Doxyfile"
OUTPUT_DIR="$DOCS_DIR/doxygen"

echo "Generating Doxygen documentation for gr-packet-protocols..."
echo "Project root: $PROJECT_ROOT"
echo "Docs directory: $DOCS_DIR"

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: Doxygen is not installed."
    echo "Please install Doxygen:"
    echo "  Ubuntu/Debian: sudo apt install doxygen"
    echo "  CentOS/RHEL: sudo yum install doxygen"
    echo "  macOS: brew install doxygen"
    exit 1
fi

# Check if Graphviz is installed (for diagrams)
if ! command -v dot &> /dev/null; then
    echo "Warning: Graphviz is not installed."
    echo "Diagrams will not be generated."
    echo "Install with: sudo apt install graphviz"
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check if Doxyfile exists
if [ ! -f "$DOXYFILE" ]; then
    echo "Error: Doxyfile not found at $DOXYFILE"
    exit 1
fi

# Generate documentation
echo "Running Doxygen..."
cd "$DOCS_DIR"
doxygen Doxyfile

# Check if generation was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "Documentation generated successfully!"
    echo "Output directory: $OUTPUT_DIR"
    echo ""
    echo "Generated files:"
    ls -la "$OUTPUT_DIR"
    echo ""
    echo "To view the documentation:"
    echo "  Open: $OUTPUT_DIR/html/index.html"
    echo ""
    echo "Or serve locally:"
    echo "  cd $OUTPUT_DIR/html && python3 -m http.server 8000"
    echo "  Then open: http://localhost:8000"
else
    echo "Error: Doxygen generation failed"
    exit 1
fi

# Generate additional documentation formats if requested
if [ "$1" = "--latex" ]; then
    echo "Generating LaTeX documentation..."
    cd "$OUTPUT_DIR/latex"
    make
    echo "LaTeX documentation generated in: $OUTPUT_DIR/latex"
fi

if [ "$1" = "--pdf" ]; then
    echo "Generating PDF documentation..."
    cd "$OUTPUT_DIR/latex"
    make
    if command -v pdflatex &> /dev/null; then
        pdflatex refman.tex
        pdflatex refman.tex  # Run twice for proper references
        echo "PDF documentation generated: $OUTPUT_DIR/latex/refman.pdf"
    else
        echo "Warning: pdflatex not found. PDF generation skipped."
    fi
fi

echo ""
echo "Documentation generation complete!"
echo "For more information, see: $DOCS_DIR/README.md"
