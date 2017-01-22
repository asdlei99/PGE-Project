/*
 * A small crossplatform set of file manipulation functions.
 * All input/output strings are UTF-8 encoded, even on Windows!
 *
 * Copyright (c) 2017 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "files.h"
#include <stdio.h>
#include <locale>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>

static std::wstring Str2WStr(const std::string &path)
{
    std::wstring wpath;
    wpath.resize(path.size());
    int newlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), path.length(), &wpath[0], path.length());
    wpath.resize(newlen);
    return wpath;
}
#else
#include <unistd.h>
#include <fcntl.h>         // open
#include <string.h>
#include <sys/stat.h>      // fstat
#include <sys/types.h>     // fstat
#include <cstdio>          // BUFSIZ
#endif

#if defined(__CYGWIN__) || defined(__DJGPP__) || defined(__MINGW32__)
#define IS_PATH_SEPARATOR(c) (((c) == '/') || ((c) == '\\'))
#else
#define IS_PATH_SEPARATOR(c) ((c) == '/')
#endif

static char fi_path_dot[] = ".";
static char fi_path_root[] = "/";

static char *fi_basename(char *s)
{
    char *rv;

    if(!s || !*s)
        return fi_path_dot;

    rv = s + strlen(s) - 1;

    do
    {
        if(IS_PATH_SEPARATOR(*rv))
            return rv + 1;
        --rv;
    }
    while(rv >= s);

    return s;
}

static char *fi_dirname(char *path)
{
    char *p;

    if(path == NULL || *path == '\0')
        return fi_path_dot;

    p = path + strlen(path) - 1;
    while(IS_PATH_SEPARATOR(*p))
    {
        if(p == path)
            return path;
        *p-- = '\0';
    }

    while(p >= path && !IS_PATH_SEPARATOR(*p))
        p--;

    if(p < path)
        return fi_path_dot;

    if(p == path)
        return fi_path_root;

    *p = '\0';
    return path;
}

bool Files::fileExists(const std::string &path)
{
    #ifdef _WIN32
    std::wstring wpath = Str2WStr(path);
    return PathFileExistsW(wpath.c_str()) == TRUE;
    #else
    FILE *ops = fopen(path.c_str(), "rb");
    if(ops)
    {
        fclose(ops);
        return true;
    }
    return false;
    #endif
}

bool Files::deleteFile(const std::string &path)
{
    #ifdef _WIN32
    std::wstring wpath = Str2WStr(path);
    return (DeleteFileW(wpath.c_str()) == TRUE);
    #else
    return ::unlink(path.c_str()) == 0;
    #endif
}

bool Files::copyFile(const std::string &to, const std::string &from, bool override)
{
    if(!override && fileExists(to))
        return false;// Don't override exist target if not requested

    bool ret = true;

    #ifdef _WIN32

    std::wstring wfrom  = Str2WStr(from);
    std::wstring wto    = Str2WStr(to);
    ret = (bool)CopyFileW(wfrom.c_str(), wto.c_str(), !override);

    #else

    char    buf[BUFSIZ];
    ssize_t size;
    ssize_t sizeOut;

    int source  = open(from.c_str(), O_RDONLY, 0);
    if(source == -1)
        return false;

    int dest    = open(to.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if(dest == -1)
    {
        close(source);
        return false;
    }

    while((size = read(source, buf, BUFSIZ)) > 0)
    {
        sizeOut = write(dest, buf, static_cast<size_t>(size));
        if(sizeOut != size)
        {
            ret = false;
            break;
        }
    }

    close(source);
    close(dest);
    #endif

    return ret;
}

bool Files::moveFile(const std::string& to, const std::string& from, bool override)
{
    bool ret = copyFile(to, from, override);
    if(ret)
        ret &= deleteFile(from);
    return ret;
}


std::string Files::dirname(std::string path)
{
    char *p = strdup(path.c_str());
    char *d = ::fi_dirname(p);
    path = d;
    free(p);
    return path;
}

std::string Files::basename(std::string path)
{
    char *p = strdup(path.c_str());
    char *d = ::fi_basename(p);
    path = d;
    free(p);
    return path;
}

std::string Files::changeSuffix(std::string path, const std::string &suffix)
{
    size_t pos = path.find_last_of('.');// Find dot
    if((path.size() < suffix.size()) || (pos == std::string::npos))
        path.append(suffix);
    else
        path.replace(pos, suffix.size(), suffix);
    return path;
}

bool Files::hasSuffix(const std::string &path, const std::string &suffix)
{
    if(suffix.size() > path.size())
        return false;

    std::locale loc;
    std::string f = path.substr(path.size() - suffix.size(), suffix.size());
    for(char &c : f)
        c = std::tolower(c, loc);
    return (f.compare(suffix) == 0);
}


bool Files::isAbsolute(const std::string& path)
{
    bool firstCharIsSlash = (path.size() > 0) ? path[0] == '/' : false;
    #ifdef _WIN32
    bool containsWinChars = (path.size() > 2) ? (path[1] == ':') && ((path[2] == '\\') || (path[2] == '/')) : false;
    if(firstCharIsSlash || containsWinChars)
    {
        return true;
    }
    return false;
    #else
    return firstCharIsSlash;
    #endif
}
