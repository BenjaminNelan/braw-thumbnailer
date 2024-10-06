#!/bin/bash

# If Resolve is installed, these libraries should already be on the system
RESOLVE_DIR="/opt/resolve/libs"
LIB_DIR="/usr/lib/blackmagic/BlackmagicRAWSDK/Linux/Libraries"
LIB64_DIR="/usr/lib64/blackmagic/BlackmagicRAWSDK/Linux/Libraries"

# List of required SDK libraries, I'm not actually sure if you need all these
FILES=(
    "libBlackmagicRawAPI.so"
    "libDecoderOpenCL.so"
    "libc++abi.so.1"
    "libInstructionSetServicesAVX2.so"
    "libc++.so.1"
    "libInstructionSetServicesAVX.so"
    "libDecoderCUDA.so"
)

# Check if either SDK library directory exists
if [ -d "$LIB_DIR" ]; then
    echo "$LIB_DIR already exists. No action needed."
    DEST_DIR="$LIB_DIR"
elif [ -d "$LIB64_DIR" ]; then
    echo "$LIB64_DIR already exists. No action needed."
    DEST_DIR="$LIB64_DIR"
else
    echo "Missing:"
    echo "- $LIB_DIR"
    echo "- $LIB64_DIR"
    echo "Creating $LIB_DIR..."
    sudo mkdir -p "$LIB_DIR"
    DEST_DIR="$LIB_DIR"
fi

# Copy files only if we created a new directory
if [ "$DEST_DIR" = "$LIB_DIR" ] && [ ! -f "$DEST_DIR/${FILES[0]}" ]; then
    echo "Copying required libraries to $DEST_DIR..."
    for file in "${FILES[@]}"; do
        if [ -f "$RESOLVE_DIR/$file" ]; then
            sudo cp "$RESOLVE_DIR/$file" "$DEST_DIR/"
            echo "Copied $file to $DEST_DIR"
        else
            echo "Warning: $file not found in $RESOLVE_DIR. You may need to download the SDK."
        fi
    done
fi

# Original installation steps
sudo cp braw-thumbnailer /usr/bin/
sudo cp braw-thumbnailer.thumbnailer /usr/share/thumbnailers/
echo "Copied files to /usr/bin/ and /usr/share/thumbnailers/"

echo "Installation completed."