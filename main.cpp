#include "consoleGameEngine.h"
#include <fstream>
#include <strstream>
#include <algorithm>

struct vec3d{
	float x, y, z;
};

struct triangle {
	vec3d p[3];

	wchar_t sym;
	short col;
};

struct mesh {
	std::vector<triangle> tris;

	bool LoadFromOBJ(std::string sFileName)
	{
		std::ifstream f(sFileName);
		if (!f.is_open())
			return false;

		std::vector<vec3d> verts;

		while (!f.eof())
		{
			char line[128];
			f.getline(line, 128);

			std::strstream s;
			s << line;

			char junk;
			if (line[0] == 'v')
			{
				vec3d v;
				s >> junk >> v.x >> v.y >> v.z;
				verts.push_back(v);
			}

			if (line[0] == 'f')
			{
				int f[3];
				s >> junk >> f[0] >> f[1] >> f[2];
				tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] });
			}
		}

		return true;
	}
};

struct mat4x4 {
	float m[4][4] = { 0 };
};

class Engine3D : public olcConsoleGameEngine{

public:
	Engine3D()
	{
		m_sAppName = L"3D Engine";
	}

private:
	mesh meshCube;
	mat4x4 matProj;

	vec3d vCamera;

	float fTheta;

	bool drawWireFrame = false;
	bool drawShaded = true;

	float objectOffset = 8.0f;
	float rotateSpeed = 1.0f;

	short wireFrameColor = FG_GREEN;

	void MultiplyMatrixVector(vec3d &i, vec3d &o, mat4x4 &m)
	{
		o.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + m.m[3][0];
		o.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + m.m[3][1];
		o.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + m.m[3][2];
		float w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + m.m[3][3];

		if (w != 0.0f)
		{
			o.x /= w; o.y /= w; o.z /= w;
		}
	}

	CHAR_INFO GetColour(float lum)
	{
		short bg_col, fg_col;
		wchar_t sym;
		int pixel_bw = (int)(13.0f*lum);
		switch (pixel_bw)
		{
		case 0: bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID; break;

		case 1: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_QUARTER; break;
		case 2: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_HALF; break;
		case 3: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_THREEQUARTERS; break;
		case 4: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_SOLID; break;

		case 5: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_QUARTER; break;
		case 6: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_HALF; break;
		case 7: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_THREEQUARTERS; break;
		case 8: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_SOLID; break;

		case 9:  bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_QUARTER; break;
		case 10: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_HALF; break;
		case 11: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_THREEQUARTERS; break;
		case 12: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_SOLID; break;
		default:
			bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID;
		}

		CHAR_INFO c;
		c.Attributes = bg_col | fg_col;
		c.Char.UnicodeChar = sym;
		return c;
	}

public:

	bool OnUserCreate() override
	{
		meshCube.LoadFromOBJ("utah.obj");

		//Projection mat
		float fNear = 0.1f;
		float fFar = 1000.0f;
		float fFov = 90.0f;
		float fAspectRatio = (float)ScreenWidth() / (float)ScreenHeight();
		float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);

		matProj.m[0][0] = fAspectRatio * fFovRad;
		matProj.m[1][1] = fFovRad;
		matProj.m[2][2] = fFar / (fFar - fNear);
		matProj.m[3][2] = (-fFar * fNear) / (fFar - fNear);
		matProj.m[2][3] = 1.0f;
		matProj.m[3][3] = 0.0f;


		return true;
	}

	bool OnUserUpdate(float delta) override
	{

		if (m_keys[0x57].bPressed)
		{
			drawWireFrame = !drawWireFrame;
		}

		if (m_keys[0x45].bPressed)
		{
			drawWireFrame = !drawWireFrame;
			drawShaded = !drawShaded;
		}

		if (m_keys[0x31].bPressed)
		{
			if (rotateSpeed == 1.0f) rotateSpeed = 10.0f;
			else rotateSpeed = 1.0f;
		}

		Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);

		mat4x4 matRotZ, matRotX;
		fTheta += rotateSpeed * delta;

		// Rotation Z
		matRotZ.m[0][0] = cosf(fTheta);
		matRotZ.m[0][1] = sinf(fTheta);
		matRotZ.m[1][0] = -sinf(fTheta);
		matRotZ.m[1][1] = cosf(fTheta);
		matRotZ.m[2][2] = 1;
		matRotZ.m[3][3] = 1;

		// Rotation X
		matRotX.m[0][0] = 1;
		matRotX.m[1][1] = cosf(fTheta * 0.5f);
		matRotX.m[1][2] = sinf(fTheta * 0.5f);
		matRotX.m[2][1] = -sinf(fTheta * 0.5f);
		matRotX.m[2][2] = cosf(fTheta * 0.5f);
		matRotX.m[3][3] = 1;

		std::vector<triangle> vecTrianglesToRaster;

		for (auto tri : meshCube.tris)
		{
			triangle triProjected, triTranslate, triRotatedZ, triRotatedZX;
			
			// Rotate Mesh
			MultiplyMatrixVector(tri.p[0], triRotatedZ.p[0], matRotZ);
			MultiplyMatrixVector(tri.p[1], triRotatedZ.p[1], matRotZ);
			MultiplyMatrixVector(tri.p[2], triRotatedZ.p[2], matRotZ);

			MultiplyMatrixVector(triRotatedZ.p[0], triRotatedZX.p[0], matRotX);
			MultiplyMatrixVector(triRotatedZ.p[1], triRotatedZX.p[1], matRotX);
			MultiplyMatrixVector(triRotatedZ.p[2], triRotatedZX.p[2], matRotX);
			
			triTranslate = triRotatedZX;
			triTranslate.p[0].z = triRotatedZX.p[0].z + objectOffset;
			triTranslate.p[1].z = triRotatedZX.p[1].z + objectOffset;
			triTranslate.p[2].z = triRotatedZX.p[2].z + objectOffset;

			// Calculate normal
			vec3d normal, line1, line2;
			line1.x = triTranslate.p[1].x - triTranslate.p[0].x;
			line1.y = triTranslate.p[1].y - triTranslate.p[0].y;
			line1.z = triTranslate.p[1].z - triTranslate.p[0].z;

			line2.x = triTranslate.p[2].x - triTranslate.p[0].x;
			line2.y = triTranslate.p[2].y - triTranslate.p[0].y;
			line2.z = triTranslate.p[2].z - triTranslate.p[0].z;

			normal.x = line1.y * line2.z - line1.z * line2.y;
			normal.y = line1.z * line2.x - line1.x * line2.z;
			normal.z = line1.x * line2.y - line1.y * line2.x;
			
			float l = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z * normal.z);
			normal.x /= l; normal.y /= l; normal.z /= l;

			// When normal is towards camera
			if (normal.x * (triTranslate.p[0].x - vCamera.x)+
				normal.y * (triTranslate.p[0].y - vCamera.y)+
				normal.z * (triTranslate.p[0].z - vCamera.z) < 0.0f
				) {

				// Calculate object lighting
				vec3d lightDirection = { 0.0f, 0.0f, -1.0f };
				float l = sqrtf(lightDirection.x*lightDirection.x + lightDirection.y*lightDirection.y + lightDirection.z * lightDirection.z);
				lightDirection.x /= l; lightDirection.y /= l; lightDirection.z /= l;

				float lightDp = normal.x * lightDirection.x + normal.y * lightDirection.y + normal.z * lightDirection.z;

				// Find face color based on lighting
				CHAR_INFO c = GetColour(lightDp);
				triTranslate.col = c.Attributes;
				triTranslate.sym = c.Char.UnicodeChar;

				// Project triangle from 3D to screen projection
				MultiplyMatrixVector(triTranslate.p[0], triProjected.p[0], matProj);
				MultiplyMatrixVector(triTranslate.p[1], triProjected.p[1], matProj);
				MultiplyMatrixVector(triTranslate.p[2], triProjected.p[2], matProj);

				triProjected.col = triTranslate.col;
				triProjected.sym = triTranslate.sym;


				triProjected.p[0].x += 1.0f; triProjected.p[0].y += 1.0f;
				triProjected.p[1].x += 1.0f; triProjected.p[1].y += 1.0f;
				triProjected.p[2].x += 1.0f; triProjected.p[2].y += 1.0f;

				triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
				triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
				triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
				triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
				triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
				triProjected.p[2].y *= 0.5f * (float)ScreenHeight();


				vecTrianglesToRaster.push_back(triProjected);
			}
		}

		sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](triangle &t1, triangle &t2) {
			float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
			float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
			return z1 > z2;
		});


		for (auto &triProjected : vecTrianglesToRaster)
		{
			if (drawShaded) {
				FillTriangle(triProjected.p[0].x, triProjected.p[0].y,
					triProjected.p[1].x, triProjected.p[1].y,
					triProjected.p[2].x, triProjected.p[2].y,
					triProjected.sym, triProjected.col
				);
			}

			if (drawWireFrame) {
				DrawTriangle(triProjected.p[0].x, triProjected.p[0].y,
					triProjected.p[1].x, triProjected.p[1].y,
					triProjected.p[2].x, triProjected.p[2].y,
					PIXEL_SOLID, FG_BLACK
				);
			}

			if (!drawShaded && drawWireFrame)
			{
				DrawTriangle(triProjected.p[0].x, triProjected.p[0].y,
					triProjected.p[1].x, triProjected.p[1].y,
					triProjected.p[2].x, triProjected.p[2].y,
					PIXEL_SOLID, wireFrameColor
				);
			}
		}
		
		return true;
	}
	
};


int main()
{
	Engine3D engine;

	if (engine.ConstructConsole(256 * 2, 240 * 2, 2, 2))
	{
		engine.Start();
	}
	else {
		return 1;
	}
}