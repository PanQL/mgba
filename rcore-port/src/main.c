#include <mgba/core/core.h>
#include <mgba/gba/core.h>
#include <mgba/feature/commandline.h>
#include <mgba/core/blip_buf.h>

#include <mgba-util/memory.h>
#include <mgba-util/string.h>
#include <mgba-util/vfs.h>
#include <mgba/internal/gba/gba.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
	printf("hello world\n");
	char ch;
	/*while(1){*/
		/*ch = getc(stdin);*/
		/*printf("\n a char entered \n");*/
		/*putc(ch, stdout);*/
	/*}*/
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
	/*fprintf(fp, "This is testing for fprintf...\n");*/
	/*fputs("This is testing for fputs...\n", fp);*/
	if(fwrite((void *)buffer, 1024*768*3, 1, fp)<0){
		printf("mgba ERROR!!!");
	}
	fclose(fp);
	return 0;
}
