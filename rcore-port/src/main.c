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
					videoBuffer[3*(x * 768 + y)] = (char)128;
					videoBuffer[3*(x * 768 + y) + 1] = (char)(256 * t);
					videoBuffer[3*(x * 768 + y) + 2] = (char)(256 * p);
					break;
				case 1 :
					videoBuffer[3*(x * 768 + y)] = (char)(256 * q);
					videoBuffer[3*(x * 768 + y) + 1] = (char)(256 * 0.5);
					videoBuffer[3*(x * 768 + y) + 2] = (char)(256 * p);
					break;
				case 2 :
					videoBuffer[3*(x * 768 + y)] = (char)(256 * p);
					videoBuffer[3*(x * 768 + y) + 1] = (char)(256 * 0.5);
					videoBuffer[3*(x * 768 + y) + 2] = (char)(256 * t);
					break;
				case 3 :
					videoBuffer[3*(x * 768 + y)] = (char)(256 * p);
					videoBuffer[3*(x * 768 + y) + 1] = (char)(256 * q);
					videoBuffer[3*(x * 768 + y) + 2] = (char)(256 * 0.5);
					break;
				case 4 :
					videoBuffer[3*(x * 768 + y)] = (char)(256 * t);
					videoBuffer[3*(x * 768 + y) + 1] = (char)(256 * p);
					videoBuffer[3*(x * 768 + y) + 2] = (char)(256 * 0.5);
					break;
				case 5 :
					videoBuffer[3*(x * 768 + y)] = (char)(256 * 0.5);
					videoBuffer[3*(x * 768 + y) + 1] = (char)(256 * p);
					videoBuffer[3*(x * 768 + y) + 2] = (char)(256 * q);
					break;
			}
		}
	}
	if(fwrite((void *)videoBuffer, 1024*768*3, 1, fd)<0){
		printf("mgba ERROR!!!");
		return -1;
	}
	return 0;
}

void mgbaMainLoop(FILE* fd) {
	struct mCoreOptions opts = {
		.useBios = false,
		.volume = 0x040,
	};

	struct mCore* core = GBACoreCreate();
	core->init(core);
	core->setVideoBuffer(core, (color_t*) videoBuffer, 768);
	mCoreConfigInit(&core->config, "rCore");
	mCoreConfigLoad(&core->config);
	mCoreConfigSetDefaultValue(&core->config, "idleOptimization", "detect");
	opts.frameskip = 0;
	mCoreConfigLoadDefaults(&core->config, &opts);
	mCoreLoadConfig(core);

	core->reset(core);

	int framecount = 0;
	while(1) {
		core->runFrame(core);
		framecount++;
		if (framecount % 100 == 0) {
			// TODO
			if(fwrite((void *)videoBuffer, 1024*768*3, 1, fd)<0){
				printf("mgba ERROR!!!");
			}
		}
	}
}

int main(){
	videoBuffer = malloc(1024*768*3);
	printf("hello world\n");
	putc(getc(stdin),stdout);	// test keyboard, still don't know whether getc() is enough

	FILE *fp = NULL;
	fp = fopen("/dev/fb0", "r+");
	/*plotMandelbrot(fp);*/
	mgbaMainLoop(fp);
	fclose(fp);
	return 0;
}

