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

const int dstPtrLen = 1024 * 768 * 3;
char* frameBuffer;
void blitFromGbaToLcd1024x768Upscaled(uint64_t* srcPtr) {
	const unsigned int upscaledWidth = 240 * 4; // 960
	const unsigned int upscaledHeight = 160 * 4; // 640
	const unsigned int offX = (1024 - upscaledWidth) / 2;
	const unsigned int offY = (768 - upscaledHeight) / 2;
	int fbStart = offY * 1024 * 3;
	int fbEnd = (768 - offY) * 1024 * 3;

	for (int i = 0; i < fbStart; ++i) // filed y black
		frameBuffer[i] = 0;
	for (int i = fbEnd; i < dstPtrLen; ++i) // filed y black
		frameBuffer[i] = 0;

	char r = 0, g = 0, b = 0;
	unsigned int pix[4];
	const int lineLen = 1024 * 3;

	for (int i = 0; i < 160; ++i) {
		char linePixs[lineLen];
		int linePixPos = 0;

		for (; linePixPos < 3 * offX; ++linePixPos) { // filed x black
			linePixs[linePixPos] = 0;
		}
		for (int j = 0; j < 240 / 4; ++j) {
			pix[0] = (*srcPtr >> 48);
			pix[1] = (*srcPtr >> 32) & (0xff);
			pix[2] = (*srcPtr >> 16) & (0xff);
			pix[3] = *srcPtr & (0xff);
			++srcPtr;

			for (int k = 0; k < 4; ++k) { // rgb565
				r = pix[k] >> 11;
				g = (pix[k] >> 5) & 0x3f;
				b = pix[k] & 0x1f;
				linePixs[linePixPos++] = b;
				linePixs[linePixPos++] = g;
				linePixs[linePixPos++] = r;

				linePixs[linePixPos++] = b;
				linePixs[linePixPos++] = g;
				linePixs[linePixPos++] = r;

				linePixs[linePixPos++] = b;
				linePixs[linePixPos++] = g;
				linePixs[linePixPos++] = r;

				linePixs[linePixPos++] = b;
				linePixs[linePixPos++] = g;
				linePixs[linePixPos++] = r;
			}
		}
		for (; linePixPos < lineLen; ++linePixPos) { // filed x black
			linePixs[linePixPos] = 0;
		}
		for (int j = 0; j < 4; ++j) {
			memcpy(frameBuffer, linePixs, lineLen);
			frameBuffer += lineLen;
		}
	}
}

void test(unsigned int* srcPtr) {
	char r, g, b;
	char* fb = frameBuffer;
	const int height = 160;
	const int width = 240;
	const int offset = ((1024 - 960) / 2 * 768 + (768 - 640) / 2) * 3;
	const int k = 4;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width / 2; ++j) {
			r = (srcPtr[i * width / 2 + j] >> 24) & 0xf8;
			g = (srcPtr[i * width / 2 + j] >> 19) & 0xfc;
			b = (srcPtr[i * width / 2 + j] >> 13) & 0xf8;
			for (int m = 0; m < k; ++m) {
				for (int n = 0; n < k; ++n) {
					fb[(2 * (j * k + m) + 1) * 768 * 3 + (i * k + n) * 3 + offset] = r;
					fb[(2 * (j * k + m) + 1) * 768 * 3 + (i * k + n) * 3 + 1 + offset] = g;
					fb[(2 * (j * k + m) + 1) * 768 * 3 + (i * k + n) * 3 + 2 + offset] = b;
				}
			}

			r = (srcPtr[i * width / 2 + j] >> 8) & 0xf8;
			g = (srcPtr[i * width / 2 + j] >> 3) & 0xfc;
			b = (srcPtr[i * width / 2 + j] << 3) & 0xf8;
			for (int m = 0; m < k; ++m) {
				for (int n = 0; n < k; ++n) {
					fb[(2 * (j * k + m)) * 768 * 3 + (i * k + n) * 3 + offset] = r;
					fb[(2 * (j * k + m)) * 768 * 3 + (i * k + n) * 3 + 1 + offset] = g;
					fb[(2 * (j * k + m)) * 768 * 3 + (i * k + n) * 3 + 2 + offset] = b;
				}
			}
			// fb[2 * j * 768 * 3 + i * 3 + offset] = r;
			// fb[2 * j * 768 * 3 + i * 3 + 1 + offset] = g;
			// fb[2 * j * 768 * 3 + i * 3 + 2 + offset] = b;
		}
	}
}
/*
void test(char* srcPtr) {
    char r, g, b;
    char* fb = frameBuffer;
    for (int i = 0; i < 160; ++i) {
        for (int j = 0; j < 240; ++j) {
            r = (srcPtr[i * 240 * 2 + j * 2] & 0xf8);
            g = ((srcPtr[i * 240 * 2 + j * 2] << 5) & 0xe0) | ((srcPtr[i * 240 * 2 + j * 2 + 1] >> 3) & 0x1c);
            b = (srcPtr[i * 240 * 2 + j * 2 + 1] << 3) & 0xf8;
            fb[j * 768 * 3 + i * 3] = r;
            fb[j * 768 * 3 + i * 3 + 1] = g;
            fb[j * 768 * 3 + i * 3 + 2] = b;
        }
    }
}
*/
void mgbaMainLoop(FILE* fd) {
	struct mCoreOptions opts = {
		.useBios = false,
		.volume = 0x040,
	};

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
		framecount++;
		/*if (framecount % 10 == 0) {*/
		// TODO
		test(videoBuffer);
		if (fwrite((void*) frameBuffer, 1024 * 768 * 3, 1, fd) < 0) {
			printf("mgba ERROR!!!");
		}
		/*}*/
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
