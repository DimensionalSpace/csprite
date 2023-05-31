#define _CRT_SECURE_NO_WARNINGS

#include <climits>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

// For Converting Strings To LowerCase in FixFileExtension function
#include <algorithm>
#include <cctype>

#include "imgui/imgui.h"
#include "tinyfiledialogs.h"

#include "main.h"
#include "save.hpp"
#include "assets.h"
#include "types.hpp"
#include "helpers.hpp"
#include "app/app.hpp"
#include "tools/tools.hpp"
#include "pixel/pixel.hpp"
#include "palette/palette.hpp"
#include "renderer/canvas.hpp"

std::string FilePath = "untitled.png"; // Default Output Filename
char const * FileFilterPatterns[3] = { "*.png", "*.jpg", "*.jpeg" };
unsigned char NumOfFilterPatterns = 3;

int CanvasDims[2] = {60, 40}; // Width, Height Default Canvas Size

Pixel* CanvasData = NULL;

u16 PaletteIndex = 0;
u16 PaletteCount = 16;

Palette ColorPalette;

unsigned int ZoomLevel = 8; // Default Zoom Level
std::string ZoomText = "Zoom: " + std::to_string(ZoomLevel) + "x"; // Human Readable string decribing zoom level for UI

Canvas* canvas = nullptr;
Pixel SelectedColor; // Holds Pointer To Currently Selected Color

bool ShouldSave = false;
bool ShowNewCanvasWindow = false; // Holds Whether to show new canvas window or not.
bool CanvasFreeze = false;
bool DidUndo = false;
bool IsDirty = false;

// Mouse Position On Window
ImVec2 MousePos; // mouse position
ImVec2 MousePosLast; // mouse position last frame

ImVec2 MousePosRel; // mouse position relative to canvas
ImVec2 MousePosRelLast; // mouse position relative to canvas last frame

struct cvstate {
	Pixel* pixelData;
	cvstate* next; // Canvas State Before This Node
	cvstate* prev; // Canvas State After This Node
};

typedef struct cvstate cvstate_t; // Canvas State Type

cvstate_t* CurrentState = NULL;

int main(int argc, char **argv) {
	if (App::Init(700, 500) != 0) {
		return 1;
	}

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ColorPalette.Add(Pixel{ 0,   0,   0,   255 });
	ColorPalette.Add(Pixel{ 29,  43,  83,  255 });
	ColorPalette.Add(Pixel{ 126, 37,  83,  255 });
	ColorPalette.Add(Pixel{ 0,   135, 81,  255 });
	ColorPalette.Add(Pixel{ 171, 82,  54,  255 });
	ColorPalette.Add(Pixel{ 95,  87,  79,  255 });
	ColorPalette.Add(Pixel{ 194, 195, 199, 255 });
	ColorPalette.Add(Pixel{ 255, 241, 232, 255 });
	ColorPalette.Add(Pixel{ 255, 0,   77,  255 });
	ColorPalette.Add(Pixel{ 255, 163, 0,   255 });
	ColorPalette.Add(Pixel{ 255, 236, 39,  255 });
	ColorPalette.Add(Pixel{ 0,   228, 54,  255 });
	ColorPalette.Add(Pixel{ 41,  173, 255, 255 });
	ColorPalette.Add(Pixel{ 131, 118, 156, 255 });
	ColorPalette.Add(Pixel{ 255, 119, 168, 255 });
	ColorPalette.Add(Pixel{ 255, 204, 170, 255 });
	SelectedColor = ColorPalette[PaletteIndex];
	CanvasData = new Pixel[CanvasDims[0] * CanvasDims[1]]{ 0, 0, 0, 0 };

	canvas = new Canvas(CanvasDims[0], CanvasDims[1]);
	RectI32 dirtyArea = { 0, 0, CanvasDims[0]/2, CanvasDims[1]/2 };

	// Initial Canvas Position & Size
	canvas->viewport.x = io.DisplaySize.x / 2 - (float)CanvasDims[0] * ZoomLevel / 2;
	canvas->viewport.y = io.DisplaySize.y / 2 - (float)CanvasDims[1] * ZoomLevel / 2;
	canvas->viewport.w = CanvasDims[0] * ZoomLevel;
	canvas->viewport.h = CanvasDims[1] * ZoomLevel;

	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoBackground;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoMove;

	int NEW_DIMS[2] = {60, 40}; // Default Width, Height New Canvas if Created One

	ZoomNLevelViewport();

	ImGuiWindowFlags CanvasWindowFlags = 0;
	CanvasWindowFlags |= ImGuiWindowFlags_NoTitleBar;
	CanvasWindowFlags |= ImGuiWindowFlags_NoMove;
	CanvasWindowFlags |= ImGuiWindowFlags_NoResize;
	CanvasWindowFlags |= ImGuiWindowFlags_NoCollapse;
	CanvasWindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	CanvasWindowFlags |= ImGuiWindowFlags_NoNavInputs;
	CanvasWindowFlags |= ImGuiWindowFlags_NoTitleBar;
	CanvasWindowFlags |= ImGuiWindowFlags_NoMouseInputs;
	CanvasWindowFlags |= ImGuiWindowFlags_NoMouseInputs;
	CanvasWindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;
	CanvasWindowFlags |= ImGuiWindowFlags_NoScrollbar;
	CanvasWindowFlags |= ImGuiWindowFlags_NoNavFocus;

	ToolType LastToolType = ToolManager::GetToolType();
	ToolShape LastToolShape = ToolManager::GetToolShape();
	Pixel EmptyColor = { 0, 0, 0, 0 };

	while (!App::ShouldClose()) {
		App::NewFrame();

		// printf("x: %f, y: %f\n", io.MouseDelta.x, io.MouseDelta.y);

		if (!CanvasFreeze) {
			MousePosLast = MousePos;
			MousePosRelLast = MousePosRel;

			MousePos = ImGui::GetMousePos();
			MousePosRel.x = (MousePos[0] - canvas->viewport.x) / ZoomLevel;
			MousePosRel.y = (MousePos[1] - canvas->viewport.y) / ZoomLevel;

			if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				double x = MousePosRel.x;
				double y = MousePosRel.y;

				if (
					x >= 0 && x < CanvasDims[0] &&
					y >= 0 && y < CanvasDims[1] &&
					ToolManager::GetToolType() == ToolType::BRUSH ||
					ToolManager::GetToolType() == ToolType::ERASER
				) {
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left, false)) {
						SaveState();
					}
					if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
						if (DidUndo == true) {
							IsDirty = true;
							DidUndo = false;
						} else {
							IsDirty = false;
						}
						SaveState();
					}
				}
			} else {
				if (io.MouseWheel > 0) AdjustZoom(true);
				if (io.MouseWheel < 0) AdjustZoom(false);
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Equal, false)) {
				if (io.KeyCtrl) AdjustZoom(true);
				else if (io.KeyShift && !io.KeyCtrl)
					PaletteIndex = PaletteIndex >= PaletteCount - 1 ? 0 : PaletteIndex + 1;
				else ToolManager::SetBrushSize(ToolManager::GetBrushSize() + 1);
			} else if (ImGui::IsKeyPressed(ImGuiKey_Minus, false)) {
				if (io.KeyCtrl) AdjustZoom(false);
				else if (io.KeyShift && !io.KeyCtrl)
					PaletteIndex = PaletteIndex > 0 ? PaletteIndex - 1 : PaletteCount - 1;
				else if (ToolManager::GetBrushSize() > 2)
					ToolManager::SetBrushSize(ToolManager::GetBrushSize() - 1);
			} else if (ImGui::IsKeyPressed(ImGuiKey_B, false)) {
				ToolManager::SetToolType(ToolType::BRUSH);
				ToolManager::SetToolShape(io.KeyShift ? ToolShape::SQUARE : ToolShape::CIRCLE);
			} else if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
				ToolManager::SetToolType(ToolType::ERASER);
				ToolManager::SetToolShape(io.KeyShift ? ToolShape::SQUARE : ToolShape::CIRCLE);
			} else if (ImGui::IsKeyPressed(ImGuiKey_I, false)) {
				LastToolType = ToolManager::GetToolType();
				LastToolShape = ToolManager::GetToolShape();
				ToolManager::SetToolType(ToolType::INK_DROPPER);
			} else if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
				LastToolType = ToolManager::GetToolType();
				LastToolShape = ToolManager::GetToolShape();
				ToolManager::SetToolType(ToolType::PAN);
			} else if (ImGui::IsKeyReleased(ImGuiKey_Space)) {
				ToolManager::SetToolType(LastToolType);
				ToolManager::SetToolShape(LastToolShape);
			} else if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
				if (io.KeyCtrl) Undo();
			} else if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
				if (io.KeyCtrl) Redo();
			} else if (ImGui::IsKeyPressed(ImGuiKey_N, false)) {
				if (io.KeyCtrl) ShowNewCanvasWindow = 1;
			} else if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
				if (ImGui::IsKeyPressed(ImGuiMod_Alt, false)) { // Show Prompt To Save if Alt + S pressed
					char *filePath = tinyfd_saveFileDialog("Save A File", NULL, NumOfFilterPatterns, FileFilterPatterns, "Image File (.png, .jpg, .jpeg)");
					if (filePath != NULL) {
						FilePath = FixFileExtension(std::string(filePath));
						SaveImageFromCanvas(FilePath);

						// Simple Hack To Get The File Name from the path and set it to the window title
						App::SetTitle(("CSprite - " + FilePath.substr(FilePath.find_last_of("/\\") + 1)).c_str());
					}
				} else if (io.KeyCtrl) { // Directly Save Don't Prompt
					FilePath = FixFileExtension(FilePath);
					SaveImageFromCanvas(FilePath);
				}
			} else if (ImGui::IsKeyPressed(ImGuiKey_O, false)) {
				if (io.KeyCtrl) {
					char *filePath = tinyfd_openFileDialog("Open A File", NULL, NumOfFilterPatterns, FileFilterPatterns, "Image File (.png, .jpg, .jpeg)", 0);
					if (filePath != NULL) {
						FilePath = std::string(filePath);
						LoadImageToCanvas(FilePath.c_str(), CanvasDims, &CanvasData);

						// Simple Hack To Get The File Name from the path and set it to the window title
						App::SetTitle(("CSprite - " + FilePath.substr(FilePath.find_last_of("/\\") + 1)).c_str());
					}
				}
			} 

			SelectedColor = ColorPalette[PaletteIndex];

			if (ToolManager::GetToolType() == ToolType::PAN) {
				canvas->viewport.x += io.MouseDelta.x;
				canvas->viewport.y += io.MouseDelta.y;
			}

			double x, y;
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				x = MousePosRel.x;
				y = MousePosRel.y;

				if (x >= 0 && x < CanvasDims[0] && y >= 0 && y < CanvasDims[1]) {
					switch (ToolManager::GetToolType()) {
						case BRUSH:
							ToolManager::Draw(x, y, CanvasDims[0], CanvasDims[1], SelectedColor, CanvasData);
							ToolManager::DrawBetween(
								x, y, MousePosRelLast.x, MousePosRelLast.y,
								CanvasDims[0], CanvasDims[1],
								SelectedColor, CanvasData
							);
							break;
						case ERASER: {
							ToolManager::Draw(x, y, CanvasDims[0], CanvasDims[1], EmptyColor, CanvasData);
							ToolManager::DrawBetween(
								x, y, MousePosRelLast.x, MousePosRelLast.y,
								CanvasDims[0], CanvasDims[1],
								EmptyColor, CanvasData
							);
							break;
						}
						case INK_DROPPER: {
							Pixel& color = GetPixel(x, y);

							// For loop starts from 1 because we don't need the first color i.e. 0,0,0,0 or transparent black
							for (int i = 0; i < PaletteCount; i++) {
								if (ColorPalette[i] == color) {
									PaletteIndex = i;
									break;
								}
							}
							break;
						}
						default: {
							break;
						}
					}
				}
			}
		}

#ifdef _DEBUG
		static bool metricsWinVisible = false;
#endif

		#define BEGIN_MENU(label) if (ImGui::BeginMenu(label)) {
		#define END_MENU() ImGui::EndMenu(); }

		#define BEGIN_MENUITEM(label, shortcut) if (ImGui::MenuItem(label, shortcut)) {
		#define END_MENUITEM() }

		if (ImGui::BeginMainMenuBar()) {
			BEGIN_MENU("File")
				BEGIN_MENUITEM("New", "Ctrl+N")
					ShowNewCanvasWindow = 1;
				END_MENUITEM()
				BEGIN_MENUITEM("Open", "Ctrl+O")
					char *filePath = tinyfd_openFileDialog("Open A File", NULL, NumOfFilterPatterns, FileFilterPatterns, "Image File (.png, .jpg, .jpeg)", 0);
					if (filePath != NULL) {
						FilePath = std::string(filePath);
						LoadImageToCanvas(FilePath.c_str(), CanvasDims, &CanvasData);
						ZoomNLevelViewport();

						// Simple Hack To Get The File Name from the path and set it to the window title
						App::SetTitle(("CSprite - " + FilePath.substr(FilePath.find_last_of("/\\") + 1)).c_str());
					}
				END_MENUITEM()
				BEGIN_MENU("Save")
					BEGIN_MENUITEM("Save", "Ctrl+S")
						FilePath = FixFileExtension(FilePath);
						SaveImageFromCanvas(FilePath);

						// Simple Hack To Get The File Name from the path and set it to the window title
						App::SetTitle(("CSprite - " + FilePath.substr(FilePath.find_last_of("/\\") + 1)).c_str());
					END_MENUITEM()
					BEGIN_MENUITEM("Save As", "Alt+S")
						char *filePath = tinyfd_saveFileDialog("Save A File", NULL, NumOfFilterPatterns, FileFilterPatterns, "Image File (.png, .jpg, .jpeg)");
						if (filePath != NULL) {
							FilePath = FixFileExtension(std::string(filePath));
							SaveImageFromCanvas(FilePath);

							// Simple Hack To Get The File Name from the path and set it to the window title
							App::SetTitle(("CSprite - " + FilePath.substr(FilePath.find_last_of("/\\") + 1)).c_str());
						}
					END_MENUITEM()
				END_MENU()
			END_MENU()

			BEGIN_MENU("Edit")
				BEGIN_MENUITEM("Undo", "Ctrl+Z") Undo(); END_MENUITEM()
				BEGIN_MENUITEM("Redo", "Ctrl+Y") Redo(); END_MENUITEM()
			END_MENU()

#ifdef _DEBUG
			BEGIN_MENU("Dev")
				BEGIN_MENUITEM("Metrics", NULL) metricsWinVisible = !metricsWinVisible; END_MENUITEM()
			END_MENU()
#endif

			BEGIN_MENU("Help")
				BEGIN_MENUITEM("About", NULL)
					openUrl("https://github.com/pegvin/CSprite/wiki/About-CSprite");
				END_MENUITEM()
				BEGIN_MENUITEM("GitHub", NULL)
					openUrl("https://github.com/pegvin/CSprite");
				END_MENUITEM()
			END_MENU()

			ImGui::EndMainMenuBar();
		}

		#undef BEGIN_MENUITEM
		#undef END_MENUITEM
		#undef BEGIN_MENU
		#undef END_MENU

		#define BEGIN_WINDOW(label, isOpenPtr, flags) if (ImGui::Begin(label, isOpenPtr, flags)) {
		#define END_WINDOW() ImGui::End(); }

		if (ShowNewCanvasWindow == 1) {
			CanvasFreeze = 1;
			ImGui::SetNextWindowSize({280, 100}, 0);
			BEGIN_WINDOW("NewCanvasWindow", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)
				ImGui::InputInt("width", &NEW_DIMS[0], 1, 1, 0);
				ImGui::InputInt("height", &NEW_DIMS[1], 1, 1, 0);

				if (ImGui::Button("Ok")) {
					delete[] CanvasData;
					CanvasDims[0] = NEW_DIMS[0];
					CanvasDims[1] = NEW_DIMS[1];

					CanvasData = new Pixel[CanvasDims[0] * CanvasDims[1]]{ 0, 0, 0, 0 };

					ZoomNLevelViewport();
					FreeHistory();
					CanvasFreeze = 0;
					ShowNewCanvasWindow = 0;
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel")) {
					CanvasFreeze = 0;
					ShowNewCanvasWindow = 0;
				}
			END_WINDOW()
		}

		// Saves Few CPU & GPU Time Since There's No Window Flags Processing Or Some Other Overhead.
		ImGui::GetBackgroundDrawList()->AddRect(
			{ canvas->viewport.x - 1, canvas->viewport.y - 1 },
			{ canvas->viewport.w + canvas->viewport.x + 1, canvas->viewport.h + canvas->viewport.y + 1 },
			ImGui::GetColorU32(ImGuiCol_Border), 0.0f, 0, 1.0f
		);
		ImGui::GetBackgroundDrawList()->AddImage(
			reinterpret_cast<ImTextureID>(canvas->id),
			{ canvas->viewport.x, canvas->viewport.y },
			{ canvas->viewport.w + canvas->viewport.x, canvas->viewport.h + canvas->viewport.y }
		);

#ifdef _DEBUG
		if (metricsWinVisible) {
			ImGui::ShowMetricsWindow(NULL);
		}
#endif

		BEGIN_WINDOW("ToolAndZoomWindow", NULL, window_flags | ImGuiWindowFlags_NoBringToFrontOnFocus |  ImGuiWindowFlags_NoFocusOnAppearing)
			ImGui::SetWindowPos({0, 20});
			std::string selectedToolText;

			switch (ToolManager::GetToolType()) {
				case BRUSH:
					if (ToolManager::GetToolShape() == ToolShape::CIRCLE) {
						selectedToolText = "Circle Brush - (Size: " + std::to_string(ToolManager::GetBrushSize()) + ")";
					} else {
						selectedToolText = "Square Brush - (Size: " + std::to_string(ToolManager::GetBrushSize()) + ")";
					}
					break;
				case ERASER:
					if (ToolManager::GetToolShape() == ToolShape::CIRCLE) {
						selectedToolText = "Circle Eraser - (Size: " + std::to_string(ToolManager::GetBrushSize()) + ")";
					} else {
						selectedToolText = "Square Eraser - (Size: " + std::to_string(ToolManager::GetBrushSize()) + ")";
					}
					break;
				case INK_DROPPER:
					selectedToolText = "Ink Dropper";
					break;
				case PAN:
					selectedToolText = "Panning";
					break;
			}

			ImVec2 textSize1 = ImGui::CalcTextSize(selectedToolText.c_str(), NULL, false, -2.0f);
			ImVec2 textSize2 = ImGui::CalcTextSize(ZoomText.c_str(), NULL, false, -2.0f);
			ImGui::SetWindowSize({(float)(textSize1.x + textSize2.x), (float)(textSize1.y + textSize2.y) * 2}); // Make Sure Text is visible everytime.

			ImGui::Text("%s", selectedToolText.c_str());
			ImGui::Text("%s", ZoomText.c_str());
		END_WINDOW()

		BEGIN_WINDOW("ColorPaletteWindow", NULL, window_flags)
			ImGui::SetWindowPos({ 0, io.DisplaySize.y - 35.0f });
			for (int i = 0; i < PaletteCount; i++) {
				if (i != 0) ImGui::SameLine();
				if (ImGui::ColorButton(
					PaletteIndex == i ? "Selected Color" : ("Color##" + std::to_string(i)).c_str(),
					{(float)ColorPalette[i].r/255, (float)ColorPalette[i].g/255, (float)ColorPalette[i].b/255, (float)ColorPalette[i].a/255})
				) {
					PaletteIndex = i;
					SelectedColor = ColorPalette[PaletteIndex];
				}
				if (PaletteIndex == i)
					ImGui::GetWindowDrawList()->AddRect(
						ImGui::GetItemRectMin(),
						ImGui::GetItemRectMax(),
						IM_COL32_WHITE
					);
			};
		END_WINDOW()

		#undef BEGIN_WINDOW
		#undef END_WINDOW

		canvas->Update(CanvasData);
		App::EndFrame();
	}

	FreeHistory();
	App::Release();
	return 0;
}

void ZoomNLevelViewport() {
	canvas->viewport.x = ImGui::GetIO().DisplaySize.x / 2 - (float)CanvasDims[0] * ZoomLevel / 2;
	canvas->viewport.y = ImGui::GetIO().DisplaySize.y / 2 - (float)CanvasDims[1] * ZoomLevel / 2;
	canvas->viewport.w = CanvasDims[0] * ZoomLevel;
	canvas->viewport.h = CanvasDims[1] * ZoomLevel;
}

void AdjustZoom(bool increase) {
	if (increase == true) {
		if (ZoomLevel < UINT_MAX) { // Max Value Of Unsigned int
			ZoomLevel++;
		}
	} else {
		if (ZoomLevel != 1) { // if zoom is 1 then don't decrease it further
			ZoomLevel--;
		}
	}

	// Comment Out To Not Center When Zooming
	canvas->viewport.x = ImGui::GetIO().DisplaySize.x / 2 - (float)CanvasDims[0] * ZoomLevel / 2;
	canvas->viewport.y = ImGui::GetIO().DisplaySize.y / 2 - (float)CanvasDims[1] * ZoomLevel / 2;

	canvas->viewport.w = CanvasDims[0] * ZoomLevel;
	canvas->viewport.h = CanvasDims[1] * ZoomLevel;
	ZoomText = "Zoom: " + std::to_string(ZoomLevel) + "x";
}

Pixel& GetPixel(int x, int y) {
	return CanvasData[(y * CanvasDims[0]) + x];
}

// Makes sure that the file extension is .png or .jpg/.jpeg
std::string FixFileExtension(std::string filepath) {
	std::string fileExt = filepath.substr(filepath.find_last_of(".") + 1);
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), [](unsigned char c){ return std::tolower(c); });

	if (fileExt != "png" && fileExt != "jpg" && fileExt != "jpeg") {
		filepath = filepath + ".png";
	}

	return filepath;
}

void SaveImageFromCanvas(std::string filepath) {
	std::string fileExt = filepath.substr(filepath.find_last_of(".") + 1);
	// Convert File Extension to LowerCase
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), [](unsigned char c){ return std::tolower(c); });

	if (fileExt == "png") {
		WritePngFromCanvas(filepath.c_str(), CanvasDims);
	} else if (fileExt == "jpg" || fileExt == "jpeg") {
		WriteJpgFromCanvas(filepath.c_str(), CanvasDims);
	} else {
		filepath = filepath + ".png";
		WritePngFromCanvas(filepath.c_str(), CanvasDims);
	}
	ShouldSave = 0;
}

/*
	Pushes Pixels On Current Canvas in "History" array at index "HistoryIndex"
	Removes The Elements in a range from "History" if "IsDirty" is true
*/
void SaveState() {
	if (IsDirty == true && CurrentState != NULL) {
		cvstate_t* tmp;
		cvstate_t* head = CurrentState->next; // we start freeing from the next node of current node

		while (head != NULL) {
			tmp = head;
			head = head->next;
			if (tmp->pixelData != NULL) {
				delete[] tmp->pixelData;
			}
			free(tmp);
		}
	}

	cvstate_t* NewState = (cvstate_t*) malloc(sizeof(cvstate_t));
	NewState->pixelData = new Pixel[CanvasDims[0] * CanvasDims[1]]{ 0, 0, 0, 0 };

	if (CurrentState == NULL) {
		CurrentState = NewState;
		CurrentState->prev = NULL;
		CurrentState->next = NULL;
	} else {
		NewState->prev = CurrentState;
		NewState->next = NULL;
		CurrentState->next = NewState;
		CurrentState = NewState;
	}

	memcpy(CurrentState->pixelData, CanvasData, CanvasDims[0] * CanvasDims[1] * sizeof(Pixel));
}

// Undo - Puts The Pixels from "History" at "HistoryIndex"
int Undo() {
	DidUndo = true;

	if (CurrentState->prev != NULL) {
		CurrentState = CurrentState->prev;
		memcpy(CanvasData, CurrentState->pixelData, CanvasDims[0] * CanvasDims[1] * sizeof(Pixel));
	}
	return 0;
}

// Redo - Puts The Pixels from "History" at "HistoryIndex"
int Redo() {
	if (CurrentState->next != NULL) {
		CurrentState = CurrentState->next;
		memcpy(CanvasData, CurrentState->pixelData, CanvasDims[0] * CanvasDims[1] * sizeof(Pixel));
	}

	return 0;
}

/*
	Function: FreeHistory()
	Takes The CurrentState Node
		- Frees All Of The Nodes Before It
		- Frees All Of The Nodes After It
*/
void FreeHistory() {
	if (CurrentState == NULL) return;

	cvstate_t* tmp;
	cvstate_t* head = CurrentState->prev;

	while (head != NULL) {
		tmp = head;
		head = head->prev;
		if (tmp != NULL && tmp->pixelData != NULL) {
			delete[] tmp->pixelData;
			free(tmp);
		}
		tmp = NULL;
	}

	head = CurrentState;

	while (head != NULL) {
		tmp = head;
		head = head->next;
		if (tmp != NULL && tmp->pixelData != NULL) {
			delete[] tmp->pixelData;
			free(tmp);
		}
		tmp = NULL;
	}

	CurrentState = NULL;
}
