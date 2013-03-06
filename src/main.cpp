#include "main.h"
#include <videoInput.h>

HINSTANCE hInst;

videoInput VI;
unsigned char *g_pRGBOriginalSample;
unsigned char *g_pRGBProcesedSample;
unsigned char *g_ostatnie;

unsigned char *g_pRGBBack;
unsigned char *g_temp;
int g_iBackWidth;
int g_iBackHeight;

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
      
      //Za³aduj Palete barw jesli jest
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
				//Nie obs³ugiwane
				break;
        case 4:
        //nie Obs³ugiwane
        break;
        case 8: //Skala szaroœci
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
    //Odczytanie nag³ówka
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
      v[2] /= 256; //Pobranie dynamiki obrazu 1 lub 2 bajty na próbkê
      
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
   
      // Wczytanie sk³adowych
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
        for(i=0;i<img->cmp[0]->dy;i++)
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

void DoSomeThingWithSample(unsigned char* pRGBSrcSample,unsigned char* pRGBDsrSample,int iWidth, int iHeight)
{
  for(int y=0;y<iHeight;y++) //Pêtla po wszystkich wierszach obrazu
  {
    for(int x=0;x<iWidth;x++) //Pêtla po wszystkich kolumnach obrazu
    {
      pRGBDsrSample[(y*iWidth+x)*3+0] = pRGBSrcSample[(y*iWidth+x)*3+0]; //Przepisanie s³adowej B
      pRGBDsrSample[(y*iWidth+x)*3+1] = pRGBSrcSample[(y*iWidth+x)*3+1]; //Przepisanie s³adowej G
      pRGBDsrSample[(y*iWidth+x)*3+2] = pRGBSrcSample[(y*iWidth+x)*3+2]; //Przepisanie s³adowej R

	  g_ostatnie[(y*iWidth+x)*3+0] = pRGBSrcSample[(y*iWidth+x)*3+0]; //Przepisanie s³adowej B
	  g_ostatnie[(y*iWidth+x)*3+1] = pRGBSrcSample[(y*iWidth+x)*3+1]; //Przepisanie s³adowej G
	  g_ostatnie[(y*iWidth+x)*3+2] = pRGBSrcSample[(y*iWidth+x)*3+2]; //Przepisanie s³adowej R

      int R = pRGBDsrSample[(y*iWidth+x)*3+2];
      int G = pRGBDsrSample[(y*iWidth+x)*3+1];
      int B = pRGBDsrSample[(y*iWidth+x)*3+0];
      float Y = 0.299f*R+0.587f*G+0.114f*B;
      float U = -0.147f*R-0.289f*G+0.437f*B;
      float V = 0.615f*R-0.515f*G+0.100f*B;
	}
  }

   for(int i=0; i<iHeight;i++)
	  {
		for(int j=0; j<iWidth;j++)
		{
		
					int R = pRGBDsrSample[(i*iWidth+j)*3+2];
					int G = pRGBDsrSample[(i*iWidth+j)*3+1];
					int B = pRGBDsrSample[(i*iWidth+j)*3+0];
					
					float Y = 0.299f*R+0.587f*G+0.114f*B;
					float U = -0.147f*R-0.289f*G+0.437f*B;
					float V = 0.615f*R-0.515f*G+0.100f*B;

					if((U<0)&&(V<0))
					{
					
					g_ostatnie[(i*iWidth+j)*3+0] = g_pRGBBack[(i*g_iBackWidth+j)*3+0]; //0;
					g_ostatnie[(i*iWidth+j)*3+1] = g_pRGBBack[(i*g_iBackWidth+j)*3+1]; // 0;
					g_ostatnie[(i*iWidth+j)*3+2] = g_pRGBBack[(i*g_iBackWidth+j)*3+2]; //0;

					}
					else
					{
					
					g_ostatnie[(i*iWidth+j)*3+0] = pRGBDsrSample[(i*iWidth+j)*3+0];
					g_ostatnie[(i*iWidth+j)*3+1] = pRGBDsrSample[(i*iWidth+j)*3+1];
					g_ostatnie[(i*iWidth+j)*3+2] =pRGBDsrSample[(i*iWidth+j)*3+2];
					}

		}

	  }

}

int abs(int liczba)
{
	if(liczba<0)
		{
			liczba=-liczba;
			return liczba;
	}
	else
		return liczba;

}

void xInitCamera(int iDevice, int iWidth, int iHeight)
{
  int width,height,size;
  VI.setupDevice(iDevice,iWidth,iHeight); // 320 x 240 resolution

  width = VI.getWidth(iDevice);
  height = VI.getHeight(iDevice);
  size = VI.getSize(iDevice); // size to initialize `
  //VI.setUseCallback(true);
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

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_CREATE:
    xInitCamera(0,320,240); //Aktywacja pierwszej kamery do pobierania obrazu o rozdzielczoœci 320x240
	g_pRGBOriginalSample = new unsigned char [320*240*3]; //Allokacja buffora pamieci na originalne próbki obrazu
    g_pRGBProcesedSample = new unsigned char [320*240*3]; //Allokacja buffora pamieci na przetworzone próbki obrazu
	g_ostatnie = new unsigned char [320*240*3]; //Allokacja buffora pamieci na przetworzone próbki obrazu

	g_pRGBBack = ReadPpmFromFile("blank.ppm",g_iBackWidth, g_iBackHeight); //Wczyt obrazu z pliku
	
    SetTimer(hwnd,GRINBOX_ID_TIMER_GET_FRAME,40,NULL); //Ustawienie minutnika na co 40 milisekund

    break;
  case WM_PAINT:
    xDisplayBmpOnWindow(hwnd,0,0,g_pRGBOriginalSample,320,240); //Narysowanie naszego buffora próbek obrazu na okienku
    xDisplayBmpOnWindow(hwnd,320,0,g_pRGBProcesedSample,320,240); //Narysowanie naszego buffora próbek obrazu na okienku
      break;
    break;
  case WM_TIMER:
    switch(wParam)
    {
    case GRINBOX_ID_TIMER_GET_FRAME:
      xGetFrame(g_pRGBOriginalSample);  //Pobranie 1 ramki obrazu z kamery
      DoSomeThingWithSample(g_pRGBOriginalSample,g_pRGBProcesedSample,320,240); //Wywo³anie procedury przetwarzaj¹cej obraz
      xDisplayBmpOnWindow(hwnd,0,0,g_pRGBOriginalSample,320,240); //Wyœwitlenie 1 ramki obrazu na okienku
     // xDisplayBmpOnWindow(hwnd,320,0,g_pRGBProcesedSample,320,240); //Wyœwitlenie tej samej ramki obrazu na okienku w innym miejscu
	  xDisplayBmpOnWindow(hwnd,320,0,g_ostatnie,320,240); //Wyœwitlenie tej samej ramki obrazu na okienku w innym miejscu
      break;
    }
    break;

  case WM_COMMAND:
	  if(HIWORD(wParam)==0)
		  switch(LOWORD(wParam))
	  {
		  case ID_MENU_EXIT:
			  PostQuitMessage(0);
			  break;
		  case ID_MENU_ABOUT:
			  MessageBox(0,TEXT("Program napisany przez Kamila Czempiñskiego i Marcina No¿yñskiego w ramach ko³a multimedialnego \nWersja rozwojowa"),TEXT("O programie"),MB_OK);
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
					ofn.lpstrFilter = "PPM Files\0*.ppm\0BMP Files\0*.bmp\0";
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

					if(!strstr(ofn.lpstrFile,".bmp")&&!strstr(ofn.lpstrFile,".ppm"))
						MessageBox(0,TEXT("No to zdecyduj siê czy chcesz otworzyæ zanim bezsensownie klikasz plik->otwórz :D"),TEXT("WTF?"),MB_OK);

					break;
			  }
	    }
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
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC)WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(GRINBOX_MAIN_ICON));
  wc.hCursor = NULL;//LoadCursor(NULL, IDC_HAND);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc.lpszMenuName = MAKEINTRESOURCE(GRINBOX_MAIN_MENU);
  wc.lpszClassName = GRINBOX_APP_CLASS_NAME;

  //Rejestracja klasy okna
  if ( !RegisterClass( &wc ) ) return( FALSE );

  // Tworzenie g³ównego okna aplikacji
  HWND hWnd = CreateWindow(
    GRINBOX_APP_CLASS_NAME,
    GRINBOX_APP_WINDOW_NAME,
    WS_OVERLAPPEDWINDOW,
    0,
    0,
	656,
    299,
    NULL,
    NULL,
    hInstance,
    NULL);

  // Sprawdzenie czy okienko siê utworzy³o
  if ( hWnd == NULL ) return( FALSE );

  // Pokazanie okinka na ekranie
  ShowWindow(hWnd,iCmdShow);

  MSG msg;

  // G³ówna pêtla komunikatów
  while( GetMessage( &msg, NULL, 0, 0) )
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}
