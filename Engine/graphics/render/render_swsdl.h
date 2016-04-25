#ifndef RENDER_SW_SDL_H
#define RENDER_SW_SDL_H

#include <common_features/rect.h>
#include "render_base.h"

struct SDL_Surface;
struct SDL_Renderer;
struct SDL_Texture;
class Render_SW_SDL : public Render_Base
{
public:
    Render_SW_SDL();
    virtual void set_SDL_settings();
    virtual bool init();
    virtual bool uninit();
    virtual void initDummyTexture();
    virtual PGE_Texture getDummyTexture();
    virtual void loadTexture(PGE_Texture &target, int width, int height, unsigned char* RGBApixels);
    virtual void deleteTexture(PGE_Texture &tx);
    virtual void deleteTexture(GLuint tx);
    virtual void getScreenPixels(int x, int y, int w, int h, unsigned char *pixels);
    virtual void setViewport(int x, int y, int w, int h);

    virtual void resetViewport();
    virtual void setViewportSize(int w, int h);
    virtual void setWindowSize(int w, int h);

    virtual void flush();
    virtual void repaint();

    /*!
     * \brief Changed color to clear screen
     * \param r level of red (from 0.0 to 1.0)
     * \param g level of green (from 0.0 to 1.0)
     * \param b level of blue (from 0.0 to 1.0)
     * \param a level of alpha (from 0.0 to 1.0)
     */
    virtual void setClearColor(float r, float g, float b, float a);
    /*!
     * \brief Clear screen with pre-defined clear color
     */
    virtual void clearScreen();

    virtual void renderRect(float x, float y, float w, float h, GLfloat red=1.f, GLfloat green=1.f, GLfloat blue=1.f, GLfloat alpha=1.f, bool filled=true);
    virtual void renderRectBR(float _left, float _top, float _right, float _bottom, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    virtual void renderTexture(PGE_Texture *texture, float x, float y);
    virtual void renderTexture(PGE_Texture *texture, float x, float y, float w, float h, float ani_top=0, float ani_bottom=1, float ani_left=0, float ani_right=1);

    virtual void BindTexture(PGE_Texture *texture);

    virtual void setRGB(float Red, float Green, float Blue, float Alpha=1.0);
    virtual void resetRGB();

    virtual void setTextureColor(float Red, float Green, float Blue, float Alpha=1.0f);
    virtual void renderTextureCur(float x, float y, float w, float h, float ani_top=0, float ani_bottom=1, float ani_left=0, float ani_right=1);
    //virtual void renderTextureCur(float x, float y);
    virtual void getCurWidth(GLint &w);
    virtual void getCurHeight(GLint &h);
    virtual void UnBindTexture();

    PGE_PointF MapToGl(PGE_Point point);
    PGE_PointF MapToGl(float x, float y);
    PGE_Point  MapToScr(PGE_Point point);
    PGE_Point  MapToScr(int x, int y);
    int  alignToCenter(int x, int w);
private:
    PGE_Texture _dummyTexture;

    //!The surface contained by the window
    SDL_Surface* screenSurface;

    //!The window renderer
    SDL_Renderer* m_gRenderer;

    unsigned char m_clearColor[4];

    SDL_Texture* m_currentTexture;
    PGE_Rect     m_currentTextureRect;

    //! Internal bank of textures
    std::vector<SDL_Texture*> m_textureBank;

    inline void setRenderTexture(GLuint &tID)
    {
        m_currentTexture = (tID >= m_textureBank.size()) ? NULL : m_textureBank[tID];
    }

    inline void setUnbindTexture()
    {
        m_currentTexture = NULL;
    }

    inline void setRenderColors()
    {
        setUnbindTexture();
    }

    //Virtual resolution of renderable zone
    int window_w;
    int window_h;

    //Scale of virtual and window resolutuins
    float scale_x;
    float scale_y;
    //Side offsets to keep ratio
    float offset_x;
    float offset_y;
    //current viewport
    float viewport_x;
    float viewport_y;
    //Need to calculate relative viewport position when screen was scaled
    float viewport_scale_x;
    float viewport_scale_y;
    //Resolution of viewport
    float viewport_w;
    float viewport_h;
    //Half values of viewport Resolution
    float viewport_w_half;
    float viewport_h_half;

    //Texture render color levels
    float color_level_red;
    float color_level_green;
    float color_level_blue;
    float color_level_alpha;

    float color_binded_texture[16];
};

#endif // RENDER_SW_SDL_H
