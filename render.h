#pragma once

typedef struct
{
	ImU32 R;
	ImU32 G;
	ImU32 B;

}RGB;

ImU32 Color(RGB color)
{
	return IM_COL32(color.R, color.G, color.B, 255);
}
	
namespace Render
{
	void DrawRect(int x, int y, int w, int h, RGB color, int thickness) 
	{
		ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), Color(color), 0, 0, thickness);
	}
	void DrawLine(float x1, float y1, float x2, float y2, RGB color, float thickness)
	{
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), Color(color), thickness);
	}
}