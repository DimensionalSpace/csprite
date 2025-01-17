#pragma once

/*
	Macro: COLOR_EQUAL(unsigned char* a, unsigned char* b)
	Description: Checks if 2 Colors RGBA are Equal
*/
#define COLOR_EQUAL(a, b) (a != NULL && b != NULL && *(a + 0) == *(b + 0) && *(a + 1) == *(b + 1) && *(a + 2) == *(b + 2) && *(a + 3) == *(b + 3))

#define RGBA2UINT32(r, g, b, a) \
	((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) + (a & 0xff)

#ifdef __cplusplus // C++ Only Macros

// ImGui::ColorConvertU32ToFloat4 but in RGBA format
#define _U32TOIV4(in)                          \
	ImVec4(                                    \
		((in >> 24) & 0xFF) * (1.0f / 255.0f), \
		((in >> 16) & 0xFF) * (1.0f / 255.0f), \
		((in >> 8) & 0xFF)  * (1.0f / 255.0f), \
		((in >> 0) & 0xFF)  * (1.0f / 255.0f)  \
	)
#endif

