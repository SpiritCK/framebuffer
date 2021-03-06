#include "framebuffer.h"
#include <stdio.h>

Color c_make(unsigned char r, unsigned char g, unsigned char b) {
	Color c;
	c.r = r;
	c.g = g;
	c.b = b;
	return c;
}

Point p_make(long x, long y) {
	Point p;
	p.x = x;
	p.y = y;
	return p;
}

void fb_init(FrameBuffer *fb, char *fbPath) {
	// Open frame buffer device
	fb->fileDescriptor = open(fbPath, O_RDWR);
	if(!fb->fileDescriptor) fprintf(stderr, "Error: can't open framebuffer");

	// Write settings to variable screen info, then re-read
	ioctl(fb->fileDescriptor, FBIOGET_VSCREENINFO, &fb->vinfo);
	fb->vinfo.grayscale = 0;
	fb->vinfo.bits_per_pixel = 32;
	fb->vinfo.xoffset = 0;
	fb->vinfo.yoffset = 0;
	ioctl(fb->fileDescriptor, FBIOPUT_VSCREENINFO, &fb->vinfo);
	ioctl(fb->fileDescriptor, FBIOGET_VSCREENINFO, &fb->vinfo);

	// Read fixed screen info
	ioctl(fb->fileDescriptor, FBIOGET_FSCREENINFO, &fb->finfo);

	long screenMemorySize = fb->vinfo.yres_virtual * fb->finfo.line_length;
	fb->address = mmap(0, screenMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fileDescriptor, (off_t) 0);
}

void fb_output(FrameBuffer *fb) {
	memcpy((uint32_t*) fb->address, &fb->backBuffer, fb->vinfo.xres * fb->vinfo.yres * (fb->vinfo.bits_per_pixel / 8));
}

void fb_printInfo(FrameBuffer *fb) {
	printf("xres: %d\n", fb->vinfo.xres);
	printf("yres: %d\n", fb->vinfo.yres);
	printf("xres_virtual: %d\n", fb->vinfo.xres_virtual);
	printf("yres_virtual: %d\n", fb->vinfo.yres_virtual);
	printf("xoffset: %d\n", fb->vinfo.xoffset);
	printf("yoffset: %d\n", fb->vinfo.yoffset);
	printf("bits_per_pixel: %d\n", fb->vinfo.bits_per_pixel);
	printf("line_length: %d\n", fb->finfo.line_length);
}

long fb_getWidth(FrameBuffer *fb) {
	return fb->vinfo.xres;
}

long fb_getHeight(FrameBuffer *fb) {
	return fb->vinfo.yres;
}

void fb_clear(FrameBuffer *fb, Color color) {
	int x, y;
	for (x = 0; x < fb_getWidth(fb); x++) {
		for (y = 0; y < fb_getHeight(fb); y++) {
			fb_drawPixel(fb, p_make(x, y), color);
		}
	}
}

uint32_t getColorValue(FrameBuffer *fb, Color color) {
	return (color.r << fb->vinfo.red.offset)
		| (color.g << fb->vinfo.green.offset)
		| (color.b << fb->vinfo.blue.offset);
}

void fb_drawPixel(FrameBuffer *fb, Point point, Color color) {
	if (point.x >= 0 && point.x < fb->vinfo.xres && point.y >= 0 && point.y < fb->vinfo.yres) {
		long addressOffset = point.x + (point.y * fb->finfo.line_length / (fb->vinfo.bits_per_pixel / 8));
		uint32_t colorValue = getColorValue(fb, color);
		fb->backBuffer[addressOffset] = colorValue;
	}
}

void fb_drawRectangle(FrameBuffer *fb, Point p1, Point p2, Color color) {
	int x, y;
	for (x = p1.x; x < p2.x; x++) {
		for (y = p1.y; y < p2.y; y++) {
			fb_drawPixel(fb, p_make(x, y), color);
		}
	}
}

void fb_drawCircle(FrameBuffer *fb, Point center, double r, Color color) {
	int x, y;
	for (x = center.x - (int) r; x <= center.x + (int) r; x++) {
		for (y = center.y - (int) r; y <= center.y + (int) r; y++) {
			int dx = center.x - x;
			int dy = center.y - y;
			if (dx*dx + dy*dy <= (int) (r*r)) {
				fb_drawPixel(fb, p_make(x, y), color);
			}
		}
	}	
}

void fb_drawCircleOutline(FrameBuffer *fb, Point center, double r, double thickness, Color color) {
	int x, y;
	for (x = center.x - (int) r; x <= center.x + (int) r; x++) {
		for (y = center.y - (int) r; y <= center.y + (int) r; y++) {
			int dx = center.x - x;
			int dy = center.y - y;
			int innerRadius = r - thickness;
			if ((dx*dx + dy*dy <= (int) (r*r)) && (dx*dx + dy*dy > (int) (innerRadius*innerRadius))) {
				fb_drawPixel(fb, p_make(x, y), color);
			}
		}
	}	
}
