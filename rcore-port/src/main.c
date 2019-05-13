#include <k9.h>

#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/feature/commandline.h>
#include <mgba/gba/core.h>

#include <mgba-util/memory.h>
#include <mgba-util/string.h>
#include <mgba-util/vfs.h>
#include <mgba/internal/gba/gba.h>

struct VFile* VFileOpenK210FatFs(const char* path, u8 mode);

#define TRACE_1 printk

FATFS sdFatFs;

static int _logLevel = 0x0f;

#define AUDIO_SAMPLE_LEN (2048)

struct mCore* gbaCore;
struct mAVStream avStream;

uint64_t videoOutputBuffer[240 * 160 / 4];
u64 lcdWorkBuffer[LCD_Y_MAX * LCD_X_MAX / 4];
int16_t audioBuffer[2][AUDIO_SAMPLE_LEN * 2];
volatile int currentAudioBuffer = 0;
volatile int audioBufferCount = 0;
int isPS2Connected = 0;

static int on_irq_dma3(void* ctx) {
	if (audioBufferCount > 0) {
		audioBufferCount--;
		currentAudioBuffer = !currentAudioBuffer;
	}
	int16_t* samples = audioBuffer[currentAudioBuffer];
	i2s_play(I2S_DEVICE_0, DMAC_CHANNEL3, samples, sizeof(audioBuffer[0]), sizeof(audioBuffer[0]), 16, 2);
	return 0;
}

int initSDCard() {
	int ret = 0;
	for (int i = 0; i < 3; i++) {
		ret = sd_init();
		if (ret == 0) {
			break;
		}
		printk("sdcard init failed, retry...\r\n");
		msleep(500);
	}
	if (ret != 0) {
		printk("sdcard init failed!\r\n");
		return -1;
	}
	uint8_t buf[512] = { 0 };
	ret = sd_read_sector(buf, 0, 1);
	if (ret != 0) {
		printk("sdcard test read sector failed!\r\n");
		return -2;
	}
	for (int i = 0; i < 512; i++) {
		printk("%02x ", buf[i]);
	}
	return 0;
}

int initAudio() {
	i2s_init(I2S_DEVICE_0, I2S_TRANSMITTER, 0x03); // mask of ch
	i2s_tx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_0, RESOLUTION_16_BIT, SCLK_CYCLES_32,
	                      /*TRIGGER_LEVEL_1*/ TRIGGER_LEVEL_4, RIGHT_JUSTIFYING_MODE);
	printk("Real sample rate: %d\n", (int) i2s_set_sample_rate(I2S_DEVICE_0, 32768));

	dmac_set_irq(DMAC_CHANNEL3, on_irq_dma3, NULL, 1);
	on_irq_dma3(0);
	return 0;
}

static void _postAudioBuffer(struct mAVStream* stream, blip_t* left, blip_t* right) {
	while (audioBufferCount >= 2) {
	}
	sysctl_disable_irq();
	int16_t* samples = audioBuffer[!currentAudioBuffer];
	blip_read_samples(left, samples, AUDIO_SAMPLE_LEN, true);
	blip_read_samples(right, samples + 1, AUDIO_SAMPLE_LEN, true);
	audioBufferCount++;
	sysctl_enable_irq();
}

int mountFatFs() {
	FRESULT fret = f_mount(&sdFatFs, "", 1);
	if (fret == FR_OK) {
		printk("mount fat partition ok!\r\n");
		return 0;
	} else {
		printk("mount fat partition failed: %d\n", fret);
	}
	return -1;
}

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

void initMgba() {
	// Set up a logger. The default logger prints everything to STDOUT, which is not usually desirable.
	static struct mLogger logger = { .log = _log };
	mLogSetDefaultLogger(&logger);
	gbaCore = GBACoreCreate();
	gbaCore->init(gbaCore);
	gbaCore->setAudioBufferSize(gbaCore, AUDIO_SAMPLE_LEN);

	avStream.postAudioBuffer = _postAudioBuffer;
	gbaCore->setAVStream(gbaCore, &avStream);

	blip_set_rates(gbaCore->getAudioChannel(gbaCore, 0), gbaCore->frequency(gbaCore), 32768);
	blip_set_rates(gbaCore->getAudioChannel(gbaCore, 1), gbaCore->frequency(gbaCore), 32768);
}

void blitFromGbaToLcd320240P2P(u64* srcPtr) {
	u64* dstPtr = lcdWorkBuffer;
	const u32 upscaledWidth = 240;
	const u32 upscaledHeight = 160;
	const u32 offX = (320 - upscaledWidth) / 2;
	const u32 offY = (240 - upscaledHeight) / 2;

	lcd_set_area(offX, offY, offX + upscaledWidth - 1, offY + upscaledHeight - 1);
	for (u32 i = 0; i < 240 * 160 / 4; i++) {
		u64 src4Pix = *srcPtr;
		srcPtr++;
		u16 pix0 = (u16) src4Pix;
		u16 pix1 = (u16)(src4Pix >> 16);
		u16 pix2 = (u16)(src4Pix >> 32);
		u16 pix3 = (u16)(src4Pix >> 48);
		*dstPtr = ((u64) pix0 << 16) | ((u64) pix1) | ((u64) pix2 << 48) | ((u64) pix3 << 32);
		dstPtr++;
	}
	tft_write_word(lcdWorkBuffer, upscaledWidth * upscaledHeight / 2);
}

void blitFromGbaToLcd320240Upscaled(u64* srcPtr) {
	u64* dstPtr = lcdWorkBuffer;
	const u32 upscaledWidth = 240 + (240 / 3); // 320
	const u32 upscaledHeight = 160 + (160 / 3); // 213
	const u32 offX = (320 - upscaledWidth) / 2;
	const u32 offY = (240 - upscaledHeight) / 2;

	lcd_set_area(offX, offY, offX + upscaledWidth - 1, offY + upscaledHeight - 1);
	for (u32 y = 0; y < 160; y++) {
		for (u32 x = 0; x < 240; x += 12) {
			u64 src4Pix = *srcPtr;
			srcPtr++;
			u16 pix0 = (u16) src4Pix;
			u16 pix1 = (u16)(src4Pix >> 16);
			u16 pix2 = (u16)(src4Pix >> 32);
			u16 pix3 = (u16)(src4Pix >> 48);

			src4Pix = *srcPtr;
			srcPtr++;
			u16 pix4 = (u16) src4Pix;
			u16 pix5 = (u16)(src4Pix >> 16);
			u16 pix6 = (u16)(src4Pix >> 32);
			u16 pix7 = (u16)(src4Pix >> 48);

			src4Pix = *srcPtr;
			srcPtr++;
			u16 pix8 = (u16) src4Pix;
			u16 pix9 = (u16)(src4Pix >> 16);
			u16 pix10 = (u16)(src4Pix >> 32);
			u16 pix11 = (u16)(src4Pix >> 48);

			*dstPtr = ((u64) pix0 << 16) | ((u64) pix1) | ((u64) pix2 << 48) | ((u64) pix2 << 32);
			dstPtr++;
			*dstPtr = ((u64) pix3 << 16) | ((u64) pix4) | ((u64) pix5 << 48) | ((u64) pix5 << 32);
			dstPtr++;
			*dstPtr = ((u64) pix6 << 16) | ((u64) pix7) | ((u64) pix8 << 48) | ((u64) pix8 << 32);
			dstPtr++;
			*dstPtr = ((u64) pix9 << 16) | ((u64) pix10) | ((u64) pix11 << 48) | ((u64) pix11 << 32);
			dstPtr++;
		}
		if (y % 3 == 2) {
			memcpy(dstPtr, dstPtr - (upscaledWidth / 4), upscaledWidth * 2);
			dstPtr += (upscaledWidth / 4);
		}
	}
	tft_write_word(lcdWorkBuffer, upscaledWidth * upscaledHeight / 2);
}

const int dstPtrLen = 1024 * 768 * 3;
char* frameBuffer = new char[dstPtrLen];
void blitFromGbaToLcd1024x768Upscaled(uint64_t* srcPtr) {
	const unsigned int upscaledWidth = 240 * 4; // 960
	const unsigned int upscaledHeight = 160 * 4; // 640
	const unsigned int offX = (1024 - upscaledWidth) / 2;
	const unsigned int offY = (768 - upscaledHeight) / 2;
	int fbStart = offY * 1024 * 3;
	int fbEnd = (1024 - offY) * 1024 * 3;

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
		for (int j = 0; j < 240; ++j) {
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
			memcpy(linePixs, frameBuffer, lineLen);
			frameBuffer += lineLen;
		}
	}
	FILE* fp = NULL;

	fp = fopen("/dev/fb0", "r+");
	if (fwrite((void*) frameBuffer, 1024 * 768 * 3, 1, fp) < 0) {
		printf("mgba ERROR!!!");
	}
	fclose(fp);
}

u16 translateKey(int ch) {
	/* enum GBAKey {
	    GBA_KEY_A = 0,
	    GBA_KEY_B = 1,
	    GBA_KEY_SELECT = 2,
	    GBA_KEY_START = 3,
	    GBA_KEY_RIGHT = 4,
	    GBA_KEY_LEFT = 5,
	    GBA_KEY_UP = 6,
	    GBA_KEY_DOWN = 7,
	    GBA_KEY_R = 8,
	    GBA_KEY_L = 9,
	    GBA_KEY_MAX,
	    GBA_KEY_NONE = -1
	};
	    */

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
	return 0;
}

void mgbaMainLoop() {
	struct mCoreOptions opts = {
		.useBios = true,
		.volume = 0x040,
	};
	const uint32_t desiredFps = 60;
	const uint64_t usPerFrame = 1000000UL / desiredFps;
	initMgba();

	struct VFile* romFile = VFileOpenK210FatFs("boot.gba", FA_READ);
	// struct VFile* savFile = VFileOpenK210FatFs("boot.sav", FA_WRITE | FA_OPEN_ALWAYS);

	struct mCore* core = gbaCore;
	core->setVideoBuffer(core, (color_t*) videoOutputBuffer, 240);
	mCoreConfigInit(&core->config, "K210");
	mCoreConfigLoad(&core->config);
	mCoreConfigSetDefaultValue(&core->config, "idleOptimization", "detect");
	opts.frameskip = 0;
	mCoreConfigLoadDefaults(&core->config, &opts);
	mCoreLoadConfig(core);

	core->loadROM(core, romFile);
	// core->loadSave(core, savFile);
	core->reset(core);
	printk("core reset done.\r\n");

	int framecount = 0;
	u16 keyState = 0;
	int ch;
	uint64_t prevCycle100Frame = read_cycle();
	while (1) {
		while ((ch = uarths_getc()) != -1) {
			// printk("received: %d\n", ch);
			keyState |= translateKey(ch);
		}
		if (isPS2Connected) {
			if (PS2X_read_gamepad(0, 0)) {
				keyState = 0;
				static const int keyMap[10] = {
					PSB_CIRCLE, // GBA_KEY_A = 0,
					PSB_CROSS, // GBA_KEY_B = 1,
					PSB_SELECT, // GBA_KEY_SELECT = 2,
					PSB_START, // GBA_KEY_START = 3,
					PSB_PAD_RIGHT, // GBA_KEY_RIGHT = 4,
					PSB_PAD_LEFT, // GBA_KEY_LEFT = 5,
					PSB_PAD_UP, // GBA_KEY_UP = 6,
					PSB_PAD_DOWN, // GBA_KEY_DOWN = 7,
					PSB_R1, // GBA_KEY_R = 8,
					PSB_L1, // GBA_KEY_L = 9,
				};
				for (int i = 0; i < 10; i++) {
					keyState <<= 1;
					keyState |= PS2X_Button(keyMap[9 - i]);
				}
			} else {
				printk("read ps2 failed!\n");
			}
		}
		core->setKeys(core, keyState);
		keyState = 0;
		core->runFrame(core);
		framecount++;
		if (framecount % 100 == 0) {
			uint64_t usPassed100Frame =
			    (read_cycle() - prevCycle100Frame) / (sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / 1000000UL);
			printk("fps: %d\r\n", (int) (1000000 / ((double) usPassed100Frame / 100)));
			prevCycle100Frame = read_cycle();
		}
		// blitFromGbaToLcd320240P2P(videoOutputBuffer);
		blitFromGbaToLcd1024x768Upscaled(videoOutputBuffer);
		ceUpdateBlockAge();
	}
}

int main() {
	sysctl_pll_set_freq(SYSCTL_CLOCK_PLL0, 1100000000UL);
	sysctl_pll_set_freq(SYSCTL_PLL1, 400000000UL);
	sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
	uarths_init();
	setupHardware();

	dmac_init();
	plic_init();
	sysctl_enable_irq();

	initAudio();
	PS2X_confg_io(GPIOHS_PS2_CS, GPIOHS_PS2_CLK, GPIOHS_PS2_MOSI, GPIOHS_PS2_MISO);
	printk("hello world!\r\n");
	lcd_init(15000000);
	lcd_clear(BLACK);
	if (initSDCard() != 0) {
		return 0;
	}
	if (mountFatFs() != 0) {
		return 0;
	}

	ceSetupMMU();
	for (int i = 0; i < 10; i++) {
		msleep(200);
		if (PS2X_read_gamepad(0, 0)) {
			isPS2Connected = 1;
			break;
		}
	}
	printf("isPS2Connected: %d\n", PS2X_read_gamepad(0, 0));
	mgbaMainLoop();
	delete frameBuffer[];
	return 0;
}

void* anonymousMemoryMap(size_t size) {
	TRACE_1("anonymousMemoryMap: %p, %d\r\n", __builtin_return_address(0), (int) size);
	void* ptr = calloc(1, size);
	assert(ptr);
	return ptr;
}

void mappedMemoryFree(void* memory, size_t size) {
	free(memory);
}

/*
int main(){
    printf("hello world\n");
    char ch;
    // while(1){
    // 	ch = getc(stdin);
    // 	printf("\n a char entered \n");
    // 	putc(ch, stdout);
    // }
    char* buffer = (char *)malloc(1024*768*3);

    float scale = 5e-3;
    int width = 1024;
    int heigth = 768;
    for(int x = 0; x < width; x ++){
        for(int y = 0; y < heigth; y ++){
            float xx = ((float)x - width / 2) * scale;
            float yy = ((float)y - heigth / 2) * scale;
            float re = xx;
            float im = yy;
            int iter = 0;
            while(1) {
                iter += 1;
                float new_re = re*re - im*im + re;
                float new_im = re*im*2.0 + yy;
                if (new_re * new_re + new_im * new_im > 1e3){
                    break;
                }
                re = new_re;
                im = new_im;

                if(iter == 60){
                    break;
                }
            }
            iter = iter * 6;
            int hi = (iter / 60) % 6;
            float f = (float)(iter %60) / 60.0;
            float p = 0.0;
            float q = 0.5 * ( 1.0 - f );
            float t = 0.5 * ( 1.0 - (1.0 - f));
            switch(hi){
                case 0 :
                    buffer[3*(x * 768 + y)] = (char)128;
                    buffer[3*(x * 768 + y) + 1] = (char)(256 * t);
                    buffer[3*(x * 768 + y) + 2] = (char)(256 * p);
                    break;
                case 1 :
                    buffer[3*(x * 768 + y)] = (char)(256 * q);
                    buffer[3*(x * 768 + y) + 1] = (char)(256 * 0.5);
                    buffer[3*(x * 768 + y) + 2] = (char)(256 * p);
                    break;
                case 2 :
                    buffer[3*(x * 768 + y)] = (char)(256 * p);
                    buffer[3*(x * 768 + y) + 1] = (char)(256 * 0.5);
                    buffer[3*(x * 768 + y) + 2] = (char)(256 * t);
                    break;
                case 3 :
                    buffer[3*(x * 768 + y)] = (char)(256 * p);
                    buffer[3*(x * 768 + y) + 1] = (char)(256 * q);
                    buffer[3*(x * 768 + y) + 2] = (char)(256 * 0.5);
                    break;
                case 4 :
                    buffer[3*(x * 768 + y)] = (char)(256 * t);
                    buffer[3*(x * 768 + y) + 1] = (char)(256 * p);
                    buffer[3*(x * 768 + y) + 2] = (char)(256 * 0.5);
                    break;
                case 5 :
                    buffer[3*(x * 768 + y)] = (char)(256 * 0.5);
                    buffer[3*(x * 768 + y) + 1] = (char)(256 * p);
                    buffer[3*(x * 768 + y) + 2] = (char)(256 * q);
                    break;
            }
        }
    }


    FILE *fp = NULL;

    fp = fopen("/dev/fb0", "r+");
    // fprintf(fp, "This is testing for fprintf...\n");
    // fputs("This is testing for fputs...\n", fp);
    if(fwrite((void *)buffer, 1024*768*3, 1, fp)<0){
        printf("mgba ERROR!!!");
    }
    fclose(fp);
    return 0;
}
*/