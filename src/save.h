#ifndef SAVE_H
#define SAVE_H

#include "macros.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// Loads a image to canvas and automatically calls FreeHistory to reset undo/redo
void LoadImageToCanvas(const char *filepath, int *canvas_dims, Uint32 **canvas_data) {
	int imgWidth, imgHeight, c;
	unsigned char* image_data = stbi_load(filepath, &imgWidth, &imgHeight, &c, 0);
	if (image_data == NULL) {
		printf("Unable to load image %s\n", filepath);
		return;
	}

	canvas_dims[0] = imgWidth;
	canvas_dims[1] = imgHeight;

	if (*canvas_data != NULL)
		free(*canvas_data);

	*canvas_data = (Uint32*)malloc(canvas_dims[0] * canvas_dims[1] * 4 * sizeof(Uint32));
	memset(*canvas_data, 0, canvas_dims[0] * canvas_dims[1] * 4 * sizeof(Uint32));

	int y, x;
	Uint32* ptr;
	unsigned char* iptr;
	for (y = 0; y < imgHeight; y++) {
		for (x = 0; x < imgWidth; x++) {
			ptr = GetPixel(x, y, NULL); // XX - Check For NULL
			iptr = image_data + ((y * canvas_dims[0] + x) * 4); // Gets Pixel At x, y in image_data

			*(ptr+0) = RGBA2UINT32(*(iptr+0), *(iptr+1), *(iptr+2), *(iptr+3));
		}
	}
	stbi_image_free(image_data);
	// FreeHistory();
}

// void WritePngFromCanvas(const char *filepath, int *canvas_dims) {
// 	unsigned char *data = (unsigned char *) malloc(canvas_dims[0] * canvas_dims[1] * 4 * sizeof(unsigned char));
// 	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
// 	stbi_write_png(filepath, canvas_dims[0], canvas_dims[1], 4, data, 0);
// 	free(data);
// }

// void WriteJpgFromCanvas(const char *filepath, int *canvas_dims) {
// 	unsigned char *data = (unsigned char *) malloc(canvas_dims[0] * canvas_dims[1] * 4 * sizeof(unsigned char));
// 	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
// 	stbi_write_jpg(filepath, canvas_dims[0], canvas_dims[1], 4, data, 100);
// 	free(data);
// }

#endif // end SAVE_H
