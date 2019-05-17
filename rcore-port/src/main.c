#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/feature/commandline.h>
#include <mgba/gba/core.h>

#include <mgba-util/memory.h>
#include <mgba-util/string.h>
#include <mgba-util/vfs.h>
#include <mgba/internal/gba/gba.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* videoBuffer = NULL;
char* frameBuffer = NULL;

int plotMandelbrot(FILE* fd) {
	float scale = 5e-3;
	int width = 1024;
	int heigth = 768;
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < heigth; y++) {
			float xx = ((float) x - width / 2) * scale;
			float yy = ((float) y - heigth / 2) * scale;
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
			switch (hi) {
			case 0:
				videoBuffer[3 * (x * 768 + y)] = (char) 128;
				videoBuffer[3 * (x * 768 + y) + 1] = (char) (256 * t);
				videoBuffer[3 * (x * 768 + y) + 2] = (char) (256 * p);
				break;
			case 1:
				videoBuffer[3 * (x * 768 + y)] = (char) (256 * q);
				videoBuffer[3 * (x * 768 + y) + 1] = (char) (256 * 0.5);
				videoBuffer[3 * (x * 768 + y) + 2] = (char) (256 * p);
				break;
			case 2:
				videoBuffer[3 * (x * 768 + y)] = (char) (256 * p);
				videoBuffer[3 * (x * 768 + y) + 1] = (char) (256 * 0.5);
				videoBuffer[3 * (x * 768 + y) + 2] = (char) (256 * t);
				break;
			case 3:
				videoBuffer[3 * (x * 768 + y)] = (char) (256 * p);
				videoBuffer[3 * (x * 768 + y) + 1] = (char) (256 * q);
				videoBuffer[3 * (x * 768 + y) + 2] = (char) (256 * 0.5);
				break;
			case 4:
				videoBuffer[3 * (x * 768 + y)] = (char) (256 * t);
				videoBuffer[3 * (x * 768 + y) + 1] = (char) (256 * p);
				videoBuffer[3 * (x * 768 + y) + 2] = (char) (256 * 0.5);
				break;
			case 5:
				videoBuffer[3 * (x * 768 + y)] = (char) (256 * 0.5);
				videoBuffer[3 * (x * 768 + y) + 1] = (char) (256 * p);
				videoBuffer[3 * (x * 768 + y) + 2] = (char) (256 * q);
				break;
			}
		}
	}
	if (fwrite((void*) videoBuffer, 1024 * 768 * 3, 1, fd) < 0) {
		printf("mgba ERROR!!!");
		return -1;
	}
	return 0;
}

void test1(unsigned short* srcPtr, FILE* file) {
	char r, g, b;
	char* fb = frameBuffer;
	const int height = 160;
	const int width = 240;
	// const int offset = ((1024 - 960) / 2 * 768 + (768 - 640) / 2) * 3;
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
					}
					fseek(file, (i * k + n) * 3 * 1024 + offset, SEEK_SET);
					fwrite((void*) frameBuffer, 4 * 3, 1, file);
				}
			}
		}
	}
}

void test(unsigned short* srcPtr) {
	char r, g, b;
	char* fb = frameBuffer;
	const int height = 160;
	const int width = 240;
	// const int offset = ((1024 - 960) / 2 * 768 + (768 - 640) / 2) * 3;
	const int offset = ((768 - 640) / 2 * 1024 + (1024 - 960) / 2) * 3;
	const int k = 4;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			r = (srcPtr[i * width + j] >> 8) & 0xf8;
			g = (srcPtr[i * width + j] >> 3) & 0xfc;
			b = (srcPtr[i * width + j] << 3) & 0xf8;
			for (int m = 0; m < k; ++m) {
				for (int n = 0; n < k; ++n) {
					fb[(j * k + m) * 3 + (i * k + n) * 3 * 1024 + offset] = b;
					fb[(j * k + m) * 3 + (i * k + n) * 3 * 1024 + 1 + offset] = g;
					fb[(j * k + m) * 3 + (i * k + n) * 3 * 1024 + 2 + offset] = r;
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

void mgbaMainLoop(FILE* fd) {
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
	while (1) {
		core->runFrame(core);
		//++framecount;
		// if (framecount == 3) {
		// framecount = 0;
		test((unsigned short*) videoBuffer);
		if (fwrite((void*) frameBuffer, 1024 * 768 * 3, 1, fd) < 0) {
			printf("mgba ERROR!!!");
		}
		//}
	}
}

int main() {
	videoBuffer = malloc(1024 * 768 * 3);
	frameBuffer = malloc(1024 * 768 * 3);
	printf("hello world???\n");
	putc(getc(stdin), stdout); // test keyboard, still don't know whether getc() is enough

	FILE* fp = NULL;
	fp = fopen("/dev/fb0", "r+");
	/*plotMandelbrot(fp);*/
	mgbaMainLoop(fp);
	fclose(fp);
	return 0;
}
