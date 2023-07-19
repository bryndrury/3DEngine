#include <SDL.h>
#include <iostream>
#include <fstream>
#include <strstream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

#define fps 60

struct vec3d
{
	float x, y, z;
};

struct triangle
{
	vec3d p[3];

	Uint8 lum;
};

struct mesh
{
	std::vector<triangle> tris;

	bool LoadFromObjectFile(std::string sFilename)
	{
		std::ifstream f(sFilename);
		if (!f.is_open())
			return false;

		// Local cache of verts
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

struct mat4x4
{
	float m[4][4] = { 0 };
};

// Complimentary Functions
void MultiplyMatrixVector(vec3d &i, vec3d &o, mat4x4 &m);

void cap_framerate( Uint32 starting_tick ); 


int main(int argc, char* argv[]) {

	// Create Window
	float screen_width = 1280;
	float screen_height = 720;
	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;

	// Render Black Screen
	//SDL_CreateWindowAndRenderer(screen_width, screen_height, SDL_WINDOW_RESIZABLE, &window, &renderer);
	window = SDL_CreateWindow("3DEngine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, (SDL_WINDOW_SHOWN, SDL_WINDOW_RESIZABLE));
	renderer = SDL_CreateRenderer(window, -1, (SDL_RENDERER_ACCELERATED, SDL_RENDERER_PRESENTVSYNC));
	SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
	SDL_RenderClear( renderer );
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	// Objects and Camera
	mesh meshCube;
	meshCube.LoadFromObjectFile("teapot.obj");
	/* 
	meshCube.tris = {

		// SOUTH
		{ 0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },

		// EAST                                                      
		{ 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f },

		// NORTH                                                     
		{ 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f },

		// WEST                                                      
		{ 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f },

		// TOP                                                       
		{ 0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 0.0f },

		// BOTTOM                                                    
		{ 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f },

	};
	*/
	vec3d vCamera;
	vCamera.x = 0; vCamera.y = 0; vCamera.z = 0;
	// Rendering Options
	bool wireframe = false;
	bool fillVert = true;


	// Projection Matrix
	float fNear = 0.1f;
	float fFar = 1000.0f;
	float fFov = 90.0f;
	float fAspectRatio = screen_height / screen_width;
	float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);

	// Project Objects on to 2D Screen
	mat4x4 matProj;
	matProj.m[0][0] = fAspectRatio * fFovRad;
	matProj.m[1][1] = fFovRad;
	matProj.m[2][2] = fFar / (fFar - fNear);
	matProj.m[3][2] = (-fFar * fNear) / (fFar - fNear);
	matProj.m[2][3] = 1.0f;
	matProj.m[3][3] = 0.0f;

	// Internal clock
	const auto startTime = std::chrono::steady_clock::now();
	std::chrono::duration<double> fElapsedTime = std::chrono::steady_clock::now() - startTime;
	Uint32 starting_tick;

	// Initializing main loop
	SDL_Event event;
	bool running = true;

	while (running) {

		starting_tick = SDL_GetTicks();

		fElapsedTime = std::chrono::steady_clock::now() - startTime;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				running = false;
				break;
			}
		}

		mat4x4 matRotZ, matRotX;
		float fTheta = 0;
		fTheta += 1.0f * fElapsedTime.count();

		// Rotation about Z
		matRotZ.m[0][0] = cosf(fTheta);
		matRotZ.m[0][1] = sinf(fTheta);
		matRotZ.m[1][0] = -sinf(fTheta);
		matRotZ.m[1][1] = cosf(fTheta);
		matRotZ.m[2][2] = 1;
		matRotZ.m[3][3] = 1;

		// Rotation about X
		matRotX.m[0][0] = 1;
		matRotX.m[1][1] = cosf(fTheta * 0.5f);
		matRotX.m[1][2] = sinf(fTheta * 0.5f);
		matRotX.m[2][1] = -sinf(fTheta * 0.5f);
		matRotX.m[2][2] = cosf(fTheta * 0.5f);
		matRotX.m[3][3] = 1;

		// Clear Frame and create new
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		std::vector<triangle> vecTrianglesToRaster;

		for (auto tri : meshCube.tris)
		{
			triangle triProjected, triTranslated, triRotatedZ, triRotatedZX;

			// Rotating the model
			MultiplyMatrixVector(tri.p[0], triRotatedZ.p[0], matRotZ);
			MultiplyMatrixVector(tri.p[1], triRotatedZ.p[1], matRotZ);
			MultiplyMatrixVector(tri.p[2], triRotatedZ.p[2], matRotZ);

			MultiplyMatrixVector(triRotatedZ.p[0], triRotatedZX.p[0], matRotX);
			MultiplyMatrixVector(triRotatedZ.p[1], triRotatedZX.p[1], matRotX);
			MultiplyMatrixVector(triRotatedZ.p[2], triRotatedZX.p[2], matRotX);

			triTranslated = triRotatedZX;
			triTranslated.p[0].z = triRotatedZX.p[0].z + 5.0f;
			triTranslated.p[1].z = triRotatedZX.p[1].z + 5.0f;
			triTranslated.p[2].z = triRotatedZX.p[2].z + 5.0f;

			// Selective Rendering
			vec3d normal, line1, line2;
			line1.x = triTranslated.p[1].x - triTranslated.p[0].x;
			line1.y = triTranslated.p[1].y - triTranslated.p[0].y;
			line1.z = triTranslated.p[1].z - triTranslated.p[0].z;

			line2.x = triTranslated.p[2].x - triTranslated.p[0].x;
			line2.y = triTranslated.p[2].y - triTranslated.p[0].y;
			line2.z = triTranslated.p[2].z - triTranslated.p[0].z;

			normal.x = line1.y * line2.z - line1.z * line2.y;
			normal.y = line1.z * line2.x - line1.x * line2.z;
			normal.z = line1.x * line2.y - line1.y * line2.x;

			float l = sqrtf( normal.x * normal.x + normal.y * normal.y + normal.z * normal.z );
			normal.x /= 1; normal.y /= l; normal.z /= l;

			if (normal.x * (triTranslated.p[0].x - vCamera.x) +
				normal.y * (triTranslated.p[0].y - vCamera.y) +
				normal.z * (triTranslated.p[0].z - vCamera.z) < 0.0f )
			{
				// Illumination
				vec3d light_direction = { 0.0f, 0.0f, -1.0f };
				float l = sqrtf(light_direction.x * light_direction.x + light_direction.y * light_direction.y + light_direction.z * light_direction.z);
				light_direction.x /= 1; light_direction.y /= l; light_direction.z /= l;

				float dp = normal.x * light_direction.x + normal.y * light_direction.y + normal.z * light_direction.z;

				Uint8 luminance = dp * 255;
				triTranslated.lum = luminance;

				// Pojecting the model
				MultiplyMatrixVector(triTranslated.p[0], triProjected.p[0], matProj);
				MultiplyMatrixVector(triTranslated.p[1], triProjected.p[1], matProj);
				MultiplyMatrixVector(triTranslated.p[2], triProjected.p[2], matProj);
				triProjected.lum = triTranslated.lum;


				// Scaling for screen
				triProjected.p[0].x += 1.0f;	triProjected.p[0].y += 1.0f;
				triProjected.p[1].x += 1.0f;	triProjected.p[1].y += 1.0f;
				triProjected.p[2].x += 1.0f;	triProjected.p[2].y += 1.0f;

				triProjected.p[0].x *= 0.5f * screen_width;
				triProjected.p[0].y *= 0.5f * screen_height;

				triProjected.p[1].x *= 0.5f * screen_width;
				triProjected.p[1].y *= 0.5f * screen_height;

				triProjected.p[2].x *= 0.5f * screen_width;
				triProjected.p[2].y *= 0.5f * screen_height;

				vecTrianglesToRaster.push_back(triProjected);

			}
		}

		sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](triangle& t1, triangle& t2)
			{
				float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
				float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
				return z1 > z2;
			});


		for (auto& triProjected : vecTrianglesToRaster)
		{
			// Drawing the triangle wireframe
			if (wireframe)
			{
				if (fillVert) SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				else SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
				SDL_RenderDrawLine(renderer, triProjected.p[0].x, triProjected.p[0].y, triProjected.p[1].x, triProjected.p[1].y);
				SDL_RenderDrawLine(renderer, triProjected.p[1].x, triProjected.p[1].y, triProjected.p[2].x, triProjected.p[2].y);
				SDL_RenderDrawLine(renderer, triProjected.p[0].x, triProjected.p[0].y, triProjected.p[2].x, triProjected.p[2].y);
			}

			// Drawing triangle fill
			if (fillVert)
			{
				Uint8 lum = triProjected.lum;
				std::vector<SDL_Vertex> verts =
				{
					{ SDL_FPoint{ triProjected.p[0].x, triProjected.p[0].y }, SDL_Color{lum, lum, lum, 255}, SDL_FPoint{ 0 } },
					{ SDL_FPoint{ triProjected.p[1].x, triProjected.p[1].y }, SDL_Color{lum, lum, lum, 255}, SDL_FPoint{ 0 } },
					{ SDL_FPoint{ triProjected.p[2].x, triProjected.p[2].y }, SDL_Color{lum, lum, lum, 255}, SDL_FPoint{ 0 } },
				};
				SDL_RenderGeometry(renderer, nullptr, verts.data(), verts.size(), nullptr, 0);
			}
		}

		// Rendering 
		SDL_RenderPresent(renderer);
		
		// FPS limiter
		cap_framerate( starting_tick );
	}

	SDL_DestroyWindow( window );
	SDL_Quit();

	return 0;
}


void MultiplyMatrixVector(vec3d &i, vec3d &o, mat4x4 &m)
{
	o.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + m.m[3][0];
	o.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + m.m[3][1];
	o.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + m.m[3][2];
	float w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + m.m[3][3];

	if (w != 0.0f)
	{
		o.x /= w;
		o.y /= w;
		o.z /= w;
	}
}

/*
void DrawTriangle(SDL_Renderer* renderer,	float x1, float y1,		float x2, float y2,		float x3, float y3)
{
	SDL_RenderDrawLine( renderer, x1, y1, x2, y2 );
	SDL_RenderDrawLine( renderer, x2, y2, x3, y3 );
	SDL_RenderDrawLine( renderer, x1, y1, x3, y3 );
	SDL_RenderPresent( renderer );
}

void FillTriangle(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float x3, float y3)
{
	std::vector<SDL_Vertex> verts =
	{
		{ SDL_FPoint{x1, y1}, SDL_Color{255, 255, 255, 255}, SDL_FPoint{ 0 } },
		{ SDL_FPoint{x2, y2}, SDL_Color{255, 255, 255, 255}, SDL_FPoint{ 0 } },
		{ SDL_FPoint{x3, y3}, SDL_Color{255, 255, 255, 255}, SDL_FPoint{ 0 } },
	};
	SDL_RenderGeometry( renderer, nullptr, verts.data(), verts.size(), nullptr, 0);
}
*/

void cap_framerate(Uint32 starting_tick)
{
	if ((1000 / fps) > SDL_GetTicks() - starting_tick)
	{
		SDL_Delay(1000 / fps - (SDL_GetTicks() - starting_tick));
		// std::cout << "FPS: " << 1000 / (SDL_GetTicks() - starting_tick) << std::endl;
	}
}
