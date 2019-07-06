#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/feature/commandline.h>
#include <mgba/gba/core.h>

#include <mgba-util/memory.h>
#include <mgba-util/string.h>
#include <mgba-util/vfs.h>
#include <mgba/internal/gba/gba.h>

#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

char* videoBuffer = NULL;
char* frameBuffer = NULL;
char* Buffer = NULL;

int fb_width = 0;
int fb_height = 0;
int fb_depth = 0;

int plotMandelbrot(int width, int height, int depth) {
	float scale = 5e-3;
	char* buf = frameBuffer;

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			float xx = ((float) x - width / 2) * scale;
			float yy = ((float) y - height / 2) * scale;
			float re = xx;
			float im = yy;
			int iter = 0;
			while (1) {
				iter += 1;
				float new_re = re * re - im * im + re;
				float new_im = re * im * 2.0 + yy;
				if (new_re * new_re + new_im * new_im > 1e3) {
					break;
				}
				re = new_re;
				im = new_im;

				if (iter == 60) {
					break;
				}
			}
			iter = iter * 6;
			int hi = (iter / 60) % 6;
			float f = (float) (iter % 60) / 60.0;
			float p = 0.0;
			float q = 0.5 * (1.0 - f);
			float t = 0.5 * (1.0 - (1.0 - f));
			// printf("here");
			switch (hi) {
			case 0:
				if (depth == 3) {
					*buf = (char) 128;
					buf++;
					*buf = (char) (256 * t);
					buf++;
					*buf = (char) (256 * p);
					buf++;
				} else if (depth == 2) {
					*(uint16_t*) buf =
					    (uint16_t)(((uint16_t) 16 << 11) | ((uint16_t)(32 * t) << 5) | ((uint16_t)(32 * p)));
					buf += 2;
				}
				break;
			case 1:
				/*frameBuffer[3 * (x * height + y)] = (char) (256 * q);*/
				/*frameBuffer[3 * (x * height + y) + 1] = (char) (256 * 0.5);*/
				/*frameBuffer[3 * (x * height + y) + 2] = (char) (256 * p);*/
				if (depth == 3) {
					*buf = (char) (256 * q);
					buf++;
					*buf = (char) (256 * 0.5);
					buf++;
					*buf = (char) (256 * p);
					buf++;
				} else if (depth == 2) {
					*(uint16_t*) buf =
					    (uint16_t)(((uint16_t)(31 * q) << 11) | ((uint16_t)(63 * 0.5) << 5) | ((uint16_t)(31 * p)));
					buf += 2;
				}
				break;
			case 2:
				/*frameBuffer[3 * (x * height + y)] = (char) (256 * p);*/
				/*frameBuffer[3 * (x * height + y) + 1] = (char) (256 * 0.5);*/
				/*frameBuffer[3 * (x * height + y) + 2] = (char) (256 * t);*/
				if (depth == 3) {
					*buf = (char) (256 * p);
					buf++;
					*buf = (char) (256 * 0.5);
					buf++;
					*buf = (char) (256 * t);
					buf++;
				} else if (depth == 2) {
					*(uint16_t*) buf =
					    (uint16_t)(((uint16_t)(31 * p) << 11) | ((uint16_t)(63 * 0.5) << 5) | ((uint16_t)(31 * t)));
					buf += 2;
				}
				break;
			case 3:
				/*frameBuffer[3 * (x * height + y)] = (char) (256 * p);*/
				/*frameBuffer[3 * (x * height + y) + 1] = (char) (256 * q);*/
				/*frameBuffer[3 * (x * height + y) + 2] = (char) (256 * 0.5);*/
				if (depth == 3) {
					*buf = (char) (256 * p);
					buf++;
					*buf = (char) (256 * q);
					buf++;
					*buf = (char) (256 * 0.5);
					buf++;
				} else if (depth == 2) {
					*(uint16_t*) buf =
					    (uint16_t)(((uint16_t)(31 * p) << 11) | ((uint16_t)(63 * q) << 5) | ((uint16_t)(31 * 0.5)));
					buf += 2;
				}
				break;
			case 4:
				/*frameBuffer[3 * (x * height + y)] = (char) (256 * t);*/
				/*frameBuffer[3 * (x * height + y) + 1] = (char) (256 * p);*/
				/*frameBuffer[3 * (x * height + y) + 2] = (char) (256 * 0.5);*/
				if (depth == 3) {
					*buf = (char) (256 * t);
					buf++;
					*buf = (char) (256 * p);
					buf++;
					*buf = (char) (256 * 0.5);
					buf++;
				} else if (depth == 2) {
					*(uint16_t*) buf =
					    (uint16_t)(((uint16_t)(31 * t) << 11) | ((uint16_t)(63 * p) << 5) | ((uint16_t)(31 * 0.5)));
					buf += 2;
				}
				break;
			case 5:
				/*frameBuffer[3 * (x * height + y)] = (char) (256 * 0.5);*/
				/*frameBuffer[3 * (x * height + y) + 1] = (char) (256 * p);*/
				/*frameBuffer[3 * (x * height + y) + 2] = (char) (256 * q);*/
				if (depth == 3) {
					*buf = (char) (256 * 0.5);
					buf++;
					*buf = (char) (256 * p);
					buf++;
					*buf = (char) (256 * q);
					buf++;
				} else if (depth == 2) {
					*(uint16_t*) buf =
					    (uint16_t)(((uint16_t)(31 * 0.5) << 11) | ((uint16_t)(63 * p) << 5) | ((uint16_t)(31 * q)));
					buf += 2;
				}
				break;
			}
		}
	}
	return 0;
}

void flash(int width, int height) {
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			frameBuffer[2 * (j + i * height)] = 0;
			frameBuffer[2 * (j + i * height) + 1] = 31;
			printf("here drawing %d %d \n", i, j);
		}
	}
}

void test1_qemu(unsigned short* srcPtr) {
	char r, g, b;
	char* fb = Buffer;
	const int height = 160;
	const int width = 240;
	const int offset = ((768 - 640) / 2 * 1024 + (1024 - 960) / 2) * 3;
	const int k = 4;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			r = (srcPtr[i * width + j] >> 8) & 0xf8;
			g = (srcPtr[i * width + j] >> 3) & 0xfc;
			b = (srcPtr[i * width + j] << 3) & 0xf8;
			if (fb[j * k * 3 + i * k * 3 * 1024 + offset] == b && fb[j * k * 3 + i * k * 3 * 1024 + 1 + offset] == g &&
			    fb[j * k * 3 + i * k * 3 * 1024 + 2 + offset] == r) {
				continue;
			} else {
				for (int n = 0; n < k; ++n) {
					for (int m = 0; m < k; ++m) {
						fb[(j * k + m) * 3 + (i * k + n) * 3 * 1024 + offset] = b;
						fb[(j * k + m) * 3 + (i * k + n) * 3 * 1024 + 1 + offset] = g;
						fb[(j * k + m) * 3 + (i * k + n) * 3 * 1024 + 2 + offset] = r;
						frameBuffer[(j * k + m) * 3 + (i * k + n) * 3 * 1024 + offset] = b;
						frameBuffer[(j * k + m) * 3 + (i * k + n) * 3 * 1024 + 1 + offset] = g;
						frameBuffer[(j * k + m) * 3 + (i * k + n) * 3 * 1024 + 2 + offset] = r;
					}
				}
			}
		}
	}
}

void pre_test1() { // 把屏幕初始化为白色，以后就无需再管灰度了
	unsigned long long* fb = (unsigned long long*) frameBuffer;
	for (int i = 0; i < 1024 * 768 * 32 / 64; ++i)
		fb[i] = (unsigned long long)(0) - (unsigned long long)(1);
}

void test1(unsigned short* srcPtr) { // for real machine
	char r, g, b;
	char* fb = Buffer;
	const int height = 160;
	const int width = 240;
	const int offset = ((768 - 640) / 2 * 1024 + (1024 - 960) / 2) * 4;
	const int k = 4;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			r = (srcPtr[i * width + j] >> 8) & 0xf8;
			g = (srcPtr[i * width + j] >> 3) & 0xfc;
			b = (srcPtr[i * width + j] << 3) & 0xf8;
			if (fb[j * k * 4 + i * k * 4 * 1024 + offset] == b && fb[j * k * 4 + i * k * 4 * 1024 + 1 + offset] == g &&
			    fb[j * k * 4 + i * k * 4 * 1024 + 2 + offset] == r) {
				continue;
			} else {
				fb[j * k * 4 + i * k * 4 * 1024 + offset] = b;
				fb[j * k * 4 + i * k * 4 * 1024 + 1 + offset] = g;
				fb[j * k * 4 + i * k * 4 * 1024 + 2 + offset] = r;
				for (int n = 0; n < k; ++n) {
					for (int m = 0; m < k; ++m) {
						frameBuffer[(j * k + m) * 4 + (i * k + n) * 4 * 1024 + offset] = b;
						frameBuffer[(j * k + m) * 4 + (i * k + n) * 4 * 1024 + 1 + offset] = g;
						frameBuffer[(j * k + m) * 4 + (i * k + n) * 4 * 1024 + 2 + offset] = r;
					}
				}
			}
		}
	}
}

int multi_times = 0;

void pre_test2(int h, int w) {
	const int height = 160;
	const int width = 240;
	for (int i = 0; h > height && w > width;) {
		++multi_times;
		h -= height;
		w -= width;
	}
}

// h, w 为目标屏幕的分辨率（rgb565）
// 需要先执行 pre_test2(h, w) ，为 multi_times 初始化
void test2(unsigned short* srcPtr, int h, int w) {
	unsigned short* buffer = (unsigned short*) Buffer;
	unsigned short* fb = (unsigned short*) frameBuffer;
	const int height = 160;
	const int width = 240;
	const int offset = (h - height * multi_times) / 2 * w + (w - width * multi_times) / 2;
	const int k = multi_times;
	int idx0 = 0;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			idx0 = i * width + j;
			if (buffer[idx0] != srcPtr[idx0]) {
				buffer[idx0] = srcPtr[idx0];
				for (int m = 0; m < k; ++m) {
					for (int n = 0; n < k; ++n) {
						fb[(j * k + m) + (i * k + n) * w + offset] = buffer[idx0];
						/*if ( videoBuffer[(j * k + m) + (i * k + n) * w + offset] != srcPtr[i * width + j] ) {*/
						/*videoBuffer[(j * k + m) + (i * k + n) * w + offset] = srcPtr[i * width + j];*/
						/*}*/
					}
				}
			}
		}
	}
}

static int _logLevel = 0x0f;
void _log(struct mLogger* log, int category, enum mLogLevel level, const char* format, va_list args) {
	if (level & _logLevel) {
		// Categories are registered at runtime, but the name can be found
		// through a simple lookup.
		printf("%s: ", mLogCategoryName(category));

		// We get a format string and a varargs context from the core, so we
		// need to use the v* version of printk.
		vprintf(format, args);

		// The format strings do NOT include a newline, so we need to
		// append it ourself.
		printf("\r\n");
	}
}

uint16_t translateKey(int ch) {
	switch (ch) {
	case 'w':
		return 1 << 6;
	case 's':
		return 1 << 7;
	case 'a':
		return 1 << 5;
	case 'd':
		return 1 << 4;
	case 'z':
		return 1 << 0;
	case 'x':
		return 1 << 1;
	case 'q':
		return 1 << 9;
	case 'e':
		return 1 << 8;
	case '1':
		return 1 << 2;
	case '2':
		return 1 << 3;
	}
	printf("the ch %s is useless", (char) ch);
	return 0;
}

void mgbaMainLoop() {
	struct mCoreOptions opts = {
		.useBios = false,
		.volume = 0x040,
	};
	static struct mLogger logger = { .log = _log };
	mLogSetDefaultLogger(&logger);
	struct VFile* romFile = VFileOpen("boot.gba", O_RDONLY);
	struct mCore* core = GBACoreCreate();
	core->init(core);
	core->setVideoBuffer(core, (color_t*) videoBuffer, 240);
	mCoreConfigInit(&core->config, "rCore");
	mCoreConfigLoad(&core->config);
	mCoreConfigSetDefaultValue(&core->config, "idleOptimization", "detect");
	opts.frameskip = 0;
	mCoreConfigLoadDefaults(&core->config, &opts);
	mCoreLoadConfig(core);

	core->loadROM(core, romFile);
	core->reset(core);

	int framecount = 0;
	uint16_t keyState = 0;
	char c;
	int counter = 10;
	while (1) {
		while (read(STDIN_FILENO, (void*) &c, 1) > 0) {
			keyState |= translateKey(c);
		}
		core->setKeys(core, keyState);
		core->runFrame(core);
		test1_qemu(videoBuffer);
	}
}

int main() {
	struct fb_var_screeninfo vinfo;
	int fbfd = open("/dev/fb0", O_WRONLY);
	if (!fbfd) {
		printf("Error: cannot open framebuffer device.\n");
		exit(1);
	}

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable information.\n");
		exit(3);
	}

	fb_depth = vinfo.bits_per_pixel;
	fb_width = vinfo.xres;
	fb_height = vinfo.yres;

	videoBuffer = malloc(320 * 240 * 2);
	Buffer = malloc(fb_width * fb_height * fb_depth / 8);
	printf("hello world???\n");
	printf("%d, %d, %d", fb_width, fb_height, fb_depth);

	frameBuffer =
	    (char*) mmap((void*) 0xfd000000, fb_width * fb_height * fb_depth / 8, PROT_WRITE, MAP_SHARED, fbfd, 0);
	close(fbfd);
	// for(int i = 0; i < 1024 * 768 * 3; i ++){
	// 	frameBuffer[i] = 255;
	// }
	// plotMandelbrot(fb_width, fb_height, fb_depth / 8);
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	mgbaMainLoop();
	return 0;
}
