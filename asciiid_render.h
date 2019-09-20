#pragma once

#include <stdint.h>
#include <assert.h>

struct AnsiCell
{
	uint8_t glyph; // CP437 only
	uint8_t fg, bk; // palettized!
	uint8_t spare; // for post pass
};

bool Render(float angle, int cx, int cy, int w, int h, int pitch, AnsiCell* ptr);

template <typename Sample, typename Shader>
inline void Rasterize(Sample* buf, int w, int h, Shader* s, const int* v[3])
{
	// each v[i] must point to 4 ints: {x,y,z,f} where f should indicate culling bits (can be 0)
	// shader must implement: bool Shader::Fill(Sample* s, int bc[3])
	// where bc contains 3 barycentric weights which are normalized to 0x8000 (use '>>15' after averaging)
	// Sample must implement bool DepthTest(int z, int divisor);
	// it must return true if z/divisor passes depth test on this sample
	// if test passes, it should write new z/d to sample's depth (if something like depth write mask is enabled)

	// produces samples between buffer cells 
	// #define BC(a,b,c) ((c)[0] - (a)[0]) * ((b)[1] - (a)[1]) - ((c)[1] - (a)[1]) * ((b)[0] - (a)[0])

	// produces samples at centers of buffer cells 
	#define BC(a,b,c) ((c)[0]*2+1 - (a)[0]*2) * ((b)[1] - (a)[1]) - ((c)[1]*2+1 - (a)[1]*2) * ((b)[0] - (a)[0])

	if ((v[0][3] & v[1][3] & v[2][3]) == 0)
	{
		int area = BC(v[0], v[1], v[2]);

		if (area > 0)
		{
			assert(area < 0x10000);
			int mul = 0x80000000 / area;

			// canvas intersection with triangle bbox
			int left = std::max(0, std::min(v[0][0], std::min(v[1][0], v[2][0])));
			int right = std::min(w, std::max(v[0][0], std::max(v[1][0], v[2][0])));
			int bottom = std::max(0, std::min(v[0][1], std::min(v[1][1], v[2][1])));
			int top = std::min(h, std::max(v[0][1], std::max(v[1][1], v[2][1])));

			Sample* col = buf + bottom * w + left;
			for (int y = bottom; y < top; y++, col+=w)
			{
				Sample* row = col;
				for (int x = left; x < right; x++, row++)
				{
					int p[2] = { x,y };

					int bc[3] =
					{
						BC(v[1], v[2], p),
						BC(v[2], v[0], p),
						BC(v[0], v[1], p)
					};

					// if (bc[0] >= 0 && bc[1] >= 0 && bc[2] >= 0)
					if ((bc[0] | bc[1] | bc[2])>=0)
					{
						// normalize and round coords to 0..0x8000 range
						bc[0] = (bc[0] * mul + 0x8000) >> 16;
						bc[1] = (bc[1] * mul + 0x8000) >> 16;
						bc[2] = (bc[2] * mul + 0x8000) >> 16;

						int z = (bc[0] * v[0][2] + bc[1] * v[1][2] + bc[2] * v[2][2] + 0x4000) >> 15;

						if (row->DepthTest(z))
							s->Fill(row, bc);
					}
				}
			}
		}
	}
	#undef BC
}
