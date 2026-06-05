
#include "Gridmap.h"
#include <math.h>

#define MAX_VALUE 99999999

particles::particles(void)
{
	pose.x = 0.0;
	pose.y = 0.0;
	pose.theta = 0.0;
	weight = 0.0;
	tempweight = 0.0;
	prop = 0.0;
}

particles::~particles(void)
{
}

ProbMap::ProbMap(void)
{
	data = NULL;
    width = 0;
    height = 0;
}

ProbMap::~ProbMap(void)
{
	if(data !=NULL)
	{
		delete [] data;
		data = NULL;
	}
}

void ProbMap::setValue(double x, double y, double v)
{
    int ix = (int) ((x - x0) / metersPerPixel);
    int iy = (int) ((y - y0) / metersPerPixel);

    setValueIndexSafe(ix, iy, v);
}



void ProbMap::setValueIndexSafe(int ix, int iy, double v)   //如果安全，没有越界，置位
{
    if (iy < 0 || ix < 0 || ix >= width || iy >= height)      //越界，返回
    {
    	//System.out.println("out of range");
    	return;
    }

    data[iy*width + ix] *= v;         //置位
#if 1
	double a = data[iy*width + ix];
	if(a > 524288)
	{
		data[iy*width + ix] = 524288;
	}
	else
	{
		double b = 1/524288;
		if(a < b)
		{
			data[iy*width + ix] = b;
		}

	}
#endif
}

void ProbMap::drawLine(double xa, double ya, double xb, double yb, double fill)
{
    double dist = sqrt(sq(xb-xa) + sq(yb-ya));         //算出两点距离
    int nsteps = (int) (dist / metersPerPixel + 1);    //这些距离有多少个像素点
    double pixelsPerMeter = 1.0 / metersPerPixel;

	//算a/b之间的直线上每个像素点的坐标，x/y单独算
	//例：一共10个点，第3个点：x = xa+3/10(xb-xa) = (1-3/10)xa+3/10xb = alpha*xb+(1-alpha)*xa, xa/xb反过来也一样
    for (int i = 0; i < nsteps; i++) 
	{
        double alpha = ((double) i)/nsteps;
        double x = xa*alpha + xb*(1-alpha);
        double y = ya*alpha + yb*(1-alpha);

        int ix = (int) ((x - x0) * pixelsPerMeter);
        int iy = (int) ((y - y0) * pixelsPerMeter);

        if (ix >= 0 && ix < width && iy >= 0 && iy < height)
		{
            data[iy*width + ix] *= fill;        //线上的点没有越界，填充
			double a = data[iy*width + ix];
			if(a > 524288)
			{
				data[iy*width + ix] = 524288;
			}
			else
			{
				double b = 1/524288;
				if(a < b)
				{
					data[iy*width + ix] = b;
				}

			}
		}
	}
}

double ProbMap::sq(double v)
{
    return v*v;
}

void ProbMap::makePixels(double dx0, double dy0, int nwidth, int nheight, double dmetersPerPixel, double ndefaultFill, bool broundUpDimensions)
{
     

        x0 = dx0;
        y0 = dy0;
        metersPerPixel = dmetersPerPixel;
        defaultFill = ndefaultFill;

        // compute pixel dimensions
        width = nwidth;
        height = nheight;

        if (broundUpDimensions) {
            // round up to multiple of four (necessary for OpenGL happiness)  4的倍数
            width += 4 - (width%4);
            height += 4 - (height%4);
        }
#if 0
		if (data == NULL)

			data = new BYTE[width*height];
		else
		{
			delete [] data;
			data = NULL;

		}
#endif

		if (data != NULL)
		{
			delete [] data;
			data = NULL;
		}
		data = new double[width*height];
      
        fill(defaultFill);   //全部填充为 defaultFill

       
}

void ProbMap::fill(double v)
{
    double bv = v;

    for (int i = 0; i < width*height; i++)
        data[i] = bv;
}







GridMap::GridMap(void)
{
	data = NULL;
    width = 0;
    height = 0;
}


GridMap::~GridMap(void)
{
	if(data !=NULL)
	{
		delete [] data;
		data = NULL;
	}
}
void GridMap::makeMeters(double dx0, double dy0, double sizex, double sizey, double dmetersPerPixel, int ndefaultFill)
{
        // compute pixel dimensions.
        int nwidth = (int) (sizex / dmetersPerPixel + 1);
        int nheight = (int) (sizey / dmetersPerPixel + 1);

        return makePixels(dx0, dy0, nwidth, nheight, dmetersPerPixel, ndefaultFill, true);
}
void GridMap::makePixels(double dx0, double dy0, int nwidth, int nheight, double dmetersPerPixel, int ndefaultFill, bool broundUpDimensions)
{
     

        x0 = dx0;
        y0 = dy0;
        metersPerPixel = dmetersPerPixel;
        defaultFill = (BYTE) ndefaultFill;

        // compute pixel dimensions
        width = nwidth;
        height = nheight;

        if (broundUpDimensions) {
            // round up to multiple of four (necessary for OpenGL happiness)  4的倍数
            width += 4 - (width%4);
            height += 4 - (height%4);
        }
#if 0
		if (data == NULL)
			data = new BYTE[width*height];
		else
		{
			delete [] data;
			data = NULL;

		}
#endif

		if (data != NULL)
		{
			delete [] data;
			data = NULL;
		}
		data = new BYTE[width*height];
      
            fill(defaultFill);   //全部填充为 defaultFill

       
}

void GridMap::cropMeters(double xmin, double ymin, double _width, double _height, bool roundUpDimensions,GridMap &gm)
    {
        return cropPixels((int) ((xmin - x0) / metersPerPixel),
                          (int) ((ymin - y0) / metersPerPixel),
                          (int) (_width / metersPerPixel),
                          (int) (_height / metersPerPixel), roundUpDimensions,gm);
    }
void GridMap::cropPixels(int xmin, int ymin, int _width, int _height, bool roundUpDimensions,GridMap &gm)
    {
        xmin = max(0, xmin);
        ymin = max(0, ymin);

        _width = max(0, _width);
        _height = max(0, _height);

        return resizePixels(xmin,ymin, _width, _height, roundUpDimensions,gm);
    }

    // Return a gridmap that contains all of the non-zero pixels, but
    // is (potentially) smaller than the original
void GridMap::crop(bool roundUpDimensions,GridMap &gm)
    {
        int xmin = MAX_VALUE, xmax = -1;
        int ymin = MAX_VALUE, ymax = -1;

        // find bounding box. (This is a bit inefficient... perhaps it
        // would be faster if we worked in from the edges.)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (data[y*width+x] != 0) {
                    xmin = min(xmin, x);
                    xmax = max(xmax, x);
                    ymin = min(ymin, y);
                    ymax = max(ymax, y);
                }
            }
        }

        if (xmax < xmin) {
            xmin = 0;
            ymin = 0;
            xmax = 0;
            ymax = 0;
        }

        return resizePixels(xmin, ymin, xmax-xmin+1, ymax-ymin+1, roundUpDimensions,gm);

    }
    // Returns a new grid map 'grid-aligned' with 'this', given the new bounds
    //  Eventually we should have crop reference this function
void GridMap::resizeMeters(double x0_m, double y0_m, double width_m, double height_m, bool roundUpDimensions)
    {
        // Compute the number of pixels to offset by
        int xmin = (int)floor((x0_m -x0) /metersPerPixel);
        int ymin = (int)floor((y0_m -y0) /metersPerPixel);

        // We may need to shift x0_m since we are constrained to 'this''s grid spacing
        double x0_round = x0 + xmin * metersPerPixel;
        double y0_round = y0 + ymin * metersPerPixel;

        int width = (int)ceil((width_m + x0_m - x0_round)/metersPerPixel);
        int height = (int)ceil((height_m + y0_m - y0_round)/metersPerPixel);


        return resizePixels(xmin, ymin, width, height, roundUpDimensions);

    }
 void GridMap::resizePixels(int xmin, int ymin, int _width, int _height, bool roundUpDimensions)
 {
#if 0
	  BYTE   *dataold;
	   dataold = new BYTE[width*height];


	   int    widthold = width;   
	   int    heightold = height;

 
	   for (int i = 0; i < width*height; i++)
            dataold[i] = data[i];
#endif
      
        x0 = x0 + xmin * metersPerPixel;
        y0 = y0 + ymin * metersPerPixel;
        width =  _width;
        height = _height;
      
   
        if (roundUpDimensions) {
            // round up to multiple of four (necessary for OpenGL happiness)
            width += (4 - (width % 4)) % 4; // final mod ensures we don't add 4 needlessly
            height += (4 - (height% 4)) % 4;
        }
#if 0
		if(data!=NULL)
		{
			delete [] data;
			data = NULL;

		}
        data = new BYTE[width*height];
		memset(data,0,sizeof(BYTE)*width*height);

        // crawl the new gm and insert old values where applicable
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (y + ymin >= 0 && y + ymin < heightold &&
                    x + xmin >= 0 && x + xmin < widthold)
                    data[y*width+x] = dataold[(y+ymin)*widthold + (x+xmin)];
                else
                    data[y*width+x] = defaultFill;
            }
        }

		if(dataold!=NULL)
		{
			delete [] dataold;
			dataold = NULL;

		}
#endif
    
    }


 void GridMap::resizePixels(int xmin, int ymin, int _width, int _height, bool roundUpDimensions,GridMap &gm)
 {
      
        gm.x0 = x0 + xmin * metersPerPixel;
        gm.y0 = y0 + ymin * metersPerPixel;
        gm.width =  _width;
        gm.height = _height;
        gm.metersPerPixel = metersPerPixel;
        gm.defaultFill = (BYTE) defaultFill;

        if (roundUpDimensions) {
            // round up to multiple of four (necessary for OpenGL happiness)
            gm.width += (4 - (gm.width % 4)) % 4; // final mod ensures we don't add 4 needlessly
            gm.height += (4 - (gm.height% 4)) % 4;
        }
		if(gm.data!=NULL)
		{
			delete [] gm.data;
			gm.data = NULL;

		}
        gm.data = new BYTE[gm.width*gm.height];

        // crawl the new gm and insert old values where applicable
        for (int y = 0; y < gm.height; y++) {
            for (int x = 0; x < gm.width; x++) {
                if (y + ymin >= 0 && y + ymin < height &&
                    x + xmin >= 0 && x + xmin < width)
                    gm.data[y*gm.width+x] = data[(y+ymin)*width + (x+xmin)];
                else
                    gm.data[y*gm.width+x] = gm.defaultFill;
            }
        }

    
    }


 void GridMap::clear()
    {
        fill(defaultFill);
    }

    /** Write the provided value to every grid element **/
 void GridMap::fill(int v)
    {
        BYTE bv = (BYTE) v;

        for (int i = 0; i < width*height; i++)
            data[i] = bv;
    }

    /** Map every current value of the gridmap to a new value. The values array should be 255. **/
 void GridMap::map(BYTE values[])
    {
        for (int i = 0; i < width*height; i++)
            data[i] = values[data[i]&0xff];
    }

    /** Modify data with all new values. **/
 void GridMap::setData(BYTE values[],int valuelengh)
    {
        for (int i = 0; i < width*height && i < valuelengh; i++)
            data[i] = values[i];
    }
 GridMap& GridMap::operator=(GridMap &gm)
 {
        x0 = gm.x0;
        y0 = gm.y0;
        metersPerPixel = gm.metersPerPixel;
        width = gm.width;
        height = gm.height;
		if(data !=NULL)
		{
			delete [] data;
			data = NULL;
		}
        data = new BYTE[gm.width*gm.height];
        defaultFill = gm.defaultFill;

        for (int i = 0; i < width*height; i++)
            data[i] = gm.data[i];

        return *this;

 }
 void GridMap::copy(GridMap &gm)
    {
       
        gm.x0 = x0;
        gm.y0 = y0;
        gm.metersPerPixel = metersPerPixel;
        gm.width = width;
        gm.height = height;
		if(gm.data!=NULL)
		{
			delete [] gm.data;
			gm.data = NULL;

		}
        gm.data = new BYTE[width*height];
        gm.defaultFill = defaultFill;

        for (int i = 0; i < width*height; i++)
            gm.data[i] = data[i];

        return ;
    }

    /** return the average value of the gridmap **/
	double GridMap::average()
    {
        double sum = 0;
        for (int i = 0; i < width*height; i++) {
            sum += data[i];
        }

        return sum / (width*height);
    }

    double GridMap::sq(double v)
    {
        return v*v;
    }

    int GridMap::sgn(double v)
    {
        if (v > 0)
            return 1;
        if (v < 0)
            return -1;
        return 0;
    }

    /** memset **/
    void GridMap::arraySet(BYTE d[], int offset, int length, BYTE value)  //offset为起点，length为长度，置为value
    {
        for (int i = 0; i < length; i++)
            d[i+offset] = value;
    }

    /** memmove (i.e., areas can overlap **/
    void GridMap::arrayMove(BYTE d[], int srcoffset, int destoffset, int len)    //srcoffset起，len长度的数据，复制到destoffset
    {                                                                               //长度为len的位置
        if (destoffset > srcoffset) {
            for (int i = len-1; i >= 0; i--)
                d[destoffset + i] = d[srcoffset + i];
        } else {
            for (int i = 0; i < len; i++)
                d[destoffset + i] = d[srcoffset + i];
        }
    }

    /** Recenter the gridmap, translating the data as necessary so
     * that cx0 and cy0 are near new center. The recenter operation is
     * skipped if the map would be translated by less than
     * maxDistance.
     **/
    void GridMap::recenter(double cx0, double cy0, double maxDistance)
    {
        double cx = x0 + (width + .5) * metersPerPixel / 2;
        double cy = y0 + (height + .5) * metersPerPixel / 2;

        double distanceSq = sq(cx - cx0) + sq(cy - cy0);
        double pixelsPerMeter = 1.0 / metersPerPixel;

        if (distanceSq < sq(maxDistance))
            return;

        // how far (in pixels) to move. We round to an exact pixel
        // boundary.  e.g., dx = source - dest
        int dx = (int) ((cx0 - cx) * pixelsPerMeter);
        int dy = (int) ((cy0 - cy) * pixelsPerMeter);

		//因为中心点有偏差，需要移动数据，一共能分成四种情况，dx/dy >/<0。
		//数据矩阵进行左上，左下，右上，右下移动dx、dy距离，不够的补defaultFill
        if (dy >= 0) {       //dy>0,数据需向下移动
            for (int dest = 0; dest < height; dest++) {
                int src = dest + dy;
                if (src < 0 || src >= height) {
                    arraySet(data, dest*width, width, defaultFill);  //哪一行y坐标超过范围，哪一行的值置为defaultFill
                    continue;
                }

                if (dx >= 0) {      //dx>0，数据需向左移动 
                    int sz = max(0, width - dx);
                    arrayMove(data, src*width + dx, dest*width, sz);
                    arraySet(data, dest*width + sz, width - sz, defaultFill);
                } else {            //dx<0，数据需向右移动
                    int sz = max(0, width + dx);
                    arrayMove(data, src*width, dest*width - dx, sz);
                    arraySet(data, dest*width, width - sz, defaultFill);
                }
            }
        } else {      //dy<0，向上移动
            for (int dest = height-1; dest >= 0; dest--) {
                int src = dest + dy;

                if (src < 0 || src >= height) {
                    arraySet(data, dest*width, width, defaultFill);
                    continue;
                }

                if (dx >= 0) {
                    int sz = max(0, width - dx);
                    arrayMove(data, src*width + dx, dest*width, sz);
                    arraySet(data, dest*width + sz, width - sz, defaultFill);
                } else {
                    int sz = max(0, width + dx);
                    arrayMove(data, src*width, dest*width - dx, sz);
                    arraySet(data, dest*width, width - sz, defaultFill);
                }
            }
        }

        cx += dx * metersPerPixel;   //数据矩阵中心代表的位置变化了
        cy += dy * metersPerPixel;
        x0 += dx * metersPerPixel;    //地图左下角坐标代表的位置变化了
        y0 += dy * metersPerPixel;
    }

    double GridMap::evaluatePath(vector<Pose> &xys, bool negativeOn255)   //路径评估
    {
        double cost = 0;

        for (int i = 0; i + 1 < xys.size(); i++) {
            double thisCost = evaluatePath(xys.at(i), xys.at(i+1), negativeOn255);
            if (thisCost < 0)
                return thisCost;
            cost += thisCost;
        }

        return cost;
    }

	    /** Evaluate the integral of the cost along the path from xy0 to
     * xy1. If negativeOn255 is set, -1 will be returned if the path
     * goes through a cell whose value is 255. **/
    double GridMap::evaluatePath(Pose xy0, Pose xy1, bool negativeOn255)
    {
        // we'll microstep at 0.25 pixels. this isn't exact but it's pretty darn close.
        double stepSize = metersPerPixel * 0.25;      //步长

        double dist = sqrt(sq(xy0.x-xy1.x) + sq(xy0.y-xy1.y));  //两点间距离

        int nsteps = ((int) (dist / stepSize)) + 1;      //多少步

        double cost = 0;

        for (int i = 0; i < nsteps; i++) {
            double alpha = ((double) i) / nsteps;
            double x = alpha*xy0.x + (1-alpha)*xy1.x;   //每一步的坐标
            double y = alpha*xy0.y + (1-alpha)*xy1.y;

            int v = getValue(x,y);
            if (negativeOn255 && v==255)
                return -1;

            cost += v;
        }

        // normalize correctly for distance.
        return cost * dist / nsteps;
    }

	 int GridMap::getValue(double x, double y)
    {
        int ix = (int) ((x - x0) / metersPerPixel);
        int iy = (int) ((y - y0) / metersPerPixel);

        return getValueIndexSafe(ix, iy, defaultFill);
    }

    int GridMap::getValueIndex(int ix, int iy)
    {
        return data[iy*width + ix]&0xff;
    }

    int GridMap::getValueIndexSafe(int ix, int iy)
    {
        return getValueIndexSafe(ix, iy, defaultFill);
    }

    int GridMap::getValueIndexSafe(int ix, int iy, int def)
    {
        if (iy < 0 || ix < 0 || ix >= width || iy >= height)
            return def;
	}

	void GridMap::drawCircleMax(double cx, double cy, double r, BYTE fill)
    {
        double pixelsPerMeter = 1.0 / metersPerPixel;

        int ix0 = (int) ((cx - r - x0) * pixelsPerMeter);
        int ix1 = (int) ((cx + r - x0) * pixelsPerMeter);
        int iy0 = (int) ((cy - r - y0) * pixelsPerMeter);
        int iy1 = (int) ((cy + r - y0) * pixelsPerMeter);

        for (int iy = iy0; iy <= iy1; iy++) {
            if (iy < 0 || iy >= height)
                continue;

            for (int ix = ix0; ix <= ix1; ix++) {
                if (ix < 0 || ix >= width)
                    continue;

                double x = x0 + (ix + .5)*metersPerPixel;
                double y = y0 + (iy + .5)*metersPerPixel;

                double d2 = (x - cx)*(x-cx) + (y - cy)*(y - cy);
                if (d2 <= (r*r))
                    data[iy*width + ix] = (BYTE) max(fill & 0xFF,
                                                          data[iy*width + ix] & 0xFF);
            }
        }
    }

    void GridMap::drawCircle(double cx, double cy, double r, BYTE fill)
    {
        double pixelsPerMeter = 1.0 / metersPerPixel;

		//以cx、cy为中心，+-r扩展出了一个矩形/正方形
        int ix0 = (int) ((cx - r - x0) * pixelsPerMeter);
        int ix1 = (int) ((cx + r - x0) * pixelsPerMeter);
        int iy0 = (int) ((cy - r - y0) * pixelsPerMeter);
        int iy1 = (int) ((cy + r - y0) * pixelsPerMeter);

		//在矩形范围内，是否越界
        for (int iy = iy0; iy <= iy1; iy++) {
            if (iy < 0 || iy >= height)
                continue;

            for (int ix = ix0; ix <= ix1; ix++) {      
                if (ix < 0 || ix >= width)
                    continue;

				//算矩形内的符合条件的点的坐标
                double x = x0 + (ix + .5)*metersPerPixel;
                double y = y0 + (iy + .5)*metersPerPixel;

				//算离圆心距离
                double d2 = (x - cx)*(x-cx) + (y - cy)*(y - cy);
                if (d2 <= (r*r))
                    data[iy*width + ix] = fill;   //在圆内，填充为fill
            }
        }
    }

    void GridMap::drawLine(double xa, double ya, double xb, double yb, BYTE fill)
    {
        double dist = sqrt(sq(xb-xa) + sq(yb-ya));         //算出两点距离
        int nsteps = (int) (dist / metersPerPixel + 1);    //这些距离有多少个像素点
        double pixelsPerMeter = 1.0 / metersPerPixel;

		//算a/b之间的直线上每个像素点的坐标，x/y单独算
		//例：一共10个点，第3个点：x = xa+3/10(xb-xa) = (1-3/10)xa+3/10xb = alpha*xb+(1-alpha)*xa, xa/xb反过来也一样
        for (int i = 0; i < nsteps; i++) {
            double alpha = ((double) i)/nsteps;
            double x = xa*alpha + xb*(1-alpha);
            double y = ya*alpha + yb*(1-alpha);

            int ix = (int) ((x - x0) * pixelsPerMeter);
            int iy = (int) ((y - y0) * pixelsPerMeter);

            if (ix >= 0 && ix < width && iy >= 0 && iy < height)
                data[iy*width + ix] = fill;        //线上的点没有越界，填充
        }
    }

    void GridMap::drawLineMax(double xa, double ya, double xb, double yb, BYTE fill)
    {
        double dist = sqrt(sq(xb-xa) + sq(yb-ya));
        int nsteps = (int) (dist / metersPerPixel + 1);
        double pixelsPerMeter = 1.0 / metersPerPixel;

        for (int i = 0; i < nsteps; i++) {
            double alpha = ((double) i)/nsteps;
            double x = xa*alpha + xb*(1-alpha);
            double y = ya*alpha + yb*(1-alpha);

            int ix = (int) ((x - x0) * pixelsPerMeter);
            int iy = (int) ((y - y0) * pixelsPerMeter);

            if (ix >= 0 && ix < width && iy >= 0 && iy < height)
                data[iy*width + ix] = (BYTE) max(data[iy*width+ix]&0xff, fill&0xff);
        }
    }

    void GridMap::drawLineInterpolate(double xa, double ya, double xb, double yb, int f0, int f1)
    {
        double dist = sqrt(sq(xb-xa) + sq(yb-ya));  //算两点距离
        int nsteps = (int) (dist / metersPerPixel + 1);  //算有多少个像素点
        double pixelsPerMeter = 1.0 / metersPerPixel;     //每米多少个点

        for (int i = 0; i < nsteps; i++) {      //挨个点走一遍
            double alpha = ((double) i)/nsteps;

			//每个点的坐标
            double x = xa*alpha + xb*(1-alpha);    
            double y = ya*alpha + yb*(1-alpha);

			//像素上的坐标
            int ix = (int) ((x - x0) * pixelsPerMeter);
            int iy = (int) ((y - y0) * pixelsPerMeter);

            if (ix >= 0 && ix < width && iy >= 0 && iy < height) {    //坐标没越界
                int f = (int) (f0*alpha + f1*(1-alpha));          //待解决
                data[iy*width + ix] = (BYTE) f;
            }
        }
    }


	    /** Get the lower-left corner of the grid (minimum x and y coordinates).
      * Note: *Not* the pixel center (1/2 pixel off in meters)
      **/
    void GridMap::getXY0(double &dx0, double &dy0)   //地图左下角的坐标，即x，y最小值的点坐标，并非是0，0点，建图起始点
    {                                                  //为0,0点，左下角一般为负数
        dx0 = x0;
		dy0 = y0;
    }

    /** Get the upper-right corner of the grid (maximum x and y coordinates).
      * Note: *Not* the pixel center (1/2 pixel off in meters)
      **/
    void GridMap::getXY1(double &dx1, double &dy1)  //地图的边界，即地图右上角点的坐标，x，y最大
    {
        dx1 = x0 + width*metersPerPixel;       //width/height是地图的宽度和高度
		dy1 = y0 + height*metersPerPixel;
    }

    void GridMap::setValue(double x, double y, BYTE v)
    {
        int ix = (int) ((x - x0) / metersPerPixel);
        int iy = (int) ((y - y0) / metersPerPixel);

        setValueIndexSafe(ix, iy, v);
    }

    void GridMap::setValueIndex(int ix, int iy, BYTE v)
    {
        data[iy*width + ix] = v;
    }

    void GridMap::setValueIndexSafe(int ix, int iy, BYTE v)   //如果安全，没有越界，置位
    {
        if (iy < 0 || ix < 0 || ix >= width || iy >= height)      //越界，返回
        {
        	//System.out.println("out of range");
        	return;
        }

        data[iy*width + ix] = v;         //置位
    }

	   /** Does the grid cell at (ix,iy) have a neighbor whose value is
     * v? ix, iy are in pixel coordinates. **/
    bool GridMap::hasNeighbor(int ix, int iy, BYTE v)     //周围8个点是否有值为 BYTE v 的相邻点
    {
        if (iy > 0) {
            if (ix > 0)
                if (data[(iy-1)*width+(ix-1)]==v)   //左下角
                    return true;
            if (data[(iy-1)*width+ix]==v)           //下
				    return true;
            if (ix+1 < width)
                if (data[(iy-1)*width+(ix+1)]==v)   //右下
                    return true;
        }

        if (ix > 0)
            if (data[iy*width+ix-1]==v)             //左
                return true;
        if (ix+1 < width)
            if (data[iy*width+ix+1]==v)             //右
                return true;

        if (iy+1 < height) {
            if (ix > 0)
                if (data[(iy+1)*width+(ix-1)]==v)  //左上
                    return true;
            if (data[(iy+1)*width+ix]==v)          //上
                return true;
            if (ix+1 < width)
                if (data[(iy+1)*width+(ix+1)]==v)  //右上
                    return true;
        }
        return false;
    }

   /** Find all grid cells with value v that have a neighbor that is
     * not v. Clear all other cells to zero.
     **/
	void GridMap::edges(BYTE v, GridMap &gm)
    {
         copy(gm);      //把当前的地图所有信息拷贝给gm，x0,y0,metersPerPixel.....

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {    //从x0，y0开始遍历data

                if (data[y*width+x] != v) {
                    gm.data[y*width+x] = 0;
                    continue;
                }

                if (!hasNeighbor(x, y, (BYTE) 0))   //周围没有值为0的点，说明此点不是边沿点
                    gm.data[y*width + x] = 0;       //此点置为0
            }
        }

        return ;
    }

	    /** Set all neighbors of grid cells with value 'v' to v. Repeat
     * this process 'iterations' times.
     **/
   void GridMap::dilate(BYTE v, int iterations,GridMap &dest)   //iterations:迭代次数
    {


    /*    for (int iter = 0; iter < iterations; iter++) {

            copy(dest);

            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    if (dest.data[y*width+x]==v)
                        continue;
                    if (hasNeighbor(x, y, v))
                        dest.data[y*width+x] = v;
                }
            }

            //src = dest; ????????
        }

      //  if (dest == null)
      //      return src.copy();*/

        for (int iter = 0; iter < iterations; iter++)         //是不是有问题，从第一个不是0的点开始，向右的点都置为-1了
		{                                                     //没用上？

            copy(dest);

            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    if (dest.data[y*width+x]==v)
                        continue;
                    if (hasNeighbor(x, y, v))
					{
                        dest.data[y*width+x] = v;
					//	data[y*width+x] = v;

					}
                }
            }

           
        }

      //  if (dest == null)
      //      return src.copy();

    }

    /** Multiply every value in the grid by 's'. **/
    void  GridMap::scale(double s)            //值按比例放大，没用上？
    {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int v0 = data[y*width+x]&0xff;

                data[y*width+x] = (BYTE) (v0 * s);
            }
        }
    }
        
   /** Subtract from every value the value 'v', stopping at zero. **/
    void GridMap::subtract(int v)           //扣去
    {
        for (int i = 0; i < width*height; i++) {     //地图遍历
            int w = data[i]&0xff;
            w -= v;
            if (w < 0)                  //原值小于等于v的点，会被置为0
                w = 0;
            data[i] = (BYTE) w;
        }
    }

    bool GridMap::isCompatible(GridMap &gm)              //是否兼容
    {
        return !(gm.x0 != x0 || gm.y0 != y0 ||            //有一个条件不满足，就返回false
                 gm.metersPerPixel != metersPerPixel ||
                 gm.width != width || gm.height != height);
    }

    void GridMap::plusEquals(GridMap &gm)         //将两个一样的地图，值相加
    {
        assert(isCompatible(gm));      //断言，是否兼容

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int v0 = data[y*width+x]&0xff;
                int v1 = gm.data[y*width+x]&0xff;

                data[y*width+x] = (BYTE) (v0 + v1);     //将gm地图的点值加到data上
            }
        }
    }

    void GridMap::maxEquals(GridMap &gm)          //两个地图，点值取最大值
    {
        assert(isCompatible(gm));

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int v0 = data[y*width+x]&0xff;
                int v1 = gm.data[y*width+x]&0xff;

                data[y*width+x] = (BYTE) max(v0, v1);
            }
        }
    }

    /** Starting at pixel coordinates (ix,iy) do a flood fill over all
     * pixels whose integer value[0,255] is less than tolerance,
     * setting those pixels to 'v'. **/
    void GridMap::fill(int ix, int iy, BYTE v, int tolerance)
    {
        fillDown(ix, ix, iy, v, tolerance);
    }

    /** the span [ix0,ix1] on row iy is a set of pixels that neighbors
        matching pixels in the row above.  For each span of matching
        pixels in this row, recurse below. If we find a pixel above us
        that needs to be set, recurse upwards.
    **/
    void GridMap::fillDown(int ix0, int ix1, int iy, BYTE v, int tolerance)   //啥破玩意
    {
        if (iy >= height)      //越界
            return;

        // grow horizontally within this row.
        while (ix0 > 0 && (data[iy*width + ix0]&0xff) < tolerance)      //从ix0向左找到第一个不小于容忍值的点
            ix0--;                                                      //如果第2个点小于容忍值，则ix0=0

        while (ix1+1 < width && (data[iy*width + ix1]&0xff) < tolerance) //ix1同上，向右找到第一个不小于容忍这的点
            ix1++;                                                       //如果倒数第二个值小于容忍值，则ix1=width-1

        // consider all pixels in this horizontal span.
        int start = -1;

        if ((data[iy*width+ix0]&0xff) < tolerance)       //只有在ix0=0时才起作用
            start = ix0;                                 //如果ix0满足要求，start=0，否则start=-1

        bool dirty = false;

        for (int ix = ix0; ix <= ix1; ix++) {           //ix0不会越界，start起始值不一定，可能是-1或0
            if ((data[iy*width+ix]&0xff) < tolerance) {
                data[iy*width+ix] = v;
                dirty = true;
                if (start < 0) { // start a new run?
                    start = ix;                       //start值为第一个小于容忍值的点
                }
            } else {
                if (start >= 0) { // end a run?
                    fillDown(start, ix-1, iy+1, v, tolerance);
                    start = -1;
                }     //若想在跳出循环后start=-1，则在ix1处，值必须大于容忍值，也就是
            }         //如果在跳出循环后start>=0,则表示ix1是最后一个数，且小于容忍值
        }

        if (start >= 0)
            fillDown(start, ix1, iy+1, v, tolerance);

        if (iy > 0 && dirty) {
            fillUp(ix0, ix1, iy-1, v, tolerance);
        }
    }
	   /** the span [ix0,ix1] on row iy is a set of pixels that neighbors
        matching pixels in the row above.  For each span of matching
        pixels in this row, recurse below. If we find a pixel above us
        that needs to be set, recurse upwards.
    **/
    void GridMap::fillUp(int ix0, int ix1, int iy, BYTE v, int tolerance)
    {
        if (iy < 0)
            return;

        // grow horizontally within this row.
        while (ix0 > 0 && (data[iy*width + ix0]&0xff) <= tolerance)
            ix0--;

        while (ix1+1 < width && (data[iy*width + ix1]&0xff) <= tolerance)
            ix1++;

        // consider all pixels in this horizontal span.
        int start = -1;

        if ((data[iy*width+ix0]&0xff) <= tolerance)
            start = ix0;

        bool dirty = false;

        for (int ix = ix0; ix <= ix1; ix++) {
            if ((data[iy*width+ix]&0xff) < tolerance) {
                data[iy*width+ix] = v;
                dirty = true;
                if (start < 0) { // start a new run?
                    start = ix;
                }

            } else {
                if (start >= 0) { // end a run?
                    fillUp(start, ix-1, iy-1, v, tolerance);
                    start = -1;
                }
            }
        }

        if (start >= 0)
            fillUp(start, ix1, iy-1, v, tolerance);


        if (iy+1 < height && dirty) {
            fillDown(ix0, ix1, iy+1, v, tolerance);
        }
    }
    /** Make a lut that has value of 255 for the first
        cliffDistMeters, then drops of as 255*exp(-expdecay*dist^2).
        The whole lut will be scaled by 'scale'.

        v = 255*scale*exp(-max(0, dist - cliff)^2*expDecayMSq)
    **/
    /*
     高斯查找表，目的是将高斯核函数在一定范围内，离散抽样，样点个数为n，则生成一个向量容器，将n个点的高斯权值依次压入栈中
     scale是比例因子，cliffDistMeters是峰值点对应的x坐标，此函数只取峰值右边的高斯函数，expDecayMSq是方差的倒数，即1/sigma^2
     峰值点为255*a，取值范围为高斯值从峰值衰减到1，对应的X坐标范围，具体算法步骤参考高斯核函数推导
	*/
	void GridMap::makeGaussianLUT(double scale, double cliffDistMeters, double expDecayMSq, LUT &lut)
    {
       
        lut.metersPerPixel = metersPerPixel;     //lut里有double meterPerPixel，vector<int> vtlut,    int length

        assert(expDecayMSq > 0);

        // how long does the LUT need to be for the last element to decay to 0?
        // solve expression for distance, setting v = 1. Then round 'd' up.
        double maxDistance = cliffDistMeters + sqrt(log(255*scale)/expDecayMSq);

        int length = (int) (maxDistance / lut.metersPerPixel + 1);  //计算结果为3，即高斯查找表长度为3,实际为2.7+1

      //  lut.plut = new int[length];

		lut.length = length; 

        for (int i = 0; i < length; i++) 
		{
            double d = max((double)0, i * lut.metersPerPixel - cliffDistMeters);//255,124,14
           // lut.plut[i] = (int) (255*scale*exp(-d*d*expDecayMSq));

			lut.vtlut.push_back((int) (255*scale*exp(-d*d*expDecayMSq)));
        }

        return ;
    }

	  /**
       255 - scale * d^2 / (stddev^2).

       Note that a scale of 0.5 would make the LUT correspond to a
       Gaussian's quadratic losses.

       Suppose you want us to decay to zero at 3 std devs. Then use
       scale = 255/9. (9=3^2).

       Now, suppose you want to convert back into a
       log-likelihood. Let q be the value of the LUT.

       log-likelihood = 0.5*(q - 255)/scale.
     **/
    void GridMap::makeQuadraticLUT(double scale, double stddev, LUT &lut)  //没用上
    {
        
        lut.metersPerPixel = metersPerPixel;

        // at what point will the LUT saturate at 255?
        double maxDistance = sqrt(255 * stddev * stddev / scale);

        int length = (int) (maxDistance / lut.metersPerPixel + 1);
      //  lut.plut = new int[length];
		lut.length = length; 

        for (int i = 0; i < length; i++) {
            double d = i * lut.metersPerPixel;
            // the .5 below is for proper rounding.
            int v = 255 - (int) (.5 + scale * d * d / (stddev * stddev));
            v = max(0, v);
          //  lut.plut[i] = v;

			lut.vtlut.push_back(v);

          //  double ll = 0.5*(lut.plut[i]-255)/scale;
			double ll = 0.5*(lut.vtlut.at(i)-255)/scale;
           // System.out.printf("%15f %5d %15f\n", d, lut.lut[i], ll);
        }

        return ;
    }

	    /** Make a lut that starts at 255 and linearly declines to 0. Used for computing distance transforms. **/
    void GridMap::makeLinearReverseLUT(LUT &lut)       //没用上
    {
       
        lut.metersPerPixel = metersPerPixel;

      //  lut.plut = new int[256];


		lut.length = 256; 
        for (int i = 0; i <=255; i++) {
           // lut.plut[i] = 255 - i;

			lut.vtlut.push_back(255-i);
        }

        return;
    }

    void GridMap::makeConstantLUT(int v, double width_meters, LUT &lut)  //查找表
    {
        
        lut.metersPerPixel = width_meters;

       // lut.plut = new int[1];
		lut.length = 1; 
       // lut.plut[0] = v;
		lut.vtlut.push_back(v);
        return ;
    }


    //通过画点功能初步了解画矩形函数来画点的实现过程和结果，就是在该点的位置，向上下左右扩展出高斯查找表长度的距离，相当于扩展出一个
    //矩形，然后依次计算矩形内每个像素点到该点的距离，然后转化为像素点个数，当像素距离在高斯范围内，将此像素填充，最终填充出的点会大
    //一些
	 void GridMap::drawDot(double x, double y, LUT& lut,int lutlength)
    {
        drawRectangle(x, y, 0, 0, 0, lut,lutlength);
    }

	//此处作为画直线的说明
	//参考画矩阵功能，y_size传参为0，则原矩形宽度为0，其余扩展方式不变，则扩展后，直线两头为1/2圆弧
    void GridMap::drawLine(double x0, double y0, double x1, double y1, LUT& lut)
    {
        double dx = (x1-x0), dy = (y1-y0);
        double length = sqrt(dx*dx + dy*dy);

		drawRectangle((x0+x1)/2, (y0+y1)/2, length, 0, atan2(y1-y0, x1-x0), lut, lut.length);
    }
	
   /*
    此函数功能是画矩阵，画点、画线也调用此函数，只是传的参数有区别，画点和画线功能在上面说明，这里说明画出的矩形效果；
    有两个点（x0,y0）（x1,y1），cx和cy是这两个点连线的中点坐标，x_size是两点连线的长度，可以理解为矩形的长度，y_size
    是矩形的宽度，方向垂直于x_size，theta是连线与X轴正方向的夹角，由tan2得来范围为[-pi/2，pi/2]；lut是高斯查找表，一般
    长度为3；首先要在原矩形大小上进行高斯距离扩展，在矩形的四个角中，在x坐标最小的点，向x轴负方向扩展出一个高斯距离，在x
    最大处，向x正方向扩展出一个高斯距离，同理在y坐标最大和最小处，向外各扩展出一个高斯距离，四个方向上扩展完后的大矩形，
    就是要搜索的范围，在原有矩形全部选中的基础上，沿两点连线方向，以y_size为上下界，向外扩展出一个高斯距离的矩形，同理，
    在y_size方向上，以x_size为左右界，向上下各扩展出一个矩形，此时扩展后的形状为一个十字，在十字的四个凹点处，是两个高斯
    距离形成的直角，将凹点出的两个顶点，以1/4圆弧连起来，此时所形成的图形，就是原矩形扩展后的要画的图形。
   */
    void GridMap::drawRectangle(double cx, double cy,            
                              double x_size, double y_size,
                              double theta,
                              LUT& lut,int lutlength)
    {
        double pixelsPerMeter = 1.0 / metersPerPixel;

        double ux = cos(theta), uy = sin(theta);

        double lutRange = metersPerPixel * lutlength;  //高斯距离，单位是米，高斯表长度是3，即0.15米

        double x_bound = (x_size / 2.0 * fabs(ux) + y_size / 2.0 * fabs(uy)) + lutRange;//需要扩展的大小
        double y_bound = (x_size / 2.0 * fabs(uy) + y_size / 2.0 * fabs(ux)) + lutRange;

        // lots of overdraw for high-aspect rectangles around 45 degrees.
        int ix0 = clamp((int) ((cx - x_bound - x0)*pixelsPerMeter), 0, width - 1);//int截断，会使数量少1,无形中相当于多扩展了一个像素，后面会通过+0.5个像素点
        int ix1 = clamp((int) ((cx + x_bound - x0)*pixelsPerMeter), 0, width - 1);//补偿距离

        int iy0 = clamp((int) ((cy - y_bound - y0)*pixelsPerMeter), 0, height - 1);
        int iy1 = clamp((int) ((cy + y_bound - y0)*pixelsPerMeter), 0, height - 1);

        // Each pixel will be evaluated based on the distance to the
        // center of that pixel.
        double y = y0 + (iy0+.5)*metersPerPixel; //对前面计算像素点时进行的int截断进行距离补偿，相当于缩小了半个栅格，定位不在栅格的左下角，而是会指向栅格中心点

        double lutPixelsPerMeter = 1.0 / lut.metersPerPixel;

        for (int iy = iy0; iy <= iy1; iy++) {

            double x = x0 + (ix0+.5)*metersPerPixel;//加0.5补偿

            for (int ix = ix0; ix <= ix1; ix++) {

                // distances from query point to center of rectangle
                //在自主行走时，建高斯地图时，如果修改drawDot参数为每个像素点的
                double dx = x - cx, dy = y - cy;   //这里x,y相当于每个栅格的中点，仔细考虑自主行走匹配时，生成高斯地图的过程，当传进来的是栅格中点时，会提高精度

                // how long are the projections of the vector (dx,dy) onto the two principle(原则)
                // components（组成部分） of the rectangle? How much longer are they than the dimensions（规模）
                // of the rectangle?
                //投影，与矩形的长和宽比较
                double c1 = fabs(dx * ux + dy * uy) - (x_size / 2);//值的正负表示是否在原矩形长或者宽的限制的范围内
                double c2 = fabs(- dx * uy + dy * ux) - (y_size / 2);

                // if the projection length is < 0, we're *inside* the rectangle.
                c1 = max((double)0, c1);//在矩形的长或者宽的范围内，值为0
                c2 = max((double)0, c2);

                double dist =sqrt(c1*c1 + c2*c2);
                //加。5的另一个作用是在矩形或线的边缘处，只有离边缘在0.5个像素距离内，此点所在的栅格会置为255，此作用体现在比如线段上下移动半个像素，会使画出的高斯图有权重变化
                int lutIdx = (int) (dist * lutPixelsPerMeter + .5);//+0.5是为了四舍五入，当进行int截断的时候，会容易使像素点个数
                                                                   //减一，导致误认为该点在中心点的高斯范围内
                if (lutIdx < lut.length) {
                    int idx = iy*width + ix;
                   // data[idx] = (BYTE) max(data[idx]&0xff, lut.plut[lutIdx]);
					data[idx] = (BYTE) max(data[idx]&0xff, lut.vtlut.at(lutIdx));//取大值，保证每个矩形范围内值都为255，属于扩展出的
                }                                                                //区域，值为高斯递减的

                x += metersPerPixel;
            }

            y += metersPerPixel;
        }
    }

    int GridMap::clamp(int v, int min, int max)  //钳制
    {
        if (v > max)
            return max;
        if (v < min)
            return min;
        return v;
    }

	void GridMap::maxConvolution(int k, GridMap& gm)   //没用上
    {
        copy(gm);

        // first, do rows.
        for (int y = 0; y < height; y++)
            maxConvolution(data, y*width, width, k, gm.data, y*width);

        BYTE *ptmp = new BYTE[height];
        BYTE *ptmp2 = new BYTE[height];
        for (int x = 0; x < width; x++) {

            // copy column into 1D array for locality
            for (int y = 0; y < height; y++)
                ptmp[y] = gm.data[y*width+x];

            maxConvolution(ptmp, 0, height, k, ptmp2, 0);

            // copy back
            for (int y = 0; y < height; y++)
                gm.data[y*width+x] = ptmp2[y];
        }

        return ;
    }

    void GridMap::maxConvolution(BYTE in[], int in_offset, int width, int k, BYTE out[], int out_offset)  //没用上
    {
        int runlength = 0;
        int runvalue = 0;

        for (int x = 0; x < width; x++) {
            int v = 0;
            int cnt = min(k, width - x);

            // if the last convolution step was all zeros, and the
            // right-most position is a zero, then we know the result
            // for this pixel will be zero.
            int right = in[in_offset+x+cnt-1] & 0xff; // right-most value
            if (right == runvalue) {
                runlength++;
            } else {
                runlength = 1;
                runvalue = right;
            }

            if (runlength >= k) {
                out[out_offset + x] = (BYTE) runvalue;
                continue;
            }

            // do the dumb convolution.
            for (int i = 0;  i < cnt; i++) {
                v = max(v, in[in_offset + x + i] & 0xff);
                if (v == 255)
                    break;
            }

            out[out_offset + x] = (BYTE) v;
        }
    }

	     //没用上
    void GridMap::maxConvolution(BYTE in[], int in_offset, int width, int k, BYTE out[], int out_offset, int hist[],int histlength)
    {
        // clear histogram
        for (int i = 0; i < histlength; i++)
            hist[i] = 0;

        int upperbound = 0;

        // warm up histogram with first k-1 values
        for (int x = 0; x < k; x++) {
            int v = in[in_offset + x] & 0xff;
            hist[v]++;
            upperbound = max(upperbound, v);
        }

        // compute convolution by adding right most element, removing
        // left most element, then finding max.

        for (int x = k; x < width + k; x++) {

            // tighten upper bound (possible because the old max might
            // have been removed on a previous iteration)
            while (upperbound > 0 && hist[upperbound]==0)
                upperbound--;

            // produce output; our upperbound is now the actual max.
            out[out_offset + x - k] = (BYTE) upperbound;

            // add right most
            if (x < width) {
                int v = in[in_offset + x] & 0xff;
                hist[v]++;

                if (v > upperbound)
                    upperbound = v;
            }

            // remove left most
            int v = in[in_offset + x - k] & 0xff;
            hist[v]--;
        }
    }

    void GridMap::decimateMax(int factor,GridMap& gm)
    {
        int newwidth = (width + factor - 1) / factor;
        int newheight = (height + factor - 1) / factor + 1;

        gm.makePixels(x0, y0, newwidth, newheight, metersPerPixel*factor, (BYTE) 0, true);
        gm.defaultFill = defaultFill;

        // loop over input rows
        for (int iy = 0; iy < height; iy++) {
            // which output row should this affect?
            int oy = iy/factor;

            // loop over input columns
            for (int ix = 0; ix < width; ix+= factor) {
                // destination column?
                int ox = ix/factor;

                int maxv = 0;
                int maxdx = min(factor, width - ox*factor);

                // loop over the input pixels that all map to (ox,oy)
                for (int dx = 0; dx < maxdx; dx++)
                    maxv = max(maxv, data[iy*width + ox*factor + dx]&0xff);

                // update output column
                gm.data[oy*gm.width + ox] = (BYTE) max(gm.data[oy*gm.width + ox]&0xff, maxv);
            }
        }

        return ;
    }

    /** Make a new gridmap where each cell at (x,y) is the max of the
     * 2x2 square at (x,y).
     **/
    void GridMap::max4(GridMap &gm)
    {
        copy(gm);
        for (int iy = 0; iy + 1 < height; iy++) {        //从左下角开始，四个格子里最大的数赋给左下角的格子
            for (int ix = 0; ix + 1 < width; ix++) {
                int v = max(max(data[iy*width+ix]&0xff,
                                          data[iy*width+ix+1]&0xff),
                                 max(data[(iy+1)*width+ix]&0xff,
                                          data[(iy+1)*width+ix+1]&0xff));
                gm.data[iy*width+ix] = (BYTE) v;
            }

            // fix up right most pixel, which is a function of just
            // itself and the pixel below it.
            if (iy + 1 < height) {
                int ix = width - 1;
                gm.data[iy*width+ix] = (BYTE) max(data[iy*width+ix]&0xff,     //每行最右边的格子，和它上面的格子比
                                                       data[(iy+1)*width+ix]&0xff);
            }
        }

        // fix up bottom row, where each pixel is just a function of
        // itself and the pixel to its right.               //剩最上面的一行
        for (int ix = 0; ix + 1 < width; ix++) {
            int iy = height - 1;

            gm.data[iy*width+ix] = (BYTE) max(data[iy*width+ix]&0xff,             //和右边的格子比
                                                   data[iy*width+ix+1]&0xff);

        }
        return ;
    }

    /** Count how many of the points land in a grid cell whose value is at least 'thresh' **/
    int GridMap::scoreThreshold(vector<Pose> &points,               //没用上
                              double tx, double ty, double theta,
                              int thresh)
    {
        double ct = cos(theta), st = sin(theta);
        double pixelsPerMeter = 1.0 / metersPerPixel;

        int score = 0;

        for (int pidx = 0; pidx < points.size(); pidx++) {

            // project the point
            Pose p = points.at(pidx);
            double x = p.x*ct - p.y*st + tx;
            double y = p.x*st + p.y*ct + ty;

            // (ix0, iy0) are the coordinates in distdata that
            // correspond to scores[0]. It's the (nominal) upper-left
            // corner of our search window.

            int ix = ((int) ((x - x0)*pixelsPerMeter));
            int iy = ((int) ((y - y0)*pixelsPerMeter));

            if (ix >= 0 && ix < width && iy >=0 && iy < height)
                if ((data[iy*width + ix]&0xff) > thresh)
                    score++;

        }
        return score;
    }

	 void GridMap::scores3D(vector<Pose> &points,
                                  double tx0, int txDim,   //tx0,ty0，是位姿矩形的左下角坐标
                                  double ty0, int tyDim,
                                  double theta0, double thetaStep, int thetaDim,
                                   Pose& priorxyt, double **pinv,vector<IntArray2D*> &vtIntArray2D)
    {
		
        IntArray2D* scores = NULL;

        for (int i = 0; i < thetaDim; i++) {
            double theta = theta0 + i*thetaStep;//按照角度的增量，每个角度，都将所有的点在扩展矩形内匹配完毕

		    //在固定角度下，将机器人坐标下的激光点，在扩展矩形内，按照机器人位姿，进行地图匹配
            scores = scores2D(points, tx0, txDim, ty0, tyDim, theta, priorxyt, pinv);

			vtIntArray2D.push_back(scores);//+=15°，增量为1°，应该会压栈30个
        }

        return ;
    }



 	 void GridMap::HistogramFilter_scores3D(vector<Pose> &points,
                                  double tx0, 
                                  double ty0, 
                                  double theta0, double thetaStep, int thetaDim,
                                  vector<IntArray2D*> &vtIntArray2D)
    {
		
        IntArray2D* scores = NULL;

        for (int i = 0; i < thetaDim; i++) {
            double theta = theta0 + i*thetaStep;//按照角度的增量，每个角度，都将所有的点在扩展矩形内匹配完毕

		    //在固定角度下，将机器人坐标下的激光点，在扩展矩形内，按照机器人位姿，进行地图匹配
            scores = HistogramFilter_scores2D(points, tx0,ty0,theta);

			vtIntArray2D.push_back(scores);//+=15°，增量为1°，应该会压栈30个
        }

        return ;
    }
    double GridMap::score(vector<Pose> &points,
                     double tx, double ty, double theta, Pose& prior, double **pinv)
    {
        double ct = cos(theta), st = sin(theta);
        double pixelsPerMeter = 1.0 / metersPerPixel;

        double score = 0;

        for (int pidx = 0; pidx < points.size(); pidx++) {

            // project the point
            Pose p = points.at(pidx);
            double x = p.x*ct - p.y*st + tx;
            double y = p.x*st + p.y*ct + ty;

            // (ix0, iy0) are the coordinates in distdata that
            // correspond to scores[0]. It's the (nominal) upper-left
            // corner of our search window.

            int ix = ((int) ((x - x0)*pixelsPerMeter));
            int iy = ((int) ((y - y0)*pixelsPerMeter));

            if (ix >= 0 && ix < width && iy >=0 && iy < height)
                score += data[iy*width + ix]&0xff;

        }

        if (pinv != NULL) {
			double ex = (tx - prior.x), ey = (ty - prior.y);
			double et = MathUtil::mod2pi(theta - prior.theta);

            double cost = ex*ex*pinv[0][0] + 2*ex*ey*pinv[0][1] + 2*ex*et*pinv[0][2] +
                ey*ey*pinv[1][1] + 2*ey*et*pinv[1][2] +
                et*et*pinv[2][2];

            score -= cost;
        }

        return score;
    }

	/*
	IntArray2D* GridMap::scores2D(vector<Pose> &points,
                               double tx0, int txDim,
                               double ty0, int tyDim,
                               double theta,  Pose& priorxyt, double **pinv)
    {
        IntArray2D *scores = new IntArray2D(tyDim, txDim);

		double prior[2];

		prior[0] = priorxyt.x;
		prior[1] = priorxyt.y;


        double ct = cos(theta), st = sin(theta);
        double pixelsPerMeter = 1.0 / metersPerPixel;

        // Evaluate each point for a fixed rotation but variable
        // translation
        for (int pidx = 0; pidx < points.size(); pidx++) {

            // project the point
        	//\BF\C9\C4ܽ\AB\BC\A4\B9\E2\B5\E3\B1\E4Ϊȫ\BE\D6\D7\F8\B1ꣿ\A3\BF
            Pose ps = points.at(pidx);
            double x = ps.x*ct - ps.y*st + tx0;
            double y = ps.x*st + ps.y*ct + ty0;

            // (ix0, iy0) are the coordinates in distdata that
            // correspond to scores[0]. It's the (nominal) upper-left
            // corner of our search window.
            //\D4\DAդ\B8\F1\D7\F8\B1\EA\D6\D0λ\D6\C3
            int ix0 = ((int) ((x - x0)*pixelsPerMeter));
            int iy0 = ((int) ((y - y0)*pixelsPerMeter));

            // compute the intersection of the box
            // (ix0,iy0)->(ix0+ixdim-1,iy0+iydim-1) and the box
            // (0,0)->(width-1, height-1). This will be our actual
            // search window.
            int bx0 = max(ix0, 0);
            int by0 = max(iy0, 0);

            int bx1 = min(ix0 + txDim - 1, width-1);
            int by1 = min(iy0 + tyDim - 1, height-1);

            for (int iy = by0; iy <= by1; iy++) {

                int sy = iy - iy0; // y coordinate in scores[]

                for (int ix = bx0; ix <= bx1; ix++) {

                    int lutval = data[iy*width + ix]&0xff;
                    int sx = ix - ix0;

                    scores->plusEquals(sy, sx, lutval);
                }
            }
        }

        if (pinv != NULL) {

            double et = 0;//MathUtil.mod2pi(theta - prior[2]);

            double priorcost0 = et*et*pinv[2][2];

            for (int sy = 0; sy < scores->Getdim1(); sy++) {
                double ey = ty0 + sy*metersPerPixel - prior[1];

                double priorcost1 = ey*ey*pinv[1][1] + 2*ey*et*pinv[1][2] + priorcost0;

                for (int sx = 0; sx < scores->Getdim2(); sx++) {
                    double ex = tx0 + sx*metersPerPixel - prior[0];

                    double cost = ex*ex*pinv[0][0] + 2*ex*ey*pinv[0][1] + 2*ex*et*pinv[0][2] +  priorcost1;

                    scores->plusEquals(sy, sx, (int) -cost);
                }
            }
        }

        return scores;
    }

	*/
IntArray2D* GridMap::scores2D(vector<Pose> &points,
                               double tx0, int txDim,     //tx0,ty0，是位姿矩形的左下角坐标
                               double ty0, int tyDim,
                               double theta,  Pose& priorxyt, double **pinv)
    {
        IntArray2D *scores = new IntArray2D(tyDim, txDim);//vs,vd，tyDim和txDim表示了矩阵的大小


		double prior[2];

		prior[0] = priorxyt.x;
		prior[1] = priorxyt.y;


        double ct = cos(theta), st = sin(theta);
        double pixelsPerMeter = 1.0 / metersPerPixel;

        // Evaluate each point for a fixed rotation but variable
        // translation
		int pointnum = points.size(); //激光点
		vector<int> tmpdata;
        for (int pidx = 0; pidx < points.size(); pidx++) {  //每一个点都拿出来比较一下

            // project the point
        	//\BF\C9\C4ܽ\AB\BC\A4\B9\E2\B5\E3\B1\E4Ϊȫ\BE\D6\D7\F8\B1ꣿ\A3\BF
            Pose ps = points.at(pidx);  //取点
            
            //x,y，为每个激光点，在最初始位姿下的世界坐标，用这个x,y，可以求出ix0，iy0，即在初始位姿下，
            //此激光点在data地图中的位置，ix0,iy0很有用，由于每个激光点都在位姿矩阵中循环一遍，所以，激光点在
            //循环时，在data中的位置变化，和在位姿矩阵中变化一样，所以，将激光点的data位置，减去ix0,iy0，就是激光点
            //在data中的变化量，变化量对应到位姿矩阵，就是此时位姿的行列序号
            double x = ps.x*ct - ps.y*st + tx0;//旋转平移，此时位姿为最初始位姿，即搜索的矩形的最左下角
            double y = ps.x*st + ps.y*ct + ty0;//tx0,ty0，是位姿矩阵的左下角坐标

            // (ix0, iy0) are the coordinates in distdata that
            // correspond to scores[0]. It's the (nominal) upper-left
            // corner of our search window.
            //\D4\DAդ\B8\F1\D7\F8\B1\EA\D6\D0λ\D6\C3
            int ix0 = ((int) ((x - x0)*pixelsPerMeter));//x0,y0，是data地图的左下角坐标
            int iy0 = ((int) ((y - y0)*pixelsPerMeter));//ix0,iy0，是此激光点，按照最左下角的位姿，得到的data地图中的位置

            // compute the intersection of the box
            // (ix0,iy0)->(ix0+ixdim-1,iy0+iydim-1) and the box
            // (0,0)->(width-1, height-1). This will be our actual
            // search window.
            int bx0 = max(ix0, 0);
            int by0 = max(iy0, 0);

            int bx1 = min(ix0 + txDim - 1, width-1);//这里确定的范围，是同一个激光点，在位姿矩形内的不同位置，对应的data中的位置
            int by1 = min(iy0 + tyDim - 1, height-1);//有可能在某个位姿下，此激光点超出了data范围，这时，只需计算在data范围内的点就行
                                                     //对应的位姿在位姿矩形内的位置，由sy，sx给出

			//这里，sy，sx是在扩展后的矩形内，点相对矩形左下角（ix0,iy0）的位置，
			//矩形大小是一定的，但是有可能超出了原来的地图范围，所以用bx0,by0,bx1,by1
			//约束了一下，从地图范围内的点开始循环，但是每个点的相对位置还是和（ix0,iy0）比较，这点要清楚
			//即，data是地图中的值，而score里向量元素序号表示的值，是相对值
            for (int iy = by0; iy <= by1; iy++) {  //防止超出地图范围，用by0，by1，iy和ix是求data序号的

                int sy = iy - iy0; // 相对位置，以扩展后的位姿矩形的左下角为基准，不管左下角位姿下，激光点有没有出data边界，因为是相对位置
                                   //sy，sx是求此时位姿在位姿矩阵中的相对序号的，比如0,0，就是最左下角位姿，0，1，就是右边的位姿

                for (int ix = bx0; ix <= bx1; ix++) {

				    //这里使用的data，是drawScan函数，最后把gm的信息传过来的，所以当前类中的dgm，就是proccessScan扫描完轮廓并在data中画图后，在drawScan中给过来的
                    int lutval = data[iy*width + ix]&0xff; //以某个点为机器人世界坐标时，算出来的坐标转换后，相应点的位置上已经有的值
					//_RPT1(_CRT_WARN,"lutval= %d,\n",lutval);
                    int sx = ix - ix0;

					//在这个函数中，vs向量相应位置上加的值不一定一样，在真正边界上，值为255，在高斯区域内，是递减的，所以，值越大，表示对应的越准
                    scores->plusEquals(sy, sx, lutval);//更新vs和vd，如果data的相应位置上有值，即lutval>0，则表示在当前的角度和机器人位姿下，此点
                                                       //匹配上了，vs相应位置值加上data中的值，vd相应位置值加一，表示又有一个点匹配上了
                }
            }
        }
		/*int maxscores = -9999999999999;
		for (int i= 0;i<scores->Getdim1();i++)
			for(int j=0;j<scores->Getdim2();j++)
			{

				if(scores->get(i,j)>maxscores)
					maxscores = scores->get(i,j);
			}

			_RPT3(_CRT_WARN,"theta =%f,pointnum= %d,maxscores = %d,\n",theta,pointnum,maxscores);*/

        if (pinv != NULL) {

            double et = 0;//MathUtil.mod2pi(theta - prior[2]);

            double priorcost0 = et*et*pinv[2][2];

            for (int sy = 0; sy < scores->Getdim1(); sy++) {
                double ey = ty0 + sy*metersPerPixel - prior[1];

                double priorcost1 = ey*ey*pinv[1][1] + 2*ey*et*pinv[1][2] + priorcost0;

                for (int sx = 0; sx < scores->Getdim2(); sx++) {
                    double ex = tx0 + sx*metersPerPixel - prior[0];

                    double cost = ex*ex*pinv[0][0] + 2*ex*ey*pinv[0][1] + 2*ex*et*pinv[0][2] +  priorcost1;

                    scores->plusEquals(sy, sx, (int) -cost);
                }
            }
        }

        return scores;//在一个角度下，全部的点循环匹配结束后，返回
    }
 IntArray2D* GridMap::HistogramFilter_scores2D(vector<Pose> &points,
                                                      double tx0, 
                                                      double ty0,
                                                      double theta)
    {
        int tyDim = 9;
		int txDim = 9;
        IntArray2D *scores = new IntArray2D(tyDim, txDim);//vs,vd，tyDim和txDim表示了矩阵的大小


        double ct = cos(theta), st = sin(theta);
        double pixelsPerMeter = 1.0 / metersPerPixel;
		
        for (int pidx = 0; pidx < points.size(); pidx++) {  //每一个点都拿出来比较一下

            // project the point
        	//\BF\C9\C4ܽ\AB\BC\A4\B9\E2\B5\E3\B1\E4Ϊȫ\BE\D6\D7\F8\B1ꣿ\A3\BF
            Pose ps = points.at(pidx);  //取点
            
            //x,y，为每个激光点，在最初始位姿下的世界坐标，用这个x,y，可以求出ix0，iy0，即在初始位姿下，
            //此激光点在data地图中的位置，ix0,iy0很有用，由于每个激光点都在位姿矩阵中循环一遍，所以，激光点在
            //循环时，在data中的位置变化，和在位姿矩阵中变化一样，所以，将激光点的data位置，减去ix0,iy0，就是激光点
            //在data中的变化量，变化量对应到位姿矩阵，就是此时位姿的行列序号
            double x = ps.x*ct - ps.y*st + tx0;//旋转平移，此时位姿为最初始位姿，即搜索的矩形的最左下角
            double y = ps.x*st + ps.y*ct + ty0;//tx0,ty0，是位姿矩阵的左下角坐标

            // (ix0, iy0) are the coordinates in distdata that
            // correspond to scores[0]. It's the (nominal) upper-left
            // corner of our search window.
            //\D4\DAդ\B8\F1\D7\F8\B1\EA\D6\D0λ\D6\C3
            int ix0 = ((int) ((x - x0)*pixelsPerMeter));//x0,y0，是data地图的左下角坐标
            int iy0 = ((int) ((y - y0)*pixelsPerMeter));//ix0,iy0，是此激光点，按照最左下角的位姿，得到的data地图中的位置

            // compute the intersection of the box
            // (ix0,iy0)->(ix0+ixdim-1,iy0+iydim-1) and the box
            // (0,0)->(width-1, height-1). This will be our actual
            // search window.
            int bx0 = max(ix0, 0);
            int by0 = max(iy0, 0);

            int bx1 = min(ix0 + txDim - 1, width-1);//这里确定的范围，是同一个激光点，在位姿矩形内的不同位置，对应的data中的位置
            int by1 = min(iy0 + tyDim - 1, height-1);//有可能在某个位姿下，此激光点超出了data范围，这时，只需计算在data范围内的点就行
                                                     //对应的位姿在位姿矩形内的位置，由sy，sx给出

			//这里，sy，sx是在扩展后的矩形内，点相对矩形左下角（ix0,iy0）的位置，
			//矩形大小是一定的，但是有可能超出了原来的地图范围，所以用bx0,by0,bx1,by1
			//约束了一下，从地图范围内的点开始循环，但是每个点的相对位置还是和（ix0,iy0）比较，这点要清楚
			//即，data是地图中的值，而score里向量元素序号表示的值，是相对值
            for (int iy = by0; iy <= by1; iy++) {  //防止超出地图范围，用by0，by1，iy和ix是求data序号的

                int sy = iy - iy0; // 相对位置，以扩展后的位姿矩形的左下角为基准，不管左下角位姿下，激光点有没有出data边界，因为是相对位置
                                   //sy，sx是求此时位姿在位姿矩阵中的相对序号的，比如0,0，就是最左下角位姿，0，1，就是右边的位姿

                for (int ix = bx0; ix <= bx1; ix++) {

				    //这里使用的data，是drawScan函数，最后把gm的信息传过来的，所以当前类中的dgm，就是proccessScan扫描完轮廓并在data中画图后，在drawScan中给过来的
                    int lutval = data[iy*width + ix]&0xff; //以某个点为机器人世界坐标时，算出来的坐标转换后，相应点的位置上已经有的值
					//_RPT1(_CRT_WARN,"lutval= %d,\n",lutval);
                    int sx = ix - ix0;

					//在这个函数中，vs向量相应位置上加的值不一定一样，在真正边界上，值为255，在高斯区域内，是递减的，所以，值越大，表示对应的越准
                    scores->plusEquals(sy, sx, lutval);//更新vs和vd，如果data的相应位置上有值，即lutval>0，则表示在当前的角度和机器人位姿下，此点
                                                       //匹配上了，vs相应位置值加上data中的值，vd相应位置值加一，表示又有一个点匹配上了
                }
            }
        }
	

        return scores;//在一个角度下，全部的点循环匹配结束后，返回
    }


void GridMap::pfscores(vector<Pose> &points,vector<particles> &pfswarm)//pfswarm:vector<pf_class> pfswarm:pose weight tempweight,prop
{
	int num = pfswarm.size();
	double pixelsPerMeter = 1.0 / metersPerPixel;
	for(int i=0;i<num;i++)
	{
		double ct = cos(pfswarm[i].pose.theta);
		double st = sin(pfswarm[i].pose.theta);

		double dx0 = pfswarm[i].pose.x;
		double dy0 = pfswarm[i].pose.y;
			
		int pointnum = points.size();
		for(int pidx = 0;pidx < pointnum;pidx++)
		{
			Pose ps = points.at(pidx);
			double x = ps.x*ct - ps.y*st + dx0;
			double y = ps.x*st + ps.y*ct + dy0;

			int ix = ((int)((x - x0)*pixelsPerMeter));
			int iy = ((int)((y - y0)*pixelsPerMeter));

			if(ix>=0 && ix<width && iy>=0 && iy<height)//if the point is in the submap
			{
				double lutval = (double)(data[iy*width + ix]&0xff);
				pfswarm[i].weight += lutval;
			}
		}
	}
	return;
}


 /** Get 8-connected nodes around point xy with cost under maxCost
      * @param xy      - Continuous-domain point around which to find
      *                  connected nodes
      * @param maxCost - Maximum cost for which a node can be considered valid
      **/
    int GridMap::getConnectedWithin(Pose& xy, int maxCost,BYTE **pRes, int &nResLength)  //不懂
    {
        double pixelsPerMeter = 1.0 / metersPerPixel;
        int px = (int) ((xy.x - x0) * pixelsPerMeter);
        int py = (int) ((xy.y - y0) * pixelsPerMeter);
        if (px < 0 || px >= width || py < 0 || py >= height)   //点xy越界，返回
        {
        	return 0;
        }

        UnionFindSimple *puf = new UnionFindSimple(width*height);

        // We connect the following points around pixel 'o':
        // . . x
        // . o x
        // . x x

        // To avoid checking bounds per neighbor, we adjust the
        // bounds of x and y
        for (int y=1; y < height-1; y++)    //没有越界，从第二行循环，上下各空一行
        {
            for (int x=0; x < width-1; x++)  //每行里循环，右边空一个
            {
                int a = y*width + x;        //点位置

                if ((((int) data[a]) & 0xFF) > maxCost)     //值>mycost,略过
                    continue;

                int b;
                // x+1, y-1
                b = (y-1)*width + (x+1);      //右下，x+1,y-1
                if ((((int) data[b]) & 0xFF) <= maxCost)
                    puf->connectNodes(puf->getRepresentative(a),
                                    puf->getRepresentative(b));

                // x+1, y
                b = (y)*width + (x+1);
                if ((((int) data[b]) & 0xFF) <= maxCost)
                    puf->connectNodes(puf->getRepresentative(a),
                                    puf->getRepresentative(b));

                // x+1, y+1
                b = (y+1)*width + (x+1);
                if ((((int) data[b]) & 0xFF) <= maxCost)
                    puf->connectNodes(puf->getRepresentative(a),
                                    puf->getRepresentative(b));

                // x  , y+1
                b = (y+1)*width + (x);
                if ((((int) data[b]) & 0xFF) <= maxCost)
                    puf->connectNodes(puf->getRepresentative(a),
                                    puf->getRepresentative(b));
            }
        }

        // get id of union around robot
        int cid = puf->getRepresentative(py*width + px);
        
		bool relocate = false;
	
		if(puf->getSetSize(cid) <= 1)
		{
			int j = 2;
			for(int tmpx = px - j ; tmpx <= px + j ;tmpx++)
			{
				for(int tmpy = py - j; tmpy <= py + j ;tmpy++)
				{
					cid = puf->getRepresentative(py*width + px);
					if(puf->getSetSize(cid) > 1)
					{
						printf("xy0 = %f %f\n",xy.x,xy.y);
						xy.x = x0+tmpx * metersPerPixel;
						xy.y = y0+tmpy * metersPerPixel;
						printf("xy1 = %f %f\n",xy.x,xy.y);
						relocate = true;
						break;
					}
				}
				if(relocate)
				{
					break;
				}
			}	
		
			if(!relocate)
			{
				delete puf;
        		return 0;
			}
		}

        *pRes = new BYTE[width*height];
        for (int i=0; i < width*height; i++)
        {
            if (puf->getRepresentative(i) == cid)
                *((*pRes)+i) = (BYTE) 0x1;
			else
				*((*pRes)+i) = 0x0;
			nResLength++;
        }

		delete puf;
        return 1;
    }


    int GridMap::getConnectedWithin(int x,int y, int maxCost,BYTE **pRes, int &nResLength)  //不懂
    {
        double pixelsPerMeter = 1.0 / metersPerPixel;
        int px = x;
        int py = y;

	

        if (px < 0 || px >= width || py < 0 || py >= height)
        {
        	return 0;
        }

        UnionFindSimple *puf = new UnionFindSimple(width*height);

        // We connect the following points around pixel 'o':
        // . . x
        // . o x
        // . x x

        // To avoid checking bounds per neighbor, we adjust the
        // bounds of x and y
        for (int y=1; y < height-1; y++)
        {
            for (int x=0; x < width-1; x++)
            {
                int a = y*width + x;

                if ((((int) data[a]) & 0xFF) > maxCost)
                    continue;

                int b;
                // x+1, y-1
                b = (y-1)*width + (x+1);
                if ((((int) data[b]) & 0xFF) <= maxCost)
                    puf->connectNodes(puf->getRepresentative(a),
                                    puf->getRepresentative(b));

                // x+1, y
                b = (y)*width + (x+1);
                if ((((int) data[b]) & 0xFF) <= maxCost)
                    puf->connectNodes(puf->getRepresentative(a),
                                    puf->getRepresentative(b));

                // x+1, y+1
                b = (y+1)*width + (x+1);
                if ((((int) data[b]) & 0xFF) <= maxCost)
                    puf->connectNodes(puf->getRepresentative(a),
                                    puf->getRepresentative(b));

                // x  , y+1
                b = (y+1)*width + (x);
                if ((((int) data[b]) & 0xFF) <= maxCost)
                    puf->connectNodes(puf->getRepresentative(a),
                                    puf->getRepresentative(b));
            }
        }

        // get id of union around robot
        int cid = puf->getRepresentative(py*width + px);

        // only return a non-null result if cells other
        // than the xy's are reachable
        if (puf->getSetSize(cid) <= 1)
        {
			delete puf;
        	return 0;
        }

        *pRes = new BYTE[width*height];
        for (int i=0; i < width*height; i++)
        {
            if (puf->getRepresentative(i) == cid)
                *((*pRes)+i) = (BYTE) 0x1;
			else
				*((*pRes)+i) = 0x0;
			nResLength++;
        }

		delete puf;
        return 1;
    }


	/*
     void filterFactoredCenteredMax(float fhoriz[], float fvert[])
    {
        BYTE r[] = new BYTE[data.length];

        // do horizontal
        for (int y = 0; y < height; y++) {
            siasun.image.SigProc.convolveSymmetricCenteredMax(data, y*width, width, fhoriz, r, y*width);
        }

        // do vertical
        byte tmp[] = new byte[height];  // the column before convolution
        byte tmp2[] = new byte[height]; // the column after convolution.

        for (int x = 0; x < width; x++) {

            // copy the column out for locality.
            for (int y = 0; y < height; y++)
                tmp[y] = r[y*width + x];

            SigProc.convolveSymmetricCenteredMax(tmp, 0, height, fvert, tmp2, 0);

            for (int y = 0; y < height; y++)
                r[y*width + x] = tmp2[y];
        }

        this.data = r;
    }*/

    /** Get pixel center from cell indices
     *    * This reference function incurs a performance hit due to object creation
     * @param int array with the cell indices to lookup
     * @return double array for pixel center or null if invalid input
     **/
   bool GridMap::getPixelCenter(int indices[],int length,double *pxRes, double *pyRes)
    {
        if (indices == NULL || length != 2)
            return 0;

		//不空，长度等于2
         if(getPixelCenter(indices[0], indices[1],pxRes,pyRes))
			return 1;
		 else
			 return 0;
    }

    /** Get pixel center from cell indices
     *    * This reference function incurs a performance hit due to object creation
      * @param ix cell index in width dimension
      * @param iy cell index in height dimension
      * @return double array for pixel center or null if invalid input
      **/
   bool GridMap::getPixelCenter(int ix, int iy, double *pxRes, double *pyRes)
    {
        if (ix >= 0 && ix < width && iy >= 0 && iy < height) {    //从像素图上转化到地图上坐标
            *pxRes = x0 + (ix + 0.5) * metersPerPixel;
            *pyRes = y0 + (iy + 0.5) * metersPerPixel;

            return 1;
        } else
            return 0;
    }

    /** Get cell indices from pixel center
     *    * This reference function incurs a performance hit due to object creation
      * @param double array with cell center in local frame
      * @return int array with cell indices or null if invalid input
      **/
    bool GridMap::getIndices(double p[],int length, int *pix,int *piy)
    {
        if (p == NULL || length != 2)
            return false;
        else
		{
            if(getIndices(p[0], p[1], pix,piy))
				return true;
			else 
				return false;

		}
    }

    /** Get cell indices from pixel center
     *    * This reference function incurs a performance hit due to object creation
      * @param px cell center in local frame (x dimension)
      * @param py cell center in local frame (y dimension)
      * @return int array with cell indices or null if invalid input
      **/
    bool GridMap::getIndices(double px, double py, int *pix,int *piy)
    {
        int ix = (int) ((px - x0) / metersPerPixel);    //从地图上转化为像素上的相对坐标
        int iy = (int) ((py - y0) / metersPerPixel);

        if (ix >= 0 && ix < width && iy >= 0 && iy < height)
		{
			*pix = ix;
			*piy = iy;
            return true;

		}
        else
		{
            return false;
		}
    }
    void GridMap::incrementSaturate(double px, double py)  //没用上
    {
        int ix = (int) ((px - x0) / metersPerPixel);
        int iy = (int) ((py - y0) / metersPerPixel);

        if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
            int v = data[iy*width+ix]&0xff;
            if (v < 255)
                data[iy*width+ix] = (BYTE) (v+1);
        }
    }

    /** if condition is satisfied, set vtrue. else vfalse. **/
    void GridMap::thresholdGreaterThanOrEqual(int thresh, BYTE vtrue, BYTE vfalse)  //没用上
    {
        for (int i = 0; i < width*height; i++) {
            int v = data[i]&0xff;
            if (v >= thresh)
                data[i] = vtrue;
            else
                data[i] = vfalse;
        }
    }


	void GridMap::BorderMap(GridMap &bordmap)     //边界地图
{

	unsigned char *m_pborder1;	
	unsigned char *m_pborder2;

         //给bordmap赋值
	    bordmap.x0 = x0;
        bordmap.y0 = y0;
        bordmap.metersPerPixel = metersPerPixel;
        bordmap.width = width;
        bordmap.height = height;
        bordmap.data = new BYTE[width*height];
        bordmap.defaultFill = defaultFill;

        for (int i = 0; i < width*height; i++)   //全部点循环，置为0
            bordmap.data[i] = 0;     


	if (height>0 && width>0)                  //有宽和高
	{
		m_pborder1 = new unsigned char[height*width];
		memset(m_pborder1, 0, height*width);
	}	
	if (height>0 && width>0)
	{
		m_pborder2 = new unsigned char[height*width];
		memset(m_pborder2, 0, height*width);
	}	
	for (int i = 0; i<width;i= i+1)   //行循环
	{
		for(int j = 1; j<height;j++)   //列循环，不含最下面一行
		{
				if(data[j*width+i] != data[(j-1)*width+i] )//不等于它下面的点
				{
					if(data[j*width+i] == 0xFF)        //有符号数-1，无符号数FF
					{
						m_pborder1[j*width+i] = 254;
					}
					else
					{
						m_pborder1[(j-1)*width+i] = 254;

					}
				}						
		}
	}

	for (int j = 0; j<height;j= j+1)
	{
		for(int i = 1; i<width;i++)
		{
				if(data[j*width+i] != data[j*width+i-1] )  //跟左边的比
				{
					if(data[j*width+i] == 0xFF)
					{
						m_pborder2[j*width+i] = 254;
					}
					else
					{
						m_pborder2[j*width+i-1] = 254;

					}
				}						
		}
	}
	for (int j = 0; j<height;j= j+1)
	{
		for(int i = 0; i<width;i++)
		{
				if( m_pborder1[j*width+i]==254 || m_pborder2[j*width+i]==254 )  //两个图比较，有一个是边界，就把此点设置为-1
				{
					bordmap.data[j*width+i]=0xFF;
				}
				else
				{
					bordmap.data[j*width+i]=0;
				}
		}
	}

	if(m_pborder1!=NULL)
	{
		delete m_pborder1;
		m_pborder1 = NULL;
	}
	if(m_pborder2!=NULL)
	{
		delete m_pborder2;
		m_pborder2 = NULL;
	}

}


void GridMap::RotateMap(double theta)   //没用上
{
	double tmpx,tmpy,x1,y1,tmpx0,tmpx1;
	int index_x,index_y;
	unsigned char *tmpmap;
	tmpmap = new BYTE[width*height];
	for(int i = 0;i < width*height;i ++)
	{
		tmpmap[i] = data[i];
		data[i] = 0;
	}
	

	for(int i = 0;i < height;i++)
	{
		for(int j = 0;j < width;j++)
		{
			if(tmpmap[i*width+j] != 0)
			{
				tmpx = x0 + (j + .5)*metersPerPixel;
               	tmpy = y0 + (i + .5)*metersPerPixel;
				x1 = tmpx*cos(theta) - tmpy * sin(theta);
				y1 = tmpx*sin(theta) + tmpy * cos(theta);

				index_x = (int)((x1 - x0) / metersPerPixel);
				index_y = (int)((y1 - y0) / metersPerPixel);
				if(index_x < width && index_x > 0 &&  index_y < height && index_y > 0)
				{ 
					data[index_y * width + index_x] = tmpmap[i*width+j];
				}
			}
		}
	}
	if(tmpmap!=NULL)
	{
		delete tmpmap;
		tmpmap = NULL;
	}
}

