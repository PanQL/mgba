#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/feature/commandline.h>
#include <mgba/gba/core.h>

#include <mgba-util/memory.h>
#include <mgba-util/string.h>
#include <mgba-util/vfs.h>
#include <mgba/internal/gba/gba.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

char* videoBuffer = NULL;
char* frameBuffer = NULL;
char* Buffer = NULL;

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

void test1(unsigned short* srcPtr) {
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

int multi_times = 0;

void pre_test2(int h, int w) {
	const int height = 160;
	const int width = 240;
	for (int i = 0; h > height && w > width) {
		++multi_times;
		h -= height;
		w -= width;
	}
}

// h, w 为目标屏幕的分辨率（rgb565）
// 需要先执行 pre_test2(h, w) ，为 multi_times 初始化
void test2(unsigned short* srcPtr, int h, int w) {
	unsigned short* fb = (unsigned short*) frameBuffer;
	const int height = 160;
	const int width = 240;
	const int offset = (h - height * multi_times) / 2 * w + (w - width * multi_times) / 2;
	const int k = multi_times;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			for (int m = 0; m < k; ++m) {
				for (int n = 0; n < k; ++n) {
					fb[(j * k + m) + (i * k + n) * 1024 + offset] = srcPtr[i * width + j];
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
	while (1) {
		while (read(STDIN_FILENO, (void*) &c, 1) > 0) {
			printf("%d \n", (int) c);
			keyState = translateKey((int) c);
			core->setKeys(core, keyState);
			keyState = 0;
		}
		core->runFrame(core);
		test1((unsigned short*) videoBuffer);
	}
}

int main() {
	videoBuffer = malloc(320 * 240 * 2);
	Buffer = malloc(1024 * 768 * 3);
	printf("hello world???\n");

	int fd = open("/dev/fb0", O_WRONLY);
	frameBuffer = (char*) mmap((void*) 0xf0000000, 1024 * 768 * 3, PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	mgbaMainLoop();
	return 0;
}
