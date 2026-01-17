#!/bin/bash
# SimpleOS QEMU Run Script
# Builds and runs the UEFI application in QEMU

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}SimpleOS UEFI Bootloader${NC}"
echo "========================="
echo ""

# Check dependencies
check_dep() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${RED}Error: $1 not found${NC}"
        return 1
    fi
    echo -e "  ✓ $1"
    return 0
}

echo "Checking dependencies..."
MISSING=0
check_dep x86_64-w64-mingw32-gcc || MISSING=1
check_dep x86_64-elf-gcc || MISSING=1
check_dep qemu-system-x86_64 || MISSING=1

if [ $MISSING -eq 1 ]; then
    echo ""
    echo -e "${YELLOW}Install missing dependencies:${NC}"
    echo "  brew install mingw-w64 x86_64-elf-gcc qemu"
    exit 1
fi

echo ""
echo "Building..."
make clean
make

echo ""
echo -e "${GREEN}Starting QEMU...${NC}"
echo "(Press Ctrl+A, then X to exit)"
echo ""
make run
