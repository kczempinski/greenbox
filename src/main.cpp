#include "main.h"
#include <videoInput.h>

#include <gl\GL.h>
#include <gl\GLU.h>
//#include <gl\glaux.h>

#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )
//#pragma comment( lib, "glaux.lib" )

#include <vector>
#include <stdlib.h>
#include <time.h>



HINSTANCE hInst;

videoInput VI;
unsigned char *g_pRGBOriginalSample;
unsigned char *g_pRGBProcesedSample;
unsigned char *g_last;
unsigned char *g_temp;


unsigned int ***g_colorDetection;
bool g_bIsCalibrating;
bool g_bIsGetFrame;

int g_iWidth = 640;
int g_iHeight = 480;
int g_iCalibFrameSize =15;
int g_iCalibFrameThick = 3;

#define GL_BGRA 0x80E1
std::vector<unsigned int> data(g_iWidth*g_iHeight*3);	

unsigned char *g_pRGBBack;
int g_iBackWidth;
int g_iBackHeight;

bool combined=true;
bool back;
bool cam;
bool filtered;

bool black=0;
bool white=0;
bool still=0;
bool gl=1;

int maskSize=0;		
int mask=1;

HWND hWnd, hWnd2;

void xEndCalibrate()
{
  unsigned int **ppHist;
  int iMax = 0;

  //Allokacje Pamieci
  ppHist = new unsigned int*[256];
  for(int i=0;i<256;i++)
  {
    ppHist[i] = new unsigned int[256];
  }

  //Normalize Histogram
  for(int i=0;i<256;i++)
  {
    for(int j=0;j<256;j++)
    {
      ppHist[i][j] = 0;
      for(int k=0;k<256;k++)
      {
        ppHist[i][j] += g_colorDetection[k][i][j];
      }
      if(iMax<ppHist[i][j]) iMax = ppHist[i][j];
    }
  }

  //Deallokacje Pamieci
  for(int i=0;i<256;i++)
  {
    delete ppHist[i];
  }
  delete ppHist;

}

unsigned char* ReadBmpFromFile(char* szFileName,int &riWidth, int &riHeight)
{
	BITMAPFILEHEADER     bfh;
	BITMAPINFOHEADER     bih;

	int                i,j,h,v,lev,l,ls;
	unsigned char*     buff = NULL;  

  unsigned char* p_palette = NULL;
  unsigned short n_colors = 0;

  unsigned char* pRGBBuffer;

  FILE* hfile = fopen(szFileName,"rb");

	if(hfile!=NULL)
	{
    fread(&bfh,sizeof(bfh),1,hfile);
		if(!(bfh.bfType != 0x4d42 || (bfh.bfOffBits < (sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)))) )
		{
      fread(&bih,sizeof(bih),1,hfile);
			v   = bih.biWidth;
			h   = bih.biHeight;
			lev = bih.biBitCount;
			
      riWidth = v;
      riHeight = h;
      pRGBBuffer = new unsigned char [riWidth*riHeight*3]; //Zaalokowanie odpowiedniego buffora obrazu
      
      //Za�aduj Palete barw jesli jest
      if((lev==1)||(lev==4)||(lev==8))
      {
        n_colors = 1<<lev;
        p_palette = new unsigned char[4*n_colors];
        fread(p_palette,4*n_colors,1,hfile);
      }

      fseek(hfile,bfh.bfOffBits,SEEK_SET);

			buff = new unsigned char[v*4];

			switch(lev)
			{
				case 1:
				//Nie obs�ugiwane
				break;
        case 4:
        //nie Obs�ugiwane
        break;
        case 8: //Skala szaro�ci
        ls = (v+3)&0xFFFFFFFC;
				for(j=(h-1);j>=0;j--)
				{
          fread(buff,ls,1,hfile);
          for(i=0,l=0;i<v;i++) 
          {
            pRGBBuffer[((j*riWidth)+i)*3+2] = p_palette[(buff[i]<<2)+2];//R
            pRGBBuffer[((j*riWidth)+i)*3+1] = p_palette[(buff[i]<<2)+1];//G
            pRGBBuffer[((j*riWidth)+i)*3+0] = p_palette[(buff[i]<<2)+0];//B
          }
				};
				break;
				case 24:
				//bitmapa RGB
				ls = (v*3+3)&0xFFFFFFFC;
				for(j=(h-1);j>=0;j--)
				{
					//x_fread(hfile,buff,ls);
          fread(buff,ls,1,hfile);
					for(i=0,l=0;i<v;i++,l+=3) 
					{
						pRGBBuffer[((j*riWidth)+i)*3+0] = buff[l+0];
						pRGBBuffer[((j*riWidth)+i)*3+1] = buff[l+1];
						pRGBBuffer[((j*riWidth)+i)*3+2] = buff[l+2];
					};
				};
				break;
				case 32:
			  // RGBA bitmap 
				for(j=(h-1);j>=0;j--)
				{
          fread(buff,v*4,1,hfile);
					for(i=0,l=0;i<v;i++,l+=4) 
					{
						pRGBBuffer[((j*riWidth)+i)*3+0] = buff[l+0];
						pRGBBuffer[((j*riWidth)+i)*3+1] = buff[l+1];
						pRGBBuffer[((j*riWidth)+i)*3+2] = buff[l+2];
					}
				};
				break;
			};
			delete buff; 
   if(p_palette) delete p_palette;

		}
	}
	return pRGBBuffer;
}

unsigned char* ReadPpmFromFile(char* szFileName,int &riWidth, int &riHeight)
{
  FILE* hfile = fopen(szFileName,"rb");
  int i,j,k,fb;
  int v[3];
  unsigned char buff[10];
  unsigned char* pRGBBuffer;

  if(hfile!=NULL)
  {
    //Odczytanie nag��wka
    fread(buff,3,1,hfile);
    if((buff[0]=='P')&&(buff[1]=='6')&&(isspace(buff[2])))
    {
      for(i=0;i<3;i++)
      {
        fread(&buff[0],1,1,hfile);
        if(buff[0]=='#') 
        {
          while((buff[0]!=13)&&(buff[0]!=10)) fread(&buff[0],1,1,hfile);
          fread(&buff[0],1,1,hfile);
        }
        k=0;
        do
        {
          k++;
          fread(&buff[k],1,1,hfile);
        }
        while(!isspace(buff[k]));
        buff[k+1] = 0;
        v[i] = atoi((char*)buff);
      }
      v[2] /= 256; //Pobranie dynamiki obrazu 1 lub 2 bajty na pr�bk�
      
      //Inicjacja obrazu
      if((v[0]*3)%4!=0)
      {
        fb = 4-(((v[0])*3)%4);
      }
      else
      {
        fb = 0;
      }
      riWidth = v[0]+fb;
      riHeight = v[1];
      pRGBBuffer = new unsigned char [riWidth*riHeight*3]; //Zaalokowanie odpowiedniego buffora obrazu
   
      // Wczytanie sk�adowych
      if(v[2]==0)
      {
        for(i=0;i<v[1];i++)
        {
          for(j=0;j<v[0];j++)
          {
            fread(&pRGBBuffer[((riHeight-i-1)*(riWidth)+j)*3+2],1,1,hfile);
            fread(&pRGBBuffer[((riHeight-i-1)*(riWidth)+j)*3+1],1,1,hfile);
            fread(&pRGBBuffer[((riHeight-i-1)*(riWidth)+j)*3+0],1,1,hfile);
          }
          
          /*
          for(j=0;j<fb;j++)
          {
            pTmp[j*3+0] = 0;
            pTmp[j*3+1] = 0;
            pTmp[j*3+2] = 0;
          }
          pTmp+=fb*3;//*/
        }
      }
      else
      {
        //16 bit samples - Nie wspierane
        /*
        for(i=0;i<img->cmp[0]->dj;i++)
          for(j=0;j<img->cmp[0]->dx;j++)
          {
            x_fread(hfile,&buff,6);
            img->cmp[0]->pel[i][j] = buff[0]<<8 & buff[1];
            img->cmp[1]->pel[i][j] = buff[2]<<8 & buff[3];
            img->cmp[2]->pel[i][j] = buff[4]<<8 & buff[5];
          }
          //*/
        delete pRGBBuffer;
        return NULL;
      }
  }
  return pRGBBuffer;
 }
 return NULL;
}

void ResetHistogram(int iHeight, int iWidth)
{
	for(double Y=0; Y<255;Y++)
	  {
		for(int U=0; U<256;U++)
			{
				for(int V=0; V<256;V++)					
					g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V] = 0;
			}
		}
}

void DoSomeThingWithSample(unsigned char* pRGBSrcSample,unsigned char* pRGBDsrSample,int iWidth, int iHeight)
{
  for(int y=0;y<iHeight;y++) //P�tla po wszystkich wierszach obrazu
  {
    for(int x=0;x<iWidth;x++) //P�tla po wszystkich kolumnach obrazu
    {
      pRGBDsrSample[(y*iWidth+x)*3+0] = pRGBSrcSample[(y*iWidth+x)*3+0]; //Przepisanie s�adowej B
      pRGBDsrSample[(y*iWidth+x)*3+1] = pRGBSrcSample[(y*iWidth+x)*3+1]; //Przepisanie s�adowej G
      pRGBDsrSample[(y*iWidth+x)*3+2] = pRGBSrcSample[(y*iWidth+x)*3+2]; //Przepisanie s�adowej R

	}
  }

   for(int i=0; i<iHeight;i++)
	  {
		for(int j=0; j<iWidth;j++)
		{

			g_temp[(i*iWidth+j)*3+0]= pRGBDsrSample[(i*iWidth+j)*3+0];
			g_temp[(i*iWidth+j)*3+1]= pRGBDsrSample[(i*iWidth+j)*3+1];
			g_temp[(i*iWidth+j)*3+2]= pRGBDsrSample[(i*iWidth+j)*3+2];

			//mask
			if(maskSize && i>maskSize && j>maskSize && j<iWidth-maskSize && i<iHeight-maskSize)
			{
				g_temp[(i*iWidth+j)*3+0]=g_temp[(i*iWidth+j)*3+0]/mask;
				g_temp[(i*iWidth+j)*3+1]=g_temp[(i*iWidth+j)*3+1]/mask;
				g_temp[(i*iWidth+j)*3+2]=g_temp[(i*iWidth+j)*3+2]/mask;
				for (int ii=-maskSize;ii<=maskSize;ii++)
				{
					for (int jj=-maskSize;jj<=maskSize;jj++)
					{
						if (ii || jj)
						{
						g_temp[(i*iWidth+j)*3+0]=g_temp[(i*iWidth+j)*3+0]+pRGBDsrSample[((i+ii)*iWidth+(j+jj))*3+0]/mask;
						g_temp[(i*iWidth+j)*3+1]=g_temp[(i*iWidth+j)*3+1]+pRGBDsrSample[((i+ii)*iWidth+(j+jj))*3+1]/mask;
						g_temp[(i*iWidth+j)*3+2]=g_temp[(i*iWidth+j)*3+2]+pRGBDsrSample[((i+ii)*iWidth+(j+jj))*3+2]/mask;
						}
					}
				}

			}
							int R = g_temp[(i*iWidth+j)*3+2];
							int G = g_temp[(i*iWidth+j)*3+1];
							int B = g_temp[(i*iWidth+j)*3+0];

							float Y = 0.299f*R+0.587f*G+0.114f*B;
							float U = -0.147f*R-0.289f*G+0.437f*B;
							float V = 0.615f*R-0.515f*G+0.100f*B;


					
					g_last[(i*iWidth+j)*3+0] = pRGBDsrSample[(i*iWidth+j)*3+0];
					g_last[(i*iWidth+j)*3+1] = pRGBDsrSample[(i*iWidth+j)*3+1];
					g_last[(i*iWidth+j)*3+2] = pRGBDsrSample[(i*iWidth+j)*3+2];
					
					
					
					 if(g_bIsCalibrating)
					 {
							//Przetwarzaj �rodek ekranu
							if((i>=g_iWidth/2-g_iCalibFrameSize)&&(i<=g_iWidth/2+g_iCalibFrameSize)&&(j>=g_iHeight/2-g_iCalibFrameSize)&&(j<=g_iHeight/2+g_iCalibFrameSize))
							{
							  //Rysuj ramk� na �rodku ekranu
							  if(!((i>=g_iWidth/2-g_iCalibFrameSize+g_iCalibFrameThick)&&(i<=g_iWidth/2+g_iCalibFrameSize-g_iCalibFrameThick)&&(j>=g_iHeight/2-g_iCalibFrameSize+g_iCalibFrameThick)&&(j<=g_iHeight/2+g_iCalibFrameSize-g_iCalibFrameThick)))
							  {
								g_last[(i*iWidth+j)*3+0] = 0;
								g_last[(i*iWidth+j)*3+1] = 0;
								g_last[(i*iWidth+j)*3+2] =255;
							  }
							  //Dodaj ramke do naszego histogramu
							  if(g_bIsGetFrame)
							  {
								g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V] += 1;
							  }
							}
						  }
						  else
						  {
							if(g_colorDetection[(unsigned char)Y][(unsigned char)U][(unsigned char)V]>1)
							{
								if (still)
								{
									g_last[(i*iWidth+j)*3+0] = g_pRGBBack[(i*g_iBackWidth+j)*3+0]; //0;
								g_last[(i*iWidth+j)*3+1] = g_pRGBBack[(i*g_iBackWidth+j)*3+1]; // 0;
								g_last[(i*iWidth+j)*3+2] = g_pRGBBack[(i*g_iBackWidth+j)*3+2]; //0;
								}
								if (gl){
									g_last[(i*iWidth+j)*3+0] = data[(2*i*g_iBackWidth+j)*3+0]; //0;
								g_last[(i*iWidth+j)*3+1] = data[(2*i*g_iBackWidth+j)*3+1]; // 0;
								g_last[(i*iWidth+j)*3+2] = data[(2*i*g_iBackWidth+j)*3+2]; //0;
								}
								if (black)
								{
									g_last[(i*iWidth+j)*3+0] = 0;
								g_last[(i*iWidth+j)*3+1] = 0;
								g_last[(i*iWidth+j)*3+2] = 0;
								}
								if (white)
								{
									g_last[(i*iWidth+j)*3+0] = 255;
								g_last[(i*iWidth+j)*3+1] = 255;
								g_last[(i*iWidth+j)*3+2] = 255;
								}
								
							}
		
						}

						}
					 }
	if(g_bIsGetFrame)
    g_bIsGetFrame = false;
 }

void xInitCamera(int iDevice, int iWidth, int iHeight)
{
  int width,height,size;
  VI.setupDevice(iDevice,iWidth,iHeight); // 320 x 240 resolution

  width = VI.getWidth(iDevice);
  height = VI.getHeight(iDevice);
  size = VI.getSize(iDevice); // size to initialize `
}

void xGetFrame(unsigned char* pRGBSample)
{
  int device;
  device = 0;
  if(VI.isFrameNew(device))
  {
    VI.getPixels(device,pRGBSample, false, false);
  }
}

void xDisplayBmpOnWindow(HWND hWnd,int iX, int iY, unsigned char* pRGBSample, int iWidth, int iHeight)
{
  HDC hDC = GetDC(hWnd);
  HDC hDCofBmp = CreateCompatibleDC(hDC);

  HBITMAP hBmp = CreateCompatibleBitmap(hDC,iWidth,iHeight);

  SelectObject(hDCofBmp,hBmp);

  BITMAPINFOHEADER biBmpInfoHeader;

  biBmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
  biBmpInfoHeader.biWidth = iWidth;
  biBmpInfoHeader.biHeight = iHeight;
  biBmpInfoHeader.biPlanes = 1;
  biBmpInfoHeader.biBitCount = 24;//32
  biBmpInfoHeader.biCompression = BI_RGB;
  biBmpInfoHeader.biSizeImage = 0;
  biBmpInfoHeader.biXPelsPerMeter = 0;
  biBmpInfoHeader.biYPelsPerMeter = 0;
  biBmpInfoHeader.biClrUsed = 0;
  biBmpInfoHeader.biClrImportant = 0;
  SetDIBits(hDCofBmp,hBmp,0,iWidth*iHeight,pRGBSample,(BITMAPINFO*)&biBmpInfoHeader,DIB_RGB_COLORS);

  BitBlt(hDC,iX,iY,iWidth,iHeight,hDCofBmp,0,0,SRCCOPY);

  SelectObject(hDCofBmp,0);
  DeleteObject(hBmp);
  DeleteDC(hDCofBmp);

  ReleaseDC(hWnd,hDC);
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)
{
	if(height==0)
	{
		height=1;
	}

	glViewport(0,0,g_iWidth,g_iHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f,(GLfloat)g_iWidth/(GLfloat)g_iHeight,0.1f,1000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

int DrawGLScene(GLvoid)
{

static GLfloat rot=0.0;
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glLoadIdentity();
glTranslatef(0,0,-10);



glRotatef(rot=rot+0.001*1000  , -rot-0.002*1000 ,   rot=rot+0.0012*1000   ,   rot=rot+0.005*1000   );


glEnable(GL_DEPTH_TEST);

GLUquadricObj *quadric6;
quadric6 = gluNewQuadric();
glColor3d(1,1,0);
glTranslatef(0, 0, 0);
gluSphere( quadric6 , 1 , 36 , 18 );
gluDeleteQuadric(quadric6); 



GLUquadricObj *quadric;
quadric = gluNewQuadric();
glColor3d(0,0.1,0.2);
glTranslatef(-2, 0, 0);
gluSphere( quadric , 0.75 , 36 , 18 );
gluDeleteQuadric(quadric); 

GLUquadricObj *quadric5;
quadric5 = gluNewQuadric();
glColor3d(0,0.3,0.2);
glTranslatef(5, 0, 0);
gluSphere( quadric5 , 0.9 , 36 , 20);
gluDeleteQuadric(quadric5); 



GLUquadricObj *quadric7;
quadric7 = gluNewQuadric();
glColor3d(0.2,0.1,0);
glTranslatef(0, 3, 3);
gluSphere( quadric7 , 0.2 , 30 , 10 );
gluDeleteQuadric(quadric7); 

return 1;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
HWND hWndButton;
  switch (message)
  {
  case WM_CREATE:

	hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Calibrate Mode"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,40,120,30,hwnd,(HMENU)GRINBOX_CALIB_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
    hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Detect Mode"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,80,120,30,hwnd,(HMENU)GRINBOX_DETECT_BUTTON,GetModuleHandle(NULL),NULL);   ////////////
    hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Get Frame"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,120,120,30,hwnd,(HMENU)GRINBOX_GETFRAME_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
	hWndButton = CreateWindow(TEXT("BUTTON"),TEXT("Reset Calibration"),BS_FLAT | WS_VISIBLE | WS_CHILD,g_iWidth+20,160,120,30,hwnd,(HMENU)GRINBOX_RESET_BUTTON,GetModuleHandle(NULL),NULL);  ///////////
	 
    xInitCamera(0,g_iWidth,g_iHeight); //Aktjwacja pierwszej kamery do pobierania obrazu o rozdzielczo�ci 320x240

	g_pRGBOriginalSample = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na originalne pr�bki obrazu
    g_pRGBProcesedSample = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na przetworzone pr�bki obrazu
	g_last = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na przetworzone pr�bki obrazu
	g_temp = new unsigned char [g_iWidth*g_iHeight*3]; //Allokacja buffora pamieci na przetworzone pr�bki obrazu
	g_pRGBBack = ReadPpmFromFile("blank.ppm",g_iBackWidth, g_iBackHeight); //Wczyt obrazu z pliku
	
    g_colorDetection = new unsigned int** [256]; //Allokacja buffora pami�ci na histogram obrazu
    for(int i=0;i<256;i++)
    {
      g_colorDetection[i] = new unsigned int* [256];
      for(int j=0;j<256;j++)
      {
        g_colorDetection[i][j] = new unsigned int [256];
        for(int k=0;k<256;k++)
        {
          g_colorDetection[i][j][k] = 0;
        }
      }
    }

    SetTimer(hwnd,GRINBOX_ID_TIMER_GET_FRAME,1000/15,NULL); //Ustawienie minutnika na co 40 milisekund

	 g_bIsCalibrating = false;
    g_bIsGetFrame = false;

    break;
  case WM_PAINT:
    xDisplayBmpOnWindow(hwnd,0,0,g_pRGBProcesedSample,g_iWidth,g_iHeight); //Narysowanie naszego buffora pr�bek obrazu na okienku
      break;
    break;
  case WM_TIMER:
    switch(wParam)
    {
    case GRINBOX_ID_TIMER_GET_FRAME:
      xGetFrame(g_pRGBOriginalSample);  //Pobranie 1 ramki obrazu z kamery
	  
      DoSomeThingWithSample(g_pRGBOriginalSample,g_pRGBProcesedSample,g_iWidth,g_iHeight); //Wywo�anie procedury przetwarzaj�cej obraz
	  if (combined) xDisplayBmpOnWindow(hwnd,0,0,g_last,g_iWidth,g_iHeight); 
	  if (back) xDisplayBmpOnWindow(hwnd,0,0,g_pRGBBack,g_iWidth,g_iHeight); 
	  if (cam) xDisplayBmpOnWindow(hwnd,0,0,g_pRGBOriginalSample,g_iWidth,g_iHeight); 
	  if (filtered) xDisplayBmpOnWindow(hwnd,0,0,g_temp,g_iWidth,g_iHeight); 
      break;
    }
    break;

  case WM_COMMAND:
	  if(HIWORD(wParam)==0)
		  switch(LOWORD(wParam))
	  {
			case GRINBOX_CALIB_BUTTON:
				g_bIsCalibrating = true;
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_UNCHECKED);
				combined=true;
				filtered=cam=back=false;
				break;
			case GRINBOX_DETECT_BUTTON:
				xEndCalibrate();
				g_bIsCalibrating = false;
				break;
			case GRINBOX_GETFRAME_BUTTON:
				g_bIsGetFrame = true;
				break;
			case GRINBOX_RESET_BUTTON:
				ResetHistogram(480,640);
				break;
			case ID_BACK:
				back=true;
				filtered=cam=combined=false;
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_UNCHECKED);
				ShowWindow(hWnd2,SW_SHOW);
				break;
			case ID_CAM:
				cam=true;
				filtered=combined=back=false;
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_UNCHECKED);
				break;
			case ID_COMB:
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_UNCHECKED);
				combined=true;
				filtered=cam=back=false;
				break;
			case ID_FILTERED:
				CheckMenuItem(GetMenu(hwnd),ID_BACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_CAM,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_COMB,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_FILTERED,MF_CHECKED);
				filtered=true;
				combined=cam=back=false;
				break;
			case ID_MASK_OFF:
				maskSize=0;
				mask=((1+2*maskSize)*(1+2*maskSize));
				CheckMenuItem(GetMenu(hwnd),ID_MASK_OFF,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_3,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_5,MF_UNCHECKED);
				break;
			case ID_MASK_3:
				maskSize=1;
				mask=((1+2*maskSize)*(1+2*maskSize));
				CheckMenuItem(GetMenu(hwnd),ID_MASK_OFF,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_3,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_5,MF_UNCHECKED);
				break;
			case ID_MASK_5:
				maskSize=2;
				mask=((1+2*maskSize)*(1+2*maskSize));
				CheckMenuItem(GetMenu(hwnd),ID_MASK_OFF,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_3,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_MASK_5,MF_CHECKED);
				break;
			case ID_STILL:
				CheckMenuItem(GetMenu(hwnd),ID_STILL,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_GL,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_BLACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_WHITE,MF_UNCHECKED);
				still=true;
				black=false;
				white=false;
				gl=false;
				break;
			case ID_GL:
				CheckMenuItem(GetMenu(hwnd),ID_STILL,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_GL,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_BLACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_WHITE,MF_UNCHECKED);
				still=false;
				black=false;
				white=false;
				gl=true;
				break;
			case ID_WHITE:
				CheckMenuItem(GetMenu(hwnd),ID_STILL,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_GL,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_BLACK,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_WHITE,MF_CHECKED);
				still=false;
				black=false;
				white=true;
				gl=false;
				break;
			case ID_BLACK:
				CheckMenuItem(GetMenu(hwnd),ID_STILL,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_GL,MF_UNCHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_BLACK,MF_CHECKED);
				CheckMenuItem(GetMenu(hwnd),ID_WHITE,MF_UNCHECKED);
				still=false;
				black=true;
				white=false;
				gl=false;
				break;
		  case ID_MENU_EXIT:
			  PostQuitMessage(0);
			  break;
		  case ID_MENU_ABOUT:
			  MessageBox(0,TEXT("Coded by\nKamil Czempinski and Marcin Nozynski\n\nversion 0.98.1 Beta (pre-release)"),TEXT("About"),MB_OK);//skad Marcin te wersje bierzesz? :D
			  break;
		  case ID_MENU_LOADBACKGROUND:
			  {
				  OPENFILENAME ofn;       // common dialog box structure
					char szFile[260];       // buffer for file name
					HANDLE hf;              // file handle

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hwnd;
					ofn.lpstrFile = szFile;
					// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
					// use the contents of szFile to initialize itself.
					ofn.lpstrFile[0] = '\0';
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "BMP Files\0*.bmp\0PPM Files\0*.ppm\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					// Display the Open dialog box. 

					if (GetOpenFileName(&ofn)==TRUE) 
						hf = CreateFile(ofn.lpstrFile, 
						GENERIC_READ,
						0,
						(LPSECURITY_ATTRIBUTES) NULL,
						CREATE_NEW,
						FILE_ATTRIBUTE_NORMAL,
						(HANDLE) NULL);

					
					if(strstr(ofn.lpstrFile,".ppm"))
					g_pRGBBack = ReadPpmFromFile(ofn.lpstrFile,g_iBackWidth, g_iBackHeight);

					if(strstr(ofn.lpstrFile,".bmp"))	  
					g_pRGBBack = ReadBmpFromFile(ofn.lpstrFile,g_iBackWidth, g_iBackHeight);

					break;
			  }
	    }
	  break;
	  case WM_CLOSE:
		  PostQuitMessage(0);
		  break;
	  case WM_SIZE:
		  ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));
		  break;
  case WM_DESTROY:
    //DeleteObject(hBitmap);

	  delete g_pRGBOriginalSample; //Deallokacja buffora pamieci na originalne pr�bki obrazu 
    delete g_pRGBProcesedSample; //Deallokacja buffora pamieci na przetworzone pr�bki obrazu 
    

    //Deallokacja buffora pami�ci na histogram obrazu
    for(int i=0;i<256;i++)
    {
      for(int j=0;j<256;j++)
      {
        delete g_colorDetection[i][j];
      }
      delete g_colorDetection[i];
    }
    delete g_colorDetection;

    PostQuitMessage(0);
    break;
  }

  return DefWindowProc (hwnd, message, wParam, lParam);
}


LRESULT CALLBACK WndProc2 (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
HWND hWndButton;
  switch (message)
  {
  case WM_CREATE:
    break;
  case WM_PAINT:
      break;
    break;
  case WM_TIMER:
    
    break;

  case WM_COMMAND:
	 
	  break;
	  case WM_CLOSE:
		  PostQuitMessage(0);
		  break;
	  case WM_SIZE:
		  ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));
		  break;
  case WM_DESTROY:
    //DeleteObject(hBitmap);
    PostQuitMessage(0);
    break;
  }

  return DefWindowProc (hwnd, message, wParam, lParam);
}


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
  //Definicja klasy okna
  WNDCLASS wc;
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.lpfnWndProc = (WNDPROC)WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(GRINBOX_MAIN_ICON));
  wc.hCursor = NULL;//LoadCursor(NULL, IDC_HAND);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc.lpszMenuName = MAKEINTRESOURCE(GRINBOX_MAIN_MENU);
  wc.lpszClassName = GRINBOX_APP_CLASS_NAME;

   WNDCLASS wc2;
  wc2.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc2.lpfnWndProc = (WNDPROC)WndProc2;
  wc2.cbClsExtra = 0;
  wc2.cbWndExtra = 0;
  wc2.hInstance = hInstance;
  wc2.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(GRINBOX_MAIN_ICON));
  wc2.hCursor = NULL;//LoadCursor(NULL, IDC_HAND);
  wc2.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc2.lpszMenuName = NULL;
  wc2.lpszClassName = "Open GL";

  //Rejestracja klasy okna
  if ( !RegisterClass( &wc ) ) return( FALSE );
  if ( !RegisterClass( &wc2 ) ) return( FALSE );

  // Tworzenie g��wnego okna aplikacji
   hWnd = CreateWindow(
    GRINBOX_APP_CLASS_NAME,
    GRINBOX_APP_WINDOW_NAME,
    WS_OVERLAPPEDWINDOW,
    0,
    0,
	g_iWidth+180,
    g_iHeight+60,
    NULL,
    NULL,
    hInstance,
    NULL);

   hWnd2 = CreateWindow(
    "Open GL",
    GRINBOX_APP_GL_WINDOW,
    WS_OVERLAPPEDWINDOW,
    0,
    0,
	g_iWidth+160,
    g_iHeight,
    NULL,
    NULL,
    hInstance,
    NULL);

  // Sprawdzenie czy okienko si� utworzy�o
  if ( hWnd == NULL ) return( FALSE );

  static PIXELFORMATDESCRIPTOR pfd=
  {
	  sizeof(PIXELFORMATDESCRIPTOR),
	  1,
	  PFD_DRAW_TO_WINDOW |
	  PFD_SUPPORT_OPENGL |
	  PFD_DOUBLEBUFFER,
	  PFD_TYPE_RGBA,
	  16,
	  0, 0, 0, 0, 0, 0,
	  0,
	  0,
	  0,
	  0, 0, 0, 0,
	  16,
	  0,
	  0,
	  PFD_MAIN_PLANE,
	  0,
	  0, 0, 0
  };
  
  HDC hDC = NULL;
  HGLRC hRC = NULL;
  GLuint PixelFormat;
 
  if(!(hDC=GetDC(hWnd2))) return 0;
  if(!(PixelFormat=ChoosePixelFormat(hDC,&pfd))) return 0;
  if(!(SetPixelFormat(hDC,PixelFormat,&pfd))) return 0;
  if(!(hRC=wglCreateContext(hDC))) return 0;
  if(!wglMakeCurrent(hDC,hRC)) return 0;

  // Pokazanie okinka na ekranie
  ShowWindow(hWnd,SW_SHOW);
  SetForegroundWindow(hWnd);
  SetFocus(hWnd);

  MSG msg;

  BOOL done = false;
  while(!done) {
	  if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
		  if (msg.message==WM_QUIT) {done= true;}
		  else
		  {
			  TranslateMessage(&msg);
			  DispatchMessage(&msg);
		  }
	  }
	  else
	  {
		  if(gl) {DrawGLScene();
		 glReadBuffer(GL_FRONT);
		glReadPixels(0,0,g_iWidth,g_iHeight,GL_BGR_EXT,GL_UNSIGNED_INT,&data[0]);
		SwapBuffers(hDC);}

		  

	  }
  }



  // G��wna p�tla komunikat�w
  while( GetMessage( &msg, NULL, 0, 0) )
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  if (hRC) {
	  !wglMakeCurrent(NULL,NULL);
	  !wglDeleteContext(hRC);
	  hRC=NULL;
  }
  if (hDC && !ReleaseDC(hWnd,hDC)) {hDC=NULL;}
  if (hWnd && !DestroyWindow(hWnd)) {hWnd=NULL;}
  return 0;
}
