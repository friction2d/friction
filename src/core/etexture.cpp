/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "etexture.h"

void eTexture::bind(QGL33 * const gl) const {
    gl->glBindTexture(GL_TEXTURE_2D, fId);
}

void eTexture::clear(GrContext* const context) {
    if(fGrTex.isValid()) {
        context->deleteBackendTexture(fGrTex);
        fGrTex = GrBackendTexture();
    }
    fId = 0;
    fWidth = 0;
    fHeight = 0;
}

void eTexture::gen(GrContext* const context,
                  const int width, const int height) {
    fGrTex = context->createBackendTexture(width, height,
                                           kRGBA_8888_SkColorType,
                                           SkColors::kTransparent,
                                           GrMipMapped::kNo,
                                           GrRenderable::kYes);
    GrGLTextureInfo texInfo;
    fGrTex.getGLTextureInfo(&texInfo);
    fWidth = width;
    fHeight = height;
    fId = texInfo.fID;
}

#include "skia/skiahelpers.h"
SkBitmap eTexture::bitmapSnapshot(QGL33 * const gl) const {
    bind(gl);
    SkBitmap bitmap;
    const auto info = SkiaHelpers::getPremulRGBAInfo(fWidth, fHeight);
    bitmap.allocPixels(info);

#ifdef USE_GLES
    GLint currentTexture = 0;
    gl->glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);

    GLint oldFbo = 0;
    gl->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFbo);

    GLuint tempFbo = 0;
    gl->glGenFramebuffers(1, &tempFbo);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);

    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, currentTexture, 0);

    gl->glReadPixels(0, 0, fWidth, fHeight, GL_RGBA,
                     GL_UNSIGNED_BYTE, bitmap.getPixels());

    gl->glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
    gl->glDeleteFramebuffers(1, &tempFbo);
#else
    gl->glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA,
                      GL_UNSIGNED_BYTE, bitmap.getPixels());
#endif

    return bitmap;
}

sk_sp<SkImage> eTexture::imageSnapshot(QGL33 * const gl) const {
    auto bitmap = bitmapSnapshot(gl);
    return SkiaHelpers::transferDataToSkImage(bitmap);
}

void eTexture::swap(eTexture &otherTexture) {
    std::swap(fId, otherTexture.fId);
    std::swap(fWidth, otherTexture.fWidth);
    std::swap(fHeight, otherTexture.fHeight);
}
