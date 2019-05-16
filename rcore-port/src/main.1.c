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

int main() {
	printf("fuck world\n");
	char ch;
	/*while(1){*/
	/*ch = getc(stdin);*/
	/*printf("\n a char entered \n");*/
	/*putc(ch, stdout);*/
	/*}*/

	float scale = 5e-3;
	int width = 240;
	int height = 160;
	char* buffer = (char*) malloc(1024 * 768 * 3);
	frameBuffer = (char*) malloc(1024 * 768 * 3);
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
			switch (hi) {
			case 0:
				buffer[3 * (x * height + y)] = (char) 128;
				buffer[3 * (x * height + y) + 1] = (char) (256 * t);
				buffer[3 * (x * height + y) + 2] = (char) (256 * p);
				break;
			case 1:
				buffer[3 * (x * height + y)] = (char) (256 * q);
				buffer[3 * (x * height + y) + 1] = (char) (256 * 0.5);
				buffer[3 * (x * height + y) + 2] = (char) (256 * p);
				break;
			case 2:
				buffer[3 * (x * height + y)] = (char) (256 * p);
				buffer[3 * (x * height + y) + 1] = (char) (256 * 0.5);
				buffer[3 * (x * height + y) + 2] = (char) (256 * t);
				break;
			case 3:
				buffer[3 * (x * height + y)] = (char) (256 * p);
				buffer[3 * (x * height + y) + 1] = (char) (256 * q);
				buffer[3 * (x * height + y) + 2] = (char) (256 * 0.5);
				break;
			case 4:
				buffer[3 * (x * height + y)] = (char) (256 * t);
				buffer[3 * (x * height + y) + 1] = (char) (256 * p);
				buffer[3 * (x * height + y) + 2] = (char) (256 * 0.5);
				break;
			case 5:
				buffer[3 * (x * height + y)] = (char) (256 * 0.5);
				buffer[3 * (x * height + y) + 1] = (char) (256 * p);
				buffer[3 * (x * height + y) + 2] = (char) (256 * q);
				break;
			}
		}
	}

	FILE* fp = NULL;

	printf("upscaled\n");
	blitFromGbaToLcd1024x768Upscaled(buffer);
	printf("end upscaled\n");

	fp = fopen("/dev/fb0", "r+");
	/*fprintf(fp, "This is testing for fprintf...\n");*/
	/*fputs("This is testing for fputs...\n", fp);*/
	if (fwrite((void*) frameBuffer, 1024 * 768 * 3, 1, fp) < 0) {
		printf("mgba ERROR!!!");
	}
	fclose(fp);
	return 0;
}
