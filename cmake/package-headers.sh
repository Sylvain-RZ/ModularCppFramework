#!/bin/bash
# Script to copy only header files, excluding build artifacts

SRC_DIR=$1
DEST_DIR=$2

# Create destination
mkdir -p "$DEST_DIR"

# Copy only .hpp files from core
find "$SRC_DIR/core" -name "*.hpp" -type f | while read file; do
    rel_path="${file#$SRC_DIR/}"
    dest_file="$DEST_DIR/$rel_path"
    mkdir -p "$(dirname "$dest_file")"
    cp "$file" "$dest_file"
done

# Copy only .hpp files from modules
find "$SRC_DIR/modules" -name "*.hpp" -type f | while read file; do
    rel_path="${file#$SRC_DIR/}"
    dest_file="$DEST_DIR/$rel_path"
    mkdir -p "$(dirname "$dest_file")"
    cp "$file" "$dest_file"
done

echo "Headers copied successfully"
