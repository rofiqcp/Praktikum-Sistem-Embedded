#!/bin/bash
# Fix src/CMakeLists.txt for all ESP32 programs

for dir in ESP32_*/; do
    echo "Fixing $dir"
    
    # Fix src/CMakeLists.txt
    cat > "${dir}src/CMakeLists.txt" << 'CMAKE'
idf_component_register(SRCS "main.c" INCLUDE_DIRS ".")
CMAKE
    
    echo "  Fixed src/CMakeLists.txt"
done

echo "All CMakeLists.txt files fixed!"
