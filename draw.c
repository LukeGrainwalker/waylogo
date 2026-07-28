#include "draw.h"
#include "util.h"

/**
 * Draws a checkerboard pattern, for testing puroses.
 */
void checker(uint32_t *buf, int width, int height){
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if ((x + (y / 8 * 8))%16 < 8) {
				buf[y * width + x] = 0xFF606060;
			} else {
				buf[y * width + x] = 0xFFE0E0E0;
			}
		}
	}
}

/**
 * uses plutosvg to draw an svg image
 */
void image(uint32_t *buf, plutosvg_document_t *svg, int width, int height){
	ireport("redering the svg (image())");
	//prepair surface
	int stride = width * 4;
	plutovg_surface_t *dest = plutovg_surface_create_for_data((unsigned char *)buf, width, height, stride);
	//prepair canvas
	plutovg_canvas_t *cv = plutovg_canvas_create(dest);

	// calculate transformation
	float scale;
	float tx = 0.0;
	float ty = 0.0;
	float svgh = plutosvg_document_get_height(svg);
	float svgw = plutosvg_document_get_width(svg);
	if (width > height){
		scale = height / svgh;
		tx = (width - (svgw*scale)) / 2.0;
	} else {
		scale = width / svgw;
		ty = (height - (svgh*scale)) / 2.0;
	}

	//render the svg
	plutovg_canvas_translate(cv, tx, ty);
	plutovg_canvas_scale(cv, scale, scale);
	plutosvg_document_render(svg, NULL, cv, NULL, NULL, NULL);

	//cleanup
	plutovg_canvas_save(cv);
	plutovg_canvas_destroy(cv);
	plutovg_surface_destroy(dest);
}

