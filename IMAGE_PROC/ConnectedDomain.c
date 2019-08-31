#include "ConnectedDomain.h"
//Labling connected components in an image, where non-zero pixels are 
// deemed as foreground, and will be labeled with an positive integer
// while background pixels will be labled with zeros.
//Input and output are 2D matrices of size h-by-w.
//Return maxLabel. Output labels are continuously ranged between [0,maxLabel).
//Assume each pixel has 4 neighbors.
//yuxianguo, 2018/3/27
//label 大小为 h * w << 2
int bwLabel(const unsigned char *bw, u16 *label, int h, int w)
{
	memset(label, 0, h * w * 2);
	//link[i]:
	//(1) link label value "i" to its connected component (another label value);
	//(2) if link[i] == i, then it is a root.
	int maxComponents = (h * w >> 1) + 1; //max possible connected components
	int * const link =  mymalloc (SRAMEX, maxComponents * sizeof(int));;
	int lb = 1, x, y, a, b, t;
	u16 *p = label;//malloc
	link[0] = 0;
	//first row
	if(bw[0]) {
		p[0] = lb;
		link[lb] = lb;
		lb++;
	}
	for(x = 1; x < w; x++) if(bw[x]) {
		if(p[x - 1])
			p[x] = p[x - 1];
		else {
			p[x] = lb;
			link[lb] = lb;
			lb++;
		}
	}
	bw += w, p += w;
	//rest rows
	for(y = 1; y < h; y++, bw += w, p += w) {
		if(bw[0]) {
			if(p[-w])
				p[0] = p[-w];
			else {
				p[0] = lb;
				link[lb] = lb;
				lb++;
			}
		}
		for(x = 1; x < w; x++) if(bw[x]) {
			a = p[x - 1], b = p[x - w]; //left & top
			if(a) {
				if(a == b)
					p[x] = a;
				else {
					//find root of a
					t = a;
					while(a != link[a])
						a = link[a];
					p[x] = link[t] = a;
					if(b) {
						//find root of b
						t = b;
						while(b != link[b])
							b = link[b];
						link[t] = b;
						//link b to a or link a to b, both fine
						if(a < b) link[b] = a; else link[a] = b;
					}
				}
			}
			else if(b) {
				//find root of b
				t = b;
				while(b != link[b])
					b = link[b];
				p[x] = link[t] = b;
			}
			else {
				//generate a new component
				p[x] = lb;
				link[lb] = lb;
				lb++;
			}
		}
	}
 
	//Rearrange the labels with continuous numbers
	t = 1;
	for(x = 1; x < lb; x++)
		if(x == link[x]) {
			link[x] = -t; //using negative values to denote roots
			t++;
		}
	for(x = 1; x < lb; x++) {
		//find the root of x
		y = x;
		while(link[y] >= 0)
			y = link[y];
		//set the value of label x
		link[x] = link[y];
	}
	//Negative to positive
	for(x = 1; x < lb; x++)
		link[x] = -link[x];
 
	//Replace existing label values by the corresponding root label values
	p = label;
	for(y = 0; y < h; y++, p += w)
		for(x = 0; x < w; x++)
			p[x] = link[p[x]];
 
	myfree(SRAMEX, link);
	return t; //num components (maxLabel + 1)
}
