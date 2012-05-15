/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 *   Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Vladimir Vukicevic <vladimir@pobox.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef WEBGLEXTENSIONS_H_
#define WEBGLEXTENSIONS_H_

namespace mozilla {

class WebGLExtensionLoseContext :
    public nsIWebGLExtensionLoseContext,
    public WebGLExtension
{
public:
    WebGLExtensionLoseContext(WebGLContext*);
    virtual ~WebGLExtensionLoseContext();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWEBGLEXTENSIONLOSECONTEXT
};

class WebGLExtensionStandardDerivatives :
    public nsIWebGLExtensionStandardDerivatives,
    public WebGLExtension
{
public:
    WebGLExtensionStandardDerivatives(WebGLContext* context);
    virtual ~WebGLExtensionStandardDerivatives();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWEBGLEXTENSION
};

class WebGLExtensionTextureFilterAnisotropic :
    public nsIWebGLExtensionTextureFilterAnisotropic,
    public WebGLExtension
{
public:
    WebGLExtensionTextureFilterAnisotropic(WebGLContext* context);
    virtual ~WebGLExtensionTextureFilterAnisotropic();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWEBGLEXTENSION
};

class WebGLExtensionCompressedTextureS3TC :
    public nsIWebGLExtensionCompressedTextureS3TC,
    public WebGLExtension
{
public:
    WebGLExtensionCompressedTextureS3TC(WebGLContext* context);
    virtual ~WebGLExtensionCompressedTextureS3TC();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWEBGLEXTENSION
};

}

#endif // WEBGLEXTENSIONS_H_