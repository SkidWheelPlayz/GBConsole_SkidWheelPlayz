#include "vid.h"
#include <stdio.h>
#include <string.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_rotozoom.h>
#include <SDL2/SDL_ttf.h>

#define INIT_FLAG_NOT 0
#define INIT_FLAG_PARTIAL 1
#define INIT_FLAG_FULL 2

#define SURFACE_TYPE 0
#define MAX_IMAGE_CACHE_FILENAME_SIZE 256

// Data
static char vid_isInitFlag = INIT_FLAG_NOT;
static SDL_Window* vid_window = NULL;
static SDL_Surface* vid_scrMain = NULL;
static char vid_cachedImageFilename[MAX_IMAGE_CACHE_FILENAME_SIZE];
static SDL_Surface* vid_cachedImage = NULL;
static int vid_cachedFontSize = -1;
static TTF_Font* vid_cachedFont = NULL;
static SDL_Surface* vid_shade = NULL;

// Setup and initialize the Video interface
int vid_init()
{
	//already initialized?
	if(vid_isInitFlag == INIT_FLAG_FULL) return 0;
	
	//initialize SDL
	if(vid_isInitFlag == INIT_FLAG_NOT) {
		int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
		if(SDL_Init(SDL_INIT_VIDEO) < 0) {
			fprintf(stderr, "vid_init: ERROR in SDL_Init() %s\n", SDL_GetError());
			vid_close();
			return 1;
			}
		if(!(IMG_Init(imgFlags) & imgFlags )) {
			fprintf(stderr, "vid_init: ERROR in IMG_Init() %s\n", IMG_GetError());
			vid_close();
			return 1;
		}
		if(TTF_Init() < 0) {
			fprintf(stderr, "vid_init: ERROR in TTF_Init() %s\n", TTF_GetError());
			vid_close();
			return 1;
		}
	}
		//get current screen info
		SDL_DisplayMode mode;
		if (SDL_GetCurrentDisplayMode(0, &mode) != 0) {
   			fprintf(stderr, "%s\n", SDL_GetError());
    		return 1;
		}
		int nResX = mode.w;
		int nResY = mode.h;
		//configure the video mode
		SDL_ShowCursor(SDL_DISABLE);
		vid_window = SDL_CreateWindow(
    		"GBConsole",
   			SDL_WINDOWPOS_CENTERED,
   			SDL_WINDOWPOS_CENTERED,
   			nResX,
   			nResY,
    		SDL_WINDOW_SHOWN
		);

		if (!vid_window) {
   			fprintf(stderr,"%s\n",SDL_GetError());
    		return 1;
		}

		vid_scrMain = SDL_GetWindowSurface(vid_window);
	//create shade surface
	vid_shade = SDL_CreateRGBSurface(SURFACE_TYPE, vid_scrMain->w, vid_scrMain->h, vid_scrMain->format->BitsPerPixel, 
		vid_scrMain->format->Rmask, vid_scrMain->format->Gmask, vid_scrMain->format->Bmask, vid_scrMain->format->Amask);

	vid_isInitFlag = INIT_FLAG_FULL;
	return 0;
	}

// Gets the current width of the screen
int vid_getScreenWidth() {
	if(vid_scrMain) return vid_scrMain->w;
	return 0;
}

// Gets the current height of the screen
int vid_getScreenHeight() {
	if(vid_scrMain) return vid_scrMain->h;
	return 0;
}

// Gets the given textures width
int vid_getTextureWidth(VidTexture* t) {
	if(t != 0) return ((SDL_Surface*)t)->w;
	return 0;
}

// Gets the given textures height
int vid_getTextureHeight(VidTexture* t) {
	if(t != 0) return ((SDL_Surface*)t)->h;
	return 0;
}

// Saves the current screen as a bitmap
void vid_saveScreen(const char* file) {
	if(vid_scrMain && file) {
		SDL_SaveBMP(vid_scrMain, file);
	}
}

// Checks if the Video interface is initialized
char vid_isInit() {
	if(vid_isInitFlag == INIT_FLAG_FULL) return 1;
	return 0;
}

// Draws a box to the video buffer
void vid_drawBox(int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char opaque) {
	int i, j;
	if(vid_scrMain) {
		Uint32 color = SDL_MapRGBA(vid_scrMain->format, r, g, b, 0);
		SDL_Rect rectBox = {(signed short)x, (signed short)y, (unsigned short)w, (unsigned short)h};
		if(opaque == 255) {
			SDL_FillRect(vid_scrMain, &rectBox, color);
	} else {
			SDL_Rect shadeBox = {0, 0, (unsigned short)w, (unsigned short)h};
			SDL_FillRect(vid_shade, &shadeBox, color);
					
			SDL_SetSurfaceBlendMode(vid_shade, SDL_BLENDMODE_BLEND);
			SDL_SetSurfaceAlphaMod(vid_shade, opaque);
			SDL_BlitSurface(vid_shade, &shadeBox, vid_scrMain, &rectBox);
		}
	}
}

// Draws a texture to the video buffer
void vid_drawTexture(VidTexture* t, int x, int y)
{
	if(vid_scrMain && t != 0) {
		SDL_Surface* tSurface = (SDL_Surface*)t;
		SDL_Rect offset = {(signed short)x, (signed short)y, 0, 0};
		SDL_BlitSurface(tSurface, NULL, vid_scrMain, &offset);
	}
}

// Generates a texture from the given image file
VidTexture* vid_generateImageTexture(const char* filename, int w, int h, char smooth)
{
	if(vid_scrMain) {
		//image already cached?
		if(strcmp(filename, vid_cachedImageFilename) != 0) {
			if(vid_cachedImage) SDL_FreeSurface(vid_cachedImage);
			SDL_Surface* imgSurfaceRaw = IMG_Load(filename);
			vid_cachedImage = SDL_ConvertSurface(imgSurfaceRaw, vid_scrMain->format, SURFACE_TYPE);
			SDL_FreeSurface(imgSurfaceRaw);
			strncpy(vid_cachedImageFilename, filename, MAX_IMAGE_CACHE_FILENAME_SIZE);
			vid_cachedImageFilename[MAX_IMAGE_CACHE_FILENAME_SIZE-1] = 0;
		}
		
		SDL_Surface* imgSurface = NULL;
		if(vid_cachedImage->format->BitsPerPixel == 32 || vid_cachedImage->format->BitsPerPixel == 8) {
			imgSurface = zoomSurface(vid_cachedImage, (double)w/(double)(vid_cachedImage->w), (double)h/(double)(vid_cachedImage->h), smooth);
		} else {
			//make sure to convert back to optimized format
			SDL_Surface* imgSurfaceRaw2 = zoomSurface(vid_cachedImage, (double)w/(double)(vid_cachedImage->w), (double)h/(double)(vid_cachedImage->h), smooth);
			imgSurface = SDL_ConvertSurface(imgSurfaceRaw2, vid_scrMain->format, SURFACE_TYPE);
			SDL_FreeSurface(imgSurfaceRaw2);
		}
		
		return (VidTexture*)imgSurface;
	}
	return 0;
}

// Generates a texture from the given text
VidTexture* vid_generateTextTexture(const char* text, unsigned char rF, unsigned char gF, unsigned char bF, 
		unsigned char rB, unsigned char gB, unsigned char bB, char fontSize, char isBold)
{
	if(vid_scrMain) {
		//font already cached?
		if(vid_cachedFontSize != (int)fontSize) {
			if(vid_cachedFont) TTF_CloseFont(vid_cachedFont);
			vid_cachedFont = TTF_OpenFont("data/img/font.ttf", fontSize);
			vid_cachedFontSize = fontSize;
		}
		
		SDL_Color foregroundColor = {rF, gF, bF};
		SDL_Color backgroundColor = {rB, gB, bB};
		if(isBold) TTF_SetFontStyle(vid_cachedFont, TTF_STYLE_BOLD);
		else TTF_SetFontStyle(vid_cachedFont, TTF_STYLE_NORMAL);
		SDL_Surface* textSurfaceRaw = TTF_RenderText_Shaded(vid_cachedFont, text, foregroundColor, backgroundColor);
		SDL_Surface* textSurface = SDL_ConvertSurface(textSurfaceRaw, vid_scrMain->format, SURFACE_TYPE);
		
		SDL_FreeSurface(textSurfaceRaw);
		return (VidTexture*)textSurface;
	}
	return 0;
}

// Generates a texture from the current state of the screen
VidTexture* vid_generateScreenTexture(int x, int y, int w, int h)
{
	if(vid_scrMain) {
		SDL_Surface* screenSurface = SDL_CreateRGBSurface(SURFACE_TYPE, (unsigned short)w, (unsigned short)h, vid_scrMain->format->BitsPerPixel, 
			vid_scrMain->format->Rmask, vid_scrMain->format->Gmask, vid_scrMain->format->Bmask, vid_scrMain->format->Amask);
		SDL_Rect rect = {(signed short)x, (signed short)y, (unsigned short)w, (unsigned short)h};
		SDL_BlitSurface(vid_scrMain, &rect, screenSurface, NULL);
		
		return (VidTexture*)screenSurface;
	}
	return 0;
}

// Compsites the given image onto the given texture
void vid_compositeImageToTexture(VidTexture* t, const char* filename, unsigned char opaque, char smooth)
{
	if(vid_scrMain) {
		SDL_Surface* tSurface = (SDL_Surface*)t;
		
		SDL_Surface* imgSurfaceRaw = IMG_Load(filename);
		SDL_Surface* imgSurfaceZoomed = zoomSurface(imgSurfaceRaw, (double)tSurface->w/(double)(imgSurfaceRaw->w), (double)tSurface->h/(double)(imgSurfaceRaw->h), smooth);
		SDL_FreeSurface(imgSurfaceRaw);
		SDL_Surface* imgSurfaceOpt = SDL_ConvertSurface(imgSurfaceZoomed, vid_scrMain->format, SURFACE_TYPE);
		SDL_FreeSurface(imgSurfaceZoomed);
		
		SDL_SetSurfaceBlendMode(imgSurfaceOpt, SDL_BLENDMODE_BLEND);
		SDL_SetSurfaceAlphaMod(imgSurfaceOpt, opaque);
		SDL_BlitSurface(imgSurfaceOpt, NULL, tSurface, NULL);
		SDL_FreeSurface(imgSurfaceOpt);
	}
}

// Compsites the given color onto the given texture
void vid_compositeColorToTexture(VidTexture* t, unsigned char r, unsigned char g, unsigned char b, unsigned char opaque)
{
	if(vid_scrMain) {
		SDL_Surface* tSurface = (SDL_Surface*)t;
		
		SDL_Surface* colorSurface = SDL_CreateRGBSurface(SURFACE_TYPE, tSurface->w, tSurface->h, tSurface->format->BitsPerPixel, 
			tSurface->format->Rmask, tSurface->format->Gmask, tSurface->format->Bmask, tSurface->format->Amask);
		Uint32 color = SDL_MapRGBA(vid_scrMain->format, r, g, b, 0);
		SDL_Rect rectBox = {0, 0, (unsigned short)tSurface->w, (unsigned short)tSurface->h};
		SDL_FillRect(colorSurface, &rectBox, color);
		
		SDL_SetSurfaceBlendMode(colorSurface, SDL_BLENDMODE_BLEND);
		SDL_SetSurfaceAlphaMod(colorSurface, opaque);
		SDL_BlitSurface(colorSurface, NULL, tSurface, NULL);
		SDL_FreeSurface(colorSurface);
	}
}

// Clears memory for the given texture
void vid_clearTexture(VidTexture* t)
{
	if(t != 0) {
		SDL_Surface* tSurface = (SDL_Surface*)t;
		if(tSurface) SDL_FreeSurface(tSurface);
	}
}

// Flushes the video buffer to the screen
void vid_flush()
{
	if(vid_scrMain) {
		SDL_UpdateWindowSurface(vid_window);
	}
}

// Clears any cached elements (must reinitialize)
int vid_clear()
{
	//clear resources
	if(vid_shade) SDL_FreeSurface(vid_shade);
	vid_shade = NULL;
	if(vid_cachedImage) SDL_FreeSurface(vid_cachedImage);
	vid_cachedImage = NULL;
	vid_cachedImageFilename[0] = 0;
	if(vid_cachedFont) TTF_CloseFont(vid_cachedFont);
	vid_cachedFont = NULL;
	vid_cachedFontSize = -1;
	
	vid_isInitFlag = INIT_FLAG_PARTIAL;
	return 0;
}

// Closes the Video interface
int vid_close()
{
	//clear resources
	vid_scrMain = NULL;

	if (vid_window)
	{
    SDL_DestroyWindow(vid_window);
    vid_window = NULL;
	}
	
	//close down SDL
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
	
	vid_isInitFlag = INIT_FLAG_NOT;
	return 0;
}
