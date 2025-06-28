# Embedding background.png as C++ Header

To embed `background.png` as a static C++ array for use in your application:

## Steps

1. Place your PNG file in this folder as `background.png`.
2. Run the following command in your terminal:

   ```sh
   xxd -i background.png > background_png.h
   ```

   - This will generate a C header file `background_png.h` containing the PNG data as a C array and its length.
   - The array will be named `resources_background_png` and the length as `resources_background_png_len`.

3. Include `background_png.h` in your C++ project:

   ```cpp
   #include "resources/background_png.h"
   ```

4. Use the array and length in your code, for example with stb_image:

   ```cpp
   stbi_load_from_memory(resources_background_png, resources_background_png_len, ...);
   ```

## Notes
- If you change `background.png`, re-run the above command to update the header.
- The generated header can be committed to version control for reproducible builds.

