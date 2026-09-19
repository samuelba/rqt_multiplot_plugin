
#!/bin/bash

# Loop through all .svg files in the current directory
for file in *.svg; do
    # Check if the file exists (prevents errors if no SVGs are found)
    if [[ -f "$file" ]]; then
        # Replace the existing viewBox attribute inline
        sed -i 's/viewBox="[^"]*"/viewBox="2 2 28 28"/g' "$file"
        echo "Updated: $file"
    fi
done

echo "Batch processing complete."