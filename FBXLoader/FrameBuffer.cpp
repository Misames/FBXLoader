
#include "FrameBuffer.hpp"

void Framebuffer::CreateFramebuffer(const uint32_t w, const uint32_t h, bool useDepth)
{
    width = (uint16_t)w;
    height = (uint16_t)h;

    // on bascule sur le sampler 0 (par defaut)
    glActiveTexture(GL_TEXTURE0);
    // creation de la texture servant de color buffer
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if (useDepth)
    {
        glGenTextures(1, &depthBuffer);
        glBindTexture(GL_TEXTURE_2D, depthBuffer);
        // format interne (3eme param) indique le format de stockage en memoire video, combine usage et taille des données
        // format (externe, 7eme et 8eme param) indique le format des donnees en RAM
        // notez que si le format interne est different du format les pilotes OpenGL peuvent proceder a une conversion couteuse
        // sauf en OpenGL ES / WebGL ou les conversions sont interdites
        // le format interne d'un depth buffer peut etre GL_DEPTH_COMPONENT16/24/32 en int, ou GL_DEPTH_COMPONENT32F en float
        // le format doit correspondre le plus possible ici GL_DEPTH_COMPONENT + GL_UNSIGNED_INT (GL_FLOAT si le format interne est GL_DEPTH_COMPONENT32F)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    // Cette ligne n'est pas necessaire ici, mais il s'agit de montrer que le FBO ne necessite pas qu'une texture
    // soit Bind pour etre utilisable comme attachment
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    // On attache ensuite le lod 0 (dernier param) de la texture 'colorBuffer' (4eme param) qui est de type GL_TEXTURE_2D (3eme param)
    // comme 'color attachment #0' (2eme param) de notre FBO precedemment bind comme GL_FRAMEBUFFER (1er param)
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);

    if (useDepth)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer invalide, code erreur = " << status << std::endl;
    }
}

void Framebuffer::DestroyFramebuffer()
{
    if (depthBuffer)
        glDeleteTextures(1, &depthBuffer);
    if (colorBuffer)
        glDeleteTextures(1, &colorBuffer);
    if (FBO)
        glDeleteFramebuffers(1, &FBO);
    depthBuffer = 0;
    colorBuffer = 0;
    FBO = 0;
}

void Framebuffer::EnableRender()
{
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glViewport(0, 0, width, height);
}

// force le rendu vers le backbuffer
void Framebuffer::RenderToBackBuffer(const uint32_t w, const uint32_t h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (w != 0 && h != 0)
        glViewport(0, 0, w, h);
}
