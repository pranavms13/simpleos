#!/usr/bin/env python3
"""Convert binary file to C header array"""

import sys

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.bin> <output.h>", file=sys.stderr)
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    with open(input_file, "rb") as f:
        data = f.read()
    
    with open(output_file, "w") as f:
        f.write("/* Auto-generated kernel data - do not edit */\n")
        f.write("#ifndef _KERNEL_DATA_H_\n")
        f.write("#define _KERNEL_DATA_H_\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write("static const uint8_t kernel_data[] = {\n")
        
        for i, b in enumerate(data):
            if i % 12 == 0:
                f.write("    ")
            f.write("0x%02x," % b)
            if i % 12 == 11:
                f.write("\n")
        
        f.write("\n};\n\n")
        f.write("#define KERNEL_SIZE %d\n\n" % len(data))
        f.write("#endif /* _KERNEL_DATA_H_ */\n")
    
    print(f"Converted {len(data)} bytes to {output_file}")

if __name__ == "__main__":
    main()
