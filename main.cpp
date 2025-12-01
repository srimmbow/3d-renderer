#define OLC_PGE_APPLICATION
#include "olcPixelEngine.h"
#include <cmath>
#include <fstream>
#include <sstream>

struct vec3d
{
	float x, y, z;
};

struct triangle
{
	vec3d point[3];
};

class mesh
{
public:
	mesh() = default;
	mesh(std::vector<triangle> t) {
		this->tris = t;
	}

	void set_tris(std::vector<triangle> tris) {
		this->tris = tris;
	}

	std::vector<triangle>& get_tris() {
		return this->tris;
	}

	bool load_from_object_file(std::string sfileName) {
		std::ifstream f(sfileName);
		if (!f.is_open())
			return false;

		// Local cache of verts
		std::vector<vec3d> verts;

		while (!f.eof()) {
			char line[128];
			f.getline(line, 128);

			std::stringstream s;
			s << line;

			char junk;

			if (line[0] == 'v') {
				vec3d v;
				s >> junk >> v.x >> v.y >> v.z;
				verts.push_back(v);
			}

			if (line[0] == 'f') {
				int f[3];
				s >> junk >> f[0] >> f[1] >> f[2];
				tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] });
			}
		}

		return true;
	}

private:
	std::vector<triangle> tris;
};

struct mat4x4
{
	float m[4][4] {};
};

class Engine : public olc::PixelGameEngine
{
public:
	Engine() {
		sAppName = "3D Rendering";
	}

private:
	mesh meshObject;
	mat4x4 matProj;
	float* m_DepthBuffer = nullptr;
	float translation = 3.0f;

	vec3d vCamera;

	float fTheta;

	bool OnUserDestroy() override
	{
		// Clean up memory
		delete[] m_DepthBuffer;
		m_DepthBuffer = nullptr;
		return true;
	}

	void MultiplyMatrixVector(vec3d& i, vec3d& o, mat4x4& m)
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

	bool OnUserCreate() override {
		m_DepthBuffer = new float[ScreenWidth() * ScreenHeight()];
		meshObject.load_from_object_file("catbetter.obj");
		translation = 70.0f;

		//meshObject.set_tris( {

		//	// SOUTH
		//	{ 0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f },
		//	{ 0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },

		//	// EAST                                                      
		//	{ 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f },
		//	{ 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f },

		//	// NORTH                                                     
		//	{ 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f },
		//	{ 1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f },

		//	// WEST                                                      
		//	{ 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f },
		//	{ 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f },

		//	// TOP                                                       
		//	{ 0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f },
		//	{ 0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 0.0f },

		//	// BOTTOM                                                    
		//	{ 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f },
		//	{ 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f },

		//});

		// Projection
		float fNear = 0.1f;
		float fFar = 1000.0f;
		float fFov = 90.0f;
		float fAspectRatio = (float)ScreenHeight() / (float)ScreenWidth();
		float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);

		matProj.m[0][0] = fAspectRatio * fFovRad;
		matProj.m[1][1] = fFovRad;
		matProj.m[2][2] = fFar / (fFar - fNear);
		matProj.m[3][2] = (-fFar * fNear) / (fFar - fNear);
		matProj.m[2][3] = 1.0f;
		matProj.m[3][3] = 0.0f;

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override {
		FillRect(olc::vi2d(0, 0), olc::vi2d(ScreenWidth(), ScreenHeight()), olc::BLACK); // clear screen

		// Clear the depth buffer by filling it with the maximum possible depth value
		std::fill(m_DepthBuffer, m_DepthBuffer + ScreenWidth() * ScreenHeight(), std::numeric_limits<float>::infinity());

		// Set up rotation matrices
		mat4x4 matRotZ, matRotY;
		fTheta += 0.7f * fElapsedTime;

		// Rotation Z
		matRotZ.m[0][0] = cosf(3.14159f);
		matRotZ.m[0][1] = sinf(3.14159f);
		matRotZ.m[1][0] = -sinf(3.14159f);
		matRotZ.m[1][1] = cosf(3.14159f);
		matRotZ.m[2][2] = 1;
		matRotZ.m[3][3] = 1;

		// Rotation Y
		matRotY.m[0][0] = cosf(fTheta * 0.5f);
		matRotY.m[0][2] = sinf(fTheta * 0.5f);
		matRotY.m[1][1] = 1;
		matRotY.m[2][0] = -sinf(fTheta * 0.5f);
		matRotY.m[2][2] = cosf(fTheta * 0.5f);
		matRotY.m[3][3] = 1;

		// Draw triangles
		for (triangle& tri : meshObject.get_tris()) {
			triangle triProjected, triTranslated, triRotatedZ, triRotatedZX;

			// Rotate in Z-Axis
			MultiplyMatrixVector(tri.point[0], triRotatedZ.point[0], matRotZ);
			MultiplyMatrixVector(tri.point[1], triRotatedZ.point[1], matRotZ);
			MultiplyMatrixVector(tri.point[2], triRotatedZ.point[2], matRotZ);

			// Rotate in Y-Axis
			MultiplyMatrixVector(triRotatedZ.point[0], triRotatedZX.point[0], matRotY);
			MultiplyMatrixVector(triRotatedZ.point[1], triRotatedZX.point[1], matRotY);
			MultiplyMatrixVector(triRotatedZ.point[2], triRotatedZX.point[2], matRotY);

			// Offset into the screen
			triTranslated = triRotatedZX;
			triTranslated.point[0].z = triRotatedZX.point[0].z + translation;
			triTranslated.point[1].z = triRotatedZX.point[1].z + translation;
			triTranslated.point[2].z = triRotatedZX.point[2].z + translation;

			triTranslated.point[0].y = triRotatedZX.point[0].y + 25.0f;
			triTranslated.point[1].y = triRotatedZX.point[1].y + 25.0f;
			triTranslated.point[2].y = triRotatedZX.point[2].y + 25.0f;

			vec3d normal, line1, line2;
			line1.x = triTranslated.point[1].x - triTranslated.point[0].x;
			line1.y = triTranslated.point[1].y - triTranslated.point[0].y;
			line1.z = triTranslated.point[1].z - triTranslated.point[0].z;

			line2.x = triTranslated.point[2].x - triTranslated.point[0].x;
			line2.y = triTranslated.point[2].y - triTranslated.point[0].y;
			line2.z = triTranslated.point[2].z - triTranslated.point[0].z;

			normal.x = line1.y * line2.z - line1.z * line2.y;
			normal.y = line1.z * line2.x - line1.x * line2.z;
			normal.z = line1.x * line2.y - line1.y * line2.x;

			float l = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
			normal.x /= l; normal.y /= l; normal.z /= l;

			// if we can see the triangle
			if (normal.x * (triTranslated.point[0].x - vCamera.x) +
				normal.y * (triTranslated.point[0].y - vCamera.y) +
				normal.z * (triTranslated.point[0].z - vCamera.z) < 0.0f) {

				// Lighting
				vec3d light_direction {0.0f, 0.0f, -1.0f};
				float l = sqrtf(light_direction.x * light_direction.x + light_direction.y * light_direction.y + light_direction.z * light_direction.z);
				light_direction.x /= l; light_direction.y /= l; light_direction.z /= l;

				float dp = normal.x * light_direction.x + normal.y * light_direction.y + normal.z * light_direction.z;

				// Project
				MultiplyMatrixVector(triTranslated.point[0], triProjected.point[0], matProj);
				MultiplyMatrixVector(triTranslated.point[1], triProjected.point[1], matProj);
				MultiplyMatrixVector(triTranslated.point[2], triProjected.point[2], matProj);

				// Scale into view
				triProjected.point[0].x += 1.0f; triProjected.point[0].y += 1.0f;
				triProjected.point[1].x += 1.0f; triProjected.point[1].y += 1.0f;
				triProjected.point[2].x += 1.0f; triProjected.point[2].y += 1.0f;

				triProjected.point[0].x *= 0.5f * (float)ScreenWidth();
				triProjected.point[0].y *= 0.5f * (float)ScreenHeight();
				triProjected.point[1].x *= 0.5f * (float)ScreenWidth();
				triProjected.point[1].y *= 0.5f * (float)ScreenHeight();
				triProjected.point[2].x *= 0.5f * (float)ScreenWidth();
				triProjected.point[2].y *= 0.5f * (float)ScreenHeight();

				// calculate buffer depth
				float average_z = (triProjected.point[0].z + triProjected.point[0].z + triProjected.point[0].z) / 3.0f;
				float buffer_depth_val = (average_z - 0.1f) / 999.9f;

				FillTriangle(
					static_cast<int32_t>(std::round(triProjected.point[0].x)), static_cast<int32_t>(std::round(triProjected.point[0].y)),
					static_cast<int32_t>(std::round(triProjected.point[1].x)), static_cast<int32_t>(std::round(triProjected.point[1].y)),
					static_cast<int32_t>(std::round(triProjected.point[2].x)), static_cast<int32_t>(std::round(triProjected.point[2].y)),
					olc::PixelF(dp, dp, dp),
					buffer_depth_val,
					m_DepthBuffer
				);

				//DrawTriangle(
				//	static_cast<int32_t>(std::round(triProjected.point[0].x)), static_cast<int32_t>(std::round(triProjected.point[0].y)),
				//	static_cast<int32_t>(std::round(triProjected.point[1].x)), static_cast<int32_t>(std::round(triProjected.point[1].y)),
				//	static_cast<int32_t>(std::round(triProjected.point[2].x)), static_cast<int32_t>(std::round(triProjected.point[2].y)),
				//	olc::BLACK
				//);
			}
		}

		return true;
	}
};

int main() {

	Engine demo;

	if (demo.Construct(1920, 1080, 1, 1)) {
		demo.Start();
	}

	return 0;
}